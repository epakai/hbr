/*
 * hbr - handbrake runner
 * Copyright (C) 2016 Joshua Honeycutt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libxml/xpath.h>

#include "gen_xml.h"
#include "xml.h"

const char *argp_program_version = "hbr 1.0";
const char *argp_program_bug_address = "<joshua.honeycutt@gmail.com>";

int global_mkv = 1;
xmlChar * global_input_basedir;
/*
 * PROTOTYPES
 */
static error_t parse_enc_opt (int key, char *arg, struct argp_state *);
xmlChar* hb_options_string(xmlDocPtr doc);
xmlChar* out_options_string(xmlDocPtr doc, int out_count);
int validate_file_string(xmlChar * file_string);
int validate_bit_rate(int bitrate, int minimum, int maximum);
int call_handbrake(xmlChar *hb_command, int out_count, int overwrite);
int hb_fork(xmlChar *hb_command, xmlChar *log_filename,
		xmlChar *filename, int out_count);

static char enc_doc[] = "handbrake runner -- runs handbrake with setting from an "
"xml file with all encoded files placed in current directory";
static char enc_args_doc[] = "<XML FILE>";

static struct argp_option enc_options[] = {
	{"in",        'i', "FILE", 0, "handbrake_encode XML File", 0},
	{"episode",   'e', "NUM",  0, "only encodes for entry with matching episode number", 1},
	{"preview",   'p', 0,      0, "generate a preview image for each output file", 1},
	{"overwrite", 'y', 0,      0, "overwrite encoded files without confirmation", 1},
	{"help",     '?', 0,              0, "Give this help list", 2},
	{"usage",    '@', 0,  OPTION_HIDDEN, "Give this help list", 2},
	{ 0, 0, 0, 0, 0, 0 }
};


struct enc_arguments {
	char *args[1];
	int episode, overwrite, preview;
};

static struct argp enc_argp = {enc_options, parse_enc_opt, enc_args_doc,
	enc_doc, NULL, 0, 0};

/*************************************************/

int main(int argc, char * argv[])
{
	struct enc_arguments enc_arguments;
	enc_arguments.episode = -1;
	enc_arguments.overwrite = 0;
	enc_arguments.preview = 0;
	struct gen_arguments gen_arguments = {-1, 0, 0, -1, 0,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL};

	// check for request to generate new xml file
	argp_parse(&gen_argp, argc, argv,
			ARGP_IN_ORDER|ARGP_NO_EXIT|ARGP_SILENT,
			0, &gen_arguments);

	if ( gen_arguments.generate > 0 ) {
		// Call gen_xml with passed arguments or defaults
		gen_xml(gen_arguments.generate, gen_arguments.title ?: 1,
				gen_arguments.season ?: 1, gen_arguments.video_type,
				gen_arguments.markers, gen_arguments.source ?: "",
				gen_arguments.year ?: "", gen_arguments.crop ?: "0:0:0:0",
				gen_arguments.name ?: "", gen_arguments.format ?: "mkv",
				gen_arguments.basedir ?: "", gen_arguments.episodes);
		return 0;
	}
	// parse normal options
	argp_parse (&enc_argp, argc, argv, ARGP_NO_HELP, 0, &enc_arguments);
	// parse xml document to tree
	xmlDocPtr xml_doc = parse_xml(enc_arguments.args[0]);
	if (xml_doc == NULL) {
		return 1;
	}
	xmlNode *root_element = xmlDocGetRootElement(xml_doc);
	if (xmlStrcmp(root_element->name, (const xmlChar *) "handbrake_encode")) {
		fprintf(stderr,
				"Wrong document type: handbrake_encode element not found in \"%s\"",
				xml_doc->URL);
		return 1;
	}

	//Verify input_basedir
	xmlNode *cur = root_element->children;
	while (cur && xmlStrcmp(cur->name, (const xmlChar *) "handbrake_options")) {
		cur = cur->next;
	}
	global_input_basedir = xmlGetNoNsProp(cur, (const xmlChar *) "input_basedir");
	xmlChar bad_chars[4];
	bad_chars[0] = '\\';
	bad_chars[1] = '`';
	bad_chars[2] = '\"';
	int i;
	for (i = 0; i<3; i++) {
		const xmlChar * temp;
		if ((temp = xmlStrchr(global_input_basedir, bad_chars[i])) != NULL ){
			fprintf(stderr, "Invalid character '%c' for global_input_basedir attribute in "
					"\"%s\" line number: ~%ld\n",
					global_input_basedir[temp-global_input_basedir],
					xml_doc->URL, xmlGetLineNo(cur)-1);
			xmlFreeDoc(xml_doc);
			xmlFree(global_input_basedir);
			return 1;
		}
	}
	errno = 0;
	if ( access((char *) global_input_basedir, R_OK|X_OK ) == -1 ) {
		perror ("main(): input_basedir was inaccessible");
		xmlFreeDoc(xml_doc);
		xmlFree(global_input_basedir);
		return 1;
	}

	//assemble call to HandBrakeCLI
	xmlChar *hb_options = NULL;
	hb_options = hb_options_string(xml_doc);
	if ( hb_options[0] == '\0'){
		fprintf(stderr,
				"Unknown error: handbrake_options was empty after parsing \"%s\".\n",
				xml_doc->URL);
		xmlFree(hb_options);
		xmlFreeDoc(xml_doc);
		return 1;
	}

	// loop for each out_file tag in xml_doc
	int out_count = outfile_count(xml_doc);
	if (out_count < 1) {
		fprintf(stderr, "No valid outfiles found in \"%s\"\n", xml_doc->URL);
	}
	i = 1; // start at outfile 1
	// Handle -e option to encode a single episode
	if (enc_arguments.episode >= 0) {
		// adjust parameters for following loop so it runs once
		int outfile_number = get_outfile_from_episode(xml_doc, enc_arguments.episode);
		i = outfile_number;
		out_count = outfile_number;
	}
	// encode all the episodes if loop parameters weren't modified above
	for (; i <= out_count; i++) {
		xmlChar *out_options = NULL;
		out_options = out_options_string(xml_doc, i);
		if ( out_options[0] == '\0'){
			xmlNode* outfile_node = get_outfile(xml_doc, i);
			fprintf(stderr,
					"%d: Bad outfile element in \"%s\" line number: "
					"%ld  was not encoded\n",
					i, xml_doc->URL, xmlGetLineNo(outfile_node));
			xmlFree(out_options);
			continue;
		}
		// build full HandBrakeCLI command
		xmlChar *hb_command = xmlCharStrdup("HandBrakeCLI");
		hb_command = xmlStrcat(hb_command, hb_options);
		hb_command = xmlStrcat(hb_command, out_options);

		const xmlChar *filename_start = xmlStrstr(hb_command, (const xmlChar *) "-o")+4;
		const xmlChar *filename_end = xmlStrstr(filename_start, (const xmlChar *) "\"");
		xmlChar *filename = xmlStrsub(filename_start, 0, filename_end-filename_start);

		printf("DEBUG: hb_command: %s\n", hb_command);
		printf("Encoding: %d/%d: %s\n", i, out_count, filename);
		if (call_handbrake(hb_command, i, enc_arguments.overwrite) == -1) {
			xmlNode* outfile_node = get_outfile(xml_doc, i);
			fprintf(stderr,
					"%d: Handbrake call failed in \"%s\" line number: "
					"%ld  was not encoded\n",
					i, xml_doc->URL, xmlGetLineNo(outfile_node));
			xmlFree(out_options);
			xmlFree(hb_command);
			continue;
		}
		// produce a thumbnail
		if (enc_arguments.preview) {
			xmlChar *ft_command = xmlCharStrdup("ffmpegthumbnailer");
			ft_command = xmlStrncat(ft_command, (xmlChar *) " -i \"", 5);
			ft_command = xmlStrncat(ft_command, filename, xmlStrlen(filename));
			ft_command = xmlStrncat(ft_command, (xmlChar *) "\" -o \"", 6);
			ft_command = xmlStrncat(ft_command, filename, xmlStrlen(filename));
			ft_command = xmlStrncat(ft_command, (xmlChar *) ".png\" -s0 -q10 &>/dev/null", 14);
			printf("Generating preview: %d/%d: %s.png\n", i, out_count, filename);
			system((char *) ft_command);
			xmlFree(ft_command);
		}
		xmlFree(filename);
		xmlFree(out_options);
		xmlFree(hb_command);
	}
	xmlFreeDoc(xml_doc);
	xmlFree(hb_options);
	xmlFree(global_input_basedir);

	return 0;
}

static error_t parse_enc_opt (int key, char *arg, struct argp_state *state)
{
	struct enc_arguments *enc_arguments = (struct enc_arguments *) state->input;
	char prog_name[] = "hbr";
	switch (key) {
		case '@': // --usage
		case '?':
			argp_help(&enc_argp, stdout, ARGP_HELP_SHORT_USAGE, prog_name);
			argp_help(&enc_argp, stdout, ARGP_HELP_PRE_DOC, prog_name);
			argp_help(&enc_argp, stdout, ARGP_HELP_LONG, prog_name);

			printf("\nUsage: hbr %s [OPTION...]\n", gen_args_doc);
			argp_help(&gen_argp, stdout, ARGP_HELP_PRE_DOC, prog_name);
			argp_help(&gen_argp, stdout, ARGP_HELP_LONG, prog_name);
			exit(0);
			break;
		case 'e':
			enc_arguments->episode = atoi(arg);
			break;
		case 'y':
			enc_arguments->overwrite =  1;
			break;
		case 'p':
			enc_arguments->preview =  1;
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num >= 1) {
				/* Too many arguments. */
				argp_usage (state);
			}
			enc_arguments->args[state->arg_num] = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 1) {
				/* Not enough arguments. */
				argp_usage (state);
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

xmlChar* hb_options_string(xmlDocPtr doc)
{
	xmlNode *root_element = xmlDocGetRootElement(doc);
	// Find the handbrake_options element
	xmlNode *cur = root_element->children;
	while (cur && xmlStrcmp(cur->name, (const xmlChar *) "handbrake_options")) {
		cur = cur->next;
	}

	// allocate and initialize options string
	xmlChar *opt_str = xmlCharStrdup("\0");

	// Build options string
	xmlChar *temp;

	opt_str = xmlStrncat(opt_str, (const xmlChar *) " -f ", 4);
	temp = xmlGetNoNsProp(cur, (const xmlChar *) "format");
	if (xmlStrcmp(temp, (const xmlChar *) "mkv") == 0){
		opt_str = xmlStrncat(opt_str, (const xmlChar *) "av_mkv", 6);
		global_mkv=1;
	} else {
		opt_str = xmlStrncat(opt_str, (const xmlChar *) "av_mp4", 6);
		global_mkv=0;
	}
	xmlFree(temp);

	opt_str = xmlStrncat(opt_str, (const xmlChar *) " -e ", 4);
	temp = xmlGetNoNsProp(cur, (const xmlChar *) "video_encoder");
	opt_str = xmlStrncat(opt_str, temp, xmlStrlen(temp));
	xmlChar * quality_string;
	if ((quality_string = xmlGetNoNsProp(cur, (const xmlChar *) "video_quality"))[0] != '\0') {
		long quality = strtol((char *) quality_string, NULL, 10);
		int good_quality= 0;
		if ( xmlStrlen(quality_string) <= 2
				&& isdigit(quality_string[0])
				&& isdigit(quality_string[1])) {
			good_quality = 1;
			if ( xmlStrncmp(temp, (const xmlChar *) "x264", 4) == 0) {
				if ( 0 <= quality && quality <=51 ) {
					good_quality= 2;
				}
			} else if ( xmlStrncmp(temp, (const xmlChar *) "mpeg4", 5) == 0) {
				if ( 1 <= quality && quality <=31 ) {
					good_quality= 2;
				}
			} else if ( xmlStrncmp(temp, (const xmlChar *) "mpeg2", 5) == 0) {
				if ( 1 <= quality && quality <=31 ) {
					good_quality= 2;
				}
			} else if ( xmlStrncmp(temp, (const xmlChar *) "VP8", 3) == 0) {
				if ( 0 <= quality && quality <=63 ) {
					good_quality= 2;
				}
			} else if ( xmlStrncmp(temp, (const xmlChar *) "theora", 6) == 0) {
				if ( 0 <= quality && quality <=63 ) {
					good_quality= 2;
				}
			}
		}
		if (good_quality == 0){
			fprintf(stderr,
					"Invalid video quality '%s' for %s encoder in "
					"\"%s\" line number: ~%ld\n",
					quality_string, temp, doc->URL, xmlGetLineNo(cur)-9);
		} else if (good_quality == 1) {
			fprintf(stderr,
					"Video quality '%s' out of range for %s encoder in "
					"\"%s\" line number: ~%ld\n",
					quality_string, temp, doc->URL, xmlGetLineNo(cur)-9);
		} else if (good_quality == 2) {
			opt_str = xmlStrncat(opt_str, (const xmlChar *) " -q ", 4);
			opt_str = xmlStrncat(opt_str, quality_string, 2);
		}

	}
	xmlFree(temp);
	xmlFree(quality_string);

	opt_str = xmlStrncat(opt_str, (const xmlChar *) " -E ", 4);
	temp = xmlGetNoNsProp(cur, (const xmlChar *) "audio_encoder");
	opt_str = xmlStrncat(opt_str, temp, xmlStrlen(temp));
	if (xmlStrcmp(temp, (const xmlChar *) "mp3") == 0
			|| xmlStrcmp(temp, (const xmlChar *) "vorbis") == 0
			|| xmlStrcmp(temp, (const xmlChar *) "copy:mp3") == 0) {
		xmlChar *temp2, *temp3;
		if ((temp2 = xmlGetNoNsProp(cur, (const xmlChar *) "audio_quality"))[0] != '\0') {
			// verify quality is within bounds
			float a_quality = strtof((char *) temp2, NULL);
			xmlFree(temp2);
			if (temp[0] == 'm') { //mp3
				if (a_quality > 0.0 && a_quality < 10.0) {
					char temp2[5];
					snprintf(temp2, 4, "%f.1", a_quality);
				}
			}
			if (temp[0] == 'v') { //vorbis
				if (a_quality > -2.0 && a_quality < 10.0) {
					char temp2[5];
					snprintf(temp2, 4, "%f.1", a_quality);
				}
			}
			opt_str = xmlStrncat(opt_str, (const xmlChar *) " -Q ", 4);
			opt_str = xmlStrncat(opt_str, temp2, xmlStrlen(temp2));
		} else if ( (temp3 = xmlGetNoNsProp(cur, (const xmlChar *) "audio_bitrate"))[0] != '\0'){
			// set bitrate for mp3/vorbis if no quality found

			// Only tests subset of bit rates for vorbis/mp3 (vorbis can go a bit higher)
			int bitrate = strtol((char *) temp3, NULL, 10);
			// test firs char of audio encoder and for valid bit rate
			if ( ((temp[0] == 'm' || temp[0] == 'c') && validate_bit_rate(bitrate, 32, 320))
					|| (temp[0] == 'v' && validate_bit_rate(bitrate, 192, 1344)) ){
				opt_str = xmlStrncat(opt_str, (const xmlChar *) " -B ", 4);
				opt_str = xmlStrncat(opt_str, temp3, xmlStrlen(temp3) );
			} else {
				// make bad guess at line numbers since there doesn't seem to be
				// a way to get attributes' line number
				fprintf(stderr,
						"Invalid bitrate '%d' for %s codec in \"%s\" line number: ~%ld\n",
						bitrate, temp, doc->URL, xmlGetLineNo(cur)-7);
			}
			xmlFree(temp3);
		}
		xmlFree(temp2);
	} else { // set bitrate for all other codecs
		xmlChar *temp2;
		temp2 = xmlGetNoNsProp(cur, (const xmlChar *) "audio_bitrate");
		if ( temp2[0] != '\0'){
			int bitrate = strtol((char *) temp2, NULL, 10);
			int good_bitrate = 0;
			if ( ((xmlStrcmp(temp, (const xmlChar *) "av_aac") == 0)
						|| (xmlStrcmp(temp, (const xmlChar *) "copy:aac") == 0))
					&& validate_bit_rate(bitrate, 192, 1536 )) {
				good_bitrate = 1;
			}
			if ( ((xmlStrcmp(temp, (const xmlChar *) "fdk_aac") == 0)
						|| (xmlStrcmp(temp, (const xmlChar *) "copy:dtshd") == 0)
						|| (xmlStrcmp(temp, (const xmlChar *) "copy") == 0)
						|| (xmlStrcmp(temp, (const xmlChar *) "copy:dts") == 0))
					&& validate_bit_rate(bitrate, 160, 1344 )) {
				good_bitrate = 1;
			}
			if ( ((xmlStrcmp(temp, (const xmlChar *) "ac3") == 0)
						|| ( xmlStrcmp(temp, (const xmlChar *) "copy:ac3") == 0))
					&& validate_bit_rate(bitrate, 224, 640)) {
				good_bitrate = 1;
			}
			if ( xmlStrcmp(temp, (const xmlChar *) "fdk_haac") == 0
					&& validate_bit_rate(bitrate, 80, 256)) {
				good_bitrate = 1;
			}
			if ( xmlStrncmp(temp, (const xmlChar *) "flac", 4) == 0) {
			}
			if (good_bitrate == 0) {
				fprintf(stderr,
						"Invalid bitrate '%d' for %s codec in \"%s\" line number: ~%ld\n",
						bitrate, temp, doc->URL, xmlGetLineNo(cur)-7);
			} else if (good_bitrate == 1) {
				opt_str = xmlStrncat(opt_str, (const xmlChar *) " -B ", 4);
				opt_str = xmlStrncat(opt_str, temp2, xmlStrlen(temp2));
			}
		}
		xmlFree(temp2);
	}
	xmlFree(temp);

	temp = xmlGetNoNsProp(cur, (const xmlChar *) "markers");
	if (xmlStrcmp(temp, (const xmlChar *) "yes") == 0 ) {
		opt_str = xmlStrncat(opt_str, (const xmlChar *) " -m", 3);
	}
	xmlFree(temp);

	temp = xmlGetNoNsProp(cur, (const xmlChar *) "anamorphic");
	if (xmlStrcmp(temp, (const xmlChar *) "strict") == 0 ) {
		opt_str = xmlStrncat(opt_str, (const xmlChar *) " --strict-anamorphic", 20);
	}
	if (xmlStrcmp(temp, (const xmlChar *) "loose") == 0 ) {
		opt_str = xmlStrncat(opt_str, (const xmlChar *) " --loose-anamorphic", 19);
	}
	xmlFree(temp);

	//deinterlace != none overrides decomb (check for 'n' is actually "none")
	xmlChar *temp2;
	if ((temp = xmlGetNoNsProp(cur, (const xmlChar *) "deinterlace"))[0] != 'n') {
		opt_str = xmlStrncat(opt_str, (const xmlChar *) " -d ", 4);
		opt_str = xmlStrncat(opt_str, temp, xmlStrlen(temp));
		xmlFree(temp);
	} else if ((temp2= xmlGetNoNsProp(cur, (const xmlChar *) "decomb"))[0] != 'n') {
		xmlFree(temp);
		if (xmlStrcmp(temp2, (const xmlChar *) "none") != 0){
		} else {
			opt_str = xmlStrncat(opt_str, (const xmlChar *) " -5 ", 4);
			if (xmlStrcmp(temp2, (const xmlChar *) "default") != 0) {
				opt_str = xmlStrncat(opt_str, temp2, xmlStrlen(temp2));
			}
		}
		xmlFree(temp2);
	} else {
		xmlFree(temp);
		xmlFree(temp2);
	}

	if ((temp = xmlGetNoNsProp(cur, (const xmlChar *) "denoise"))[0] != 'n') {
		opt_str = xmlStrncat(opt_str, (const xmlChar *) " -8 ", 4);
		opt_str = xmlStrncat(opt_str, temp, xmlStrlen(temp));
	}
	xmlFree(temp);

	opt_str = xmlStrncat(opt_str, (const xmlChar *) " ", 1);
	return opt_str;
}

int validate_bit_rate(int bitrate, int minimum, int maximum)
{
	// list is slightly reordered to put common rates first
	int valid_bitrates[] = { 128, 160, 192, 224, 256, 320,
		384, 448, 512, 40, 48, 56, 64, 80, 96, 112, 576, 640,
		768, 960, 1152, 1344, 1536, 2304, 3072, 4608, 6144 };
	int i;
	for (i=0; i<27; i++) {
		if (bitrate == valid_bitrates[i]){
			if (bitrate > minimum && bitrate < maximum) {
				return 1; //valid
			}
		}
	}
	return 0;
}

xmlChar* out_options_string(xmlDocPtr doc, int out_count)
{
	// output filename stuff
	xmlChar *type, *name, *year, *season, *episode_number, *specific_name;
	// handbrake options
	xmlChar *iso_filename, *dvdtitle, *crop, *chapters, *audio, *subtitle;
	xmlChar *out_options;
	int badstring = 0;

	// Fetch all tag contents for a single outfile
	xmlChar* tag_name[] = {(xmlChar *) "type",
		(xmlChar *) "name",
		(xmlChar *) "year",
		(xmlChar *) "season",
		(xmlChar *) "episode_number",
		(xmlChar *) "specific_name",
		(xmlChar *) "iso_filename",
		(xmlChar *) "dvdtitle",
		(xmlChar *) "crop",
		(xmlChar *) "chapters",
		(xmlChar *) "audio",
		(xmlChar *) "subtitle"};

	type = get_outfile_child_content(doc, out_count, tag_name[0]);
	name = get_outfile_child_content(doc, out_count, tag_name[1]);
	year = get_outfile_child_content(doc, out_count, tag_name[2]);
	season = get_outfile_child_content(doc, out_count, tag_name[3]);
	episode_number = get_outfile_child_content(doc, out_count, tag_name[4]);
	specific_name = get_outfile_child_content(doc, out_count, tag_name[5]);

	iso_filename = get_outfile_child_content(doc, out_count, tag_name[6]);
	dvdtitle = get_outfile_child_content(doc, out_count, tag_name[7]);
	crop = get_outfile_child_content(doc, out_count, tag_name[8]);
	chapters = get_outfile_child_content(doc, out_count, tag_name[9]);
	audio = get_outfile_child_content(doc, out_count, tag_name[10]);
	subtitle = get_outfile_child_content(doc, out_count, tag_name[11]);

	// Test all tag contents for bad characters
	xmlChar *validate_array[] = {type, name, year, season, episode_number, specific_name,
		iso_filename, dvdtitle, chapters, audio, subtitle};
	int i;
	for (i=0; i<11; i++) {
		int ch;
		if ((ch = validate_file_string(validate_array[i])) != 0) {
			fprintf(stderr, "%d: Invalid character '%c' in \"%s\" tag "
					"<%s> line number: %ld\n",
					out_count, validate_array[i][ch], doc->URL, tag_name[i],
					get_outfile_line_number(doc, out_count, tag_name[i]) );
			badstring = 1;
		}
	}

	//Begin building out_options
	out_options = xmlStrndup((const xmlChar *) "-o \"", 4);
	char *cwd = getcwd(NULL, 0);
	out_options = xmlStrcat(out_options, (xmlChar *) cwd);
	free(cwd);
	out_options = xmlStrncat(out_options, (const xmlChar *) "/", 1);

	// Verify <name> is non-empty
	int name_len = xmlStrlen(name);
	if ( name_len == 0 ) {
		fprintf(stderr, "%d: Empty name in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, tag_name[1],
				get_outfile_line_number(doc, out_count, tag_name[1]) );
		badstring = 1;
	}
	out_options = xmlStrncat(out_options, name, name_len);
	// Build filename for <type>series
	// series name = -o <name> - s<season>e<episode_number> - <specific_name>
	if ( xmlStrcmp(type, (const xmlChar *) "series") == 0 ){
		out_options = xmlStrncat(out_options, (const xmlChar *) " - ", 3);
		// add season if episode also exists
		// Season is a one or two digit number, left padded with a 0 if one digit
		if (season[0] != '\0' && episode_number[0] != '\0') {
			if (xmlStrlen(season) <= 2 && isdigit(season[0]) && (isdigit(season[1])
						|| season[1] == '\0')) {
				out_options = xmlStrncat(out_options, (const xmlChar *) "s", 1);
				if (xmlStrlen(season) == 1){
					out_options = xmlStrncat(out_options, (const xmlChar *) "0", 1);
				}
				out_options = xmlStrncat(out_options, season, xmlStrlen(season));
			} else {
				fprintf(stderr, "%d: Invalid season in \"%s\" tag <%s> line number: %ld\n",
						out_count, doc->URL, tag_name[3],
						get_outfile_line_number(doc, out_count, tag_name[3]) );
				badstring = 1;
			}
		}
		//Verify episode number is at most 3 digits, left pad with 0s
		for (i=0; i<4; i++) {
			if ( isdigit(episode_number[i]) && i < 3) {
				continue;
			} else if ( episode_number[i] == '\0' && i > 0) {
				out_options = xmlStrncat(out_options, (const xmlChar *) "e", 1);
				if (i == 1) {
					out_options = xmlStrncat(out_options, (const xmlChar *) "00", 2);
				}
				if (i == 2) {
					out_options = xmlStrncat(out_options, (const xmlChar *) "0", 1);
				}
				out_options = xmlStrncat(out_options, episode_number, xmlStrlen(episode_number));
				out_options = xmlStrncat(out_options, (const xmlChar *) " - ", 3);
				break;
			} else {
				fprintf(stderr, "%d: Invalid episode number in \"%s\" tag "
						"<%s> line number: %ld\n",
						out_count, doc->URL, tag_name[4],
						get_outfile_line_number(doc, out_count, tag_name[4]) );
				badstring = 1;
			}
		}
		if (specific_name[0] != '\0') {
			out_options = xmlStrncat(out_options, specific_name, xmlStrlen(specific_name));
		}
		if (xmlStrlen(out_options) > 249){
			xmlChar * temp_out_options = xmlStrndup(out_options, 249);
			temp_out_options[249] = '\0';
			xmlFree(out_options);
			out_options = temp_out_options;
		}
		if (global_mkv) {
			out_options = xmlStrncat(out_options, (const xmlChar *) ".mkv\"", 5);
		} else {
			out_options = xmlStrncat(out_options, (const xmlChar *) ".mp4\"", 5);
		}
		// Build filename for <type>movie
		// movie name = -o <name> (<year>) - <specific_name>
	} else if ( xmlStrcmp(type, (const xmlChar *) "movie") == 0 ){

		for (i=0; i<5; i++) {
			if ( isdigit(year[i]) && i < 4) {
				continue;
			} else if ( year[i] == '\0' && i == 4) {
				out_options = xmlStrncat(out_options, (const xmlChar *) " (", 2);
				out_options = xmlStrncat(out_options, year, xmlStrlen(year));
				out_options = xmlStrncat(out_options, (const xmlChar *) ")", 1);
			} else {
				fprintf(stderr,
						"%d: Invalid year in \"%s\" tag <%s> line number: %ld\n",
						out_count, doc->URL, tag_name[2],
						get_outfile_line_number(doc, out_count, tag_name[2]) );
				badstring = 1;
			}
		}
		if (specific_name[0] != '\0') {
			out_options = xmlStrncat(out_options, (const xmlChar *) " - ", 3);
			out_options = xmlStrncat(out_options, specific_name, xmlStrlen(specific_name));
		}
		if (xmlStrlen(out_options) > 249){
			xmlChar * temp_out_options = xmlStrndup(out_options, 249);
			temp_out_options[249] = '\0';
			xmlFree(out_options);
			out_options = temp_out_options;
		}
		if (global_mkv) {
			out_options = xmlStrncat(out_options, (const xmlChar *) ".mkv\"", 5);
		}
		else {
			out_options = xmlStrncat(out_options, (const xmlChar *) ".mp4\"", 5);
		}
	} else {
		fprintf(stderr, "%d: Invalid type in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, tag_name[0],
				get_outfile_line_number(doc, out_count, tag_name[0]) );
		badstring = 1;
	}

	// Set input filename based off input_basedir
	out_options = xmlStrncat(out_options, (const xmlChar *) " -i \"", 5);
	int gib_length = xmlStrlen(global_input_basedir);
	out_options = xmlStrncat(out_options, global_input_basedir, gib_length);
	if (global_input_basedir[gib_length-1] != '/') {
		out_options = xmlStrncat(out_options, (const xmlChar *) "/", 1);
	}
	out_options = xmlStrncat(out_options, iso_filename, xmlStrlen(iso_filename) );
	out_options = xmlStrncat(out_options, (const xmlChar *) "\"", 1);

	// check input filename exists or error
	xmlChar* full_path = xmlStrdup(global_input_basedir);
	if (global_input_basedir[gib_length-1] != '/') {
		full_path = xmlStrncat(full_path, (const xmlChar *) "/", 1);
	}
	full_path = xmlStrncat(full_path, iso_filename, xmlStrlen(iso_filename) );
	errno = 0;
	if ( access((char *) full_path, R_OK) == -1 ) {
		fprintf(stderr, "%s", full_path);
		perror (" out_options_string(): Input file was inaccessible");
		badstring = 1;
	}
	xmlFree(full_path);

	// Set dvd title
	if (xmlStrlen(dvdtitle) <= 2 && isdigit(dvdtitle[0])
			&& (isdigit(dvdtitle[1]) || dvdtitle[1] == '\0')) {
		out_options = xmlStrncat(out_options, (const xmlChar *) " -t ", 4);
		out_options = xmlStrncat(out_options, dvdtitle, xmlStrlen(dvdtitle));
	} else {
		fprintf(stderr, "%d: Invalid dvdtitle in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, tag_name[7],
				get_outfile_line_number(doc, out_count, tag_name[7]) );
		badstring = 1;
	}

	// Test crop values to be all digits and smaller than the video size
	if (crop != '\0') {
		xmlChar *temp = xmlStrdup(crop);
		char *crop_str[4];
		// break crop into components
		crop_str[0] = strtok((char *) temp, ":");
		crop_str[1] = strtok(NULL, ":");
		crop_str[2] = strtok(NULL, ":");
		crop_str[3] = strtok(NULL, "\0");
		// verify each crop is 4 digit max (video isn't that big yet)
		if ((strnlen(crop_str[0], 5) <= 4) && (strnlen(crop_str[1], 5) <= 4)
				&& (strnlen(crop_str[2], 5) <= 4) && (strnlen(crop_str[3], 5) <= 4)){
			int non_digit_found = 0;
			int i;
			// verify all characters are digits
			for (i = 0; i<4; i++){
				int j = 0;
				while (crop_str[i][j] != '\0'){
					if (!isdigit(crop_str[i][j])){
						non_digit_found = 1;
					}
					j++;
				}
			}
			if (non_digit_found == 0){
				out_options = xmlStrncat(out_options, (const xmlChar *) " -c ", 4);
				out_options = xmlStrncat(out_options, crop, xmlStrlen(crop));
				//re-grab the property because strtok destroyed it
			} else {
				fprintf(stderr, "%d: Invalid crop (non-digits) "
						"'%s:%s:%s:%s' in \"%s\" line number: %ld\n",
						out_count, crop_str[0], crop_str[1], crop_str[2], crop_str[3],
						doc->URL, get_outfile_line_number(doc, out_count, tag_name[8]) );
			}
		} else {
			fprintf(stderr, "%d: Invalid crop (values too large) "
					"'%s:%s:%s:%s' in \"%s\" line number: %ld\n",
					out_count, crop_str[0], crop_str[1], crop_str[2], crop_str[3],
					doc->URL, get_outfile_line_number(doc, out_count, tag_name[8]) );
		}
		xmlFree(temp);
	}

	// Verify chapters matches range (1-3) or single number
	int dash_count = 0;
	int non_digit_dash_found = 0;
	// Check for digits and count '-' characters
	for (i = 0; i< xmlStrlen(chapters); i++) {
		if (!isdigit(chapters[i]) && chapters[i] != '-' ) {
			non_digit_dash_found = 1;
		}
		if (chapters[i] == '-') {
			dash_count++;
		}
	}
	// Ensure only digits and '-'
	if (non_digit_dash_found) {
		fprintf(stderr, "%d: Invalid character(s) in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, tag_name[9],
				get_outfile_line_number(doc, out_count, tag_name[9]) );
		badstring = 1;
		// Ensure single '-' and '-' is not first or last character
	} else if (dash_count > 1 || chapters[0] == '-' || chapters[xmlStrlen(chapters)] == '-') {
		fprintf(stderr, "%d: Bad usage of '-' (%s) in \"%s\" tag <%s> line number: %ld\n",
				out_count, chapters, doc->URL, tag_name[9],
				get_outfile_line_number(doc, out_count, tag_name[9]) );
		badstring = 1;
	} else {
		// convert to longs for testing
		int first = strtol((const char *) chapters, NULL, 10);
		if (first > 98 || first < 1) {
			fprintf(stderr, "%d: Chapter value outside range (1-98) "
					"(%s) in \"%s\" tag <%s> line number: %ld\n",
					out_count, chapters, doc->URL, tag_name[8],
					get_outfile_line_number(doc, out_count, tag_name[9]) );
			badstring = 1;
		}
		// Deal with second digit if a single '-' was found
		if (dash_count == 1) {
			int second;
			//substring starts with -, increment pointer once
			const xmlChar *temp = xmlStrstr(chapters, (const xmlChar *) "-") + 1;
			second = strtol((const char *) temp, NULL, 10);
			if (first > second) {
				printf("first: %d second: %d\n", first, second);
				fprintf(stderr, "%d: Initial chapter is after final chapter "
						"(%s) in \"%s\" tag <%s> line number: %ld\n",
						out_count, chapters, doc->URL, tag_name[8],
						get_outfile_line_number(doc, out_count, tag_name[9]) );
				badstring = 1;
			}
			if (second > 98 ) {
				fprintf(stderr, "%d: Max chapter value of 98 exceeded "
						"(%s) in \"%s\" tag <%s> line number: %ld\n",
						out_count, chapters, doc->URL, tag_name[9],
						get_outfile_line_number(doc, out_count, tag_name[9]) );
				badstring = 1;
			}
		}
		out_options = xmlStrncat(out_options, (const xmlChar *) " -c ", 4);
		out_options = xmlStrncat(out_options, chapters, xmlStrlen(chapters));
	}

	// Verify audio tracks (comma separated list)
	if (audio[0] != '\0') {
		if (audio[0] == ',' || audio[xmlStrlen(audio)] == ',') {
			fprintf(stderr,
					"%d: Extra leading or trailing ',' (%s) in \"%s\" tag "
					"<%s> line number: %ld\n",
					out_count, audio, doc->URL, tag_name[10],
					get_outfile_line_number(doc, out_count, tag_name[10]) );
		} else {
			int bad_character = 0;
			int comma_count = 0;
			for (i = 0; i < xmlStrlen(audio); i++) {
				if (audio[i] == ',') {
					if ( audio[i-1] == ',' ) {
						fprintf(stderr,
								"%d: Extra comma(s) in \"%s\" tag <%s> line number: %ld\n",
								out_count, doc->URL, tag_name[10],
								get_outfile_line_number(doc, out_count, tag_name[10]) );
						badstring = 1;
						bad_character = 0;
					}
					comma_count++;
				}
				if (!isdigit(audio[i]) &&  audio[i] != ',') {
					fprintf(stderr,
							"%d: Invalid character(s) in \"%s\" tag <%s> line number: %ld\n",
							out_count, doc->URL, tag_name[10],
							get_outfile_line_number(doc, out_count, tag_name[10]) );
					bad_character = 1;
					badstring = 1;
				}
			}
			if (bad_character == 0 ){
				// verify each track number is between 1 and 99
				int *track_numbers = (int *) malloc((comma_count+1)*sizeof(int));
				char **track_strings = (char **) malloc((comma_count+1)*sizeof(char *));
				char *token;
				int track_out_of_range = 0;
				xmlChar *audio_copy = xmlStrdup(audio);
				token = track_strings[0] = strtok((char *) audio_copy, ",");
				track_numbers[0] = strtol(track_strings[0], NULL, 10);
				if (track_numbers[0] < 1 || track_numbers[0] > 99) {
					track_out_of_range = 1;
				}
				i = 1;
				while ( token != NULL ) {
					token = strtok(NULL, ",");
					if (token != NULL ) {
						track_strings[i] = token;
						track_numbers[i] = strtol((char *) track_strings[i], NULL, 10);
						if (track_numbers[0] < 1 || track_numbers[0] > 99) {
							track_out_of_range = 1;
							break;
						}
					}
					i++;
				}
				xmlFree(audio_copy);
				free(track_numbers);
				free(track_strings);
				if (track_out_of_range) {
					fprintf(stderr,
							"%d: Audio track number too large in "
							"\"%s\" tag <%s> line number: %ld\n",
							out_count, doc->URL, tag_name[10],
							get_outfile_line_number(doc, out_count, tag_name[10]) );
				} else {
					out_options = xmlStrncat(out_options, (const xmlChar *) " -a ", 4);
					out_options = xmlStrncat(out_options, audio, xmlStrlen(audio));
				}
			}
		}
	}

	// Verify subtitle tracks (comma separated list) (this is a copy of the audio track verify)
	if (subtitle[0] != '\0') {
		if (subtitle[0] == ',' || subtitle[xmlStrlen(subtitle)] == ',') {
			fprintf(stderr, "%d: Extra leading or trailing ',' (%s) in \"%s\" tag "
					"<%s> line number: %ld\n",
					out_count, subtitle, doc->URL, tag_name[11],
					get_outfile_line_number(doc, out_count, tag_name[11]) );
		} else {
			int bad_character = 0;
			int comma_count = 0;
			for (i = 0; i < xmlStrlen(subtitle); i++) {
				if (subtitle[i] == ',') {
					if ( subtitle[i-1] == ',' ) {
						fprintf(stderr, "%d: Extra comma(s) in \"%s\" tag "
								"<%s> line number: %ld\n",
								out_count, doc->URL, tag_name[11],
								get_outfile_line_number(doc, out_count, tag_name[11]) );
						badstring = 1;
						bad_character = 0;
					}
					comma_count++;
				}
				if (!isdigit(subtitle[i]) &&  subtitle[i] != ',') {
					fprintf(stderr, "%d: Invalid character(s) in \"%s\" tag "
							"<%s> line number: %ld\n",
							out_count, doc->URL, tag_name[11],
							get_outfile_line_number(doc, out_count, tag_name[11]) );
					bad_character = 1;
					badstring = 1;
				}
			}
			if (bad_character == 0 ){
				int *track_numbers = (int *) malloc((comma_count+1)*sizeof(int));
				char **track_strings = (char **) malloc((comma_count+1)*sizeof(char *));
				char *token;
				int track_out_of_range = 0;
				xmlChar *subtitle_copy = xmlStrdup(subtitle);
				token = track_strings[0] = strtok((char *) subtitle_copy, ",");
				track_numbers[0] = strtol(track_strings[0], NULL, 10);
				if (track_numbers[0] < 1 || track_numbers[0] > 99) {
					track_out_of_range = 1;
				}
				i = 1;
				while ( token != NULL ) {
					token = strtok(NULL, ",");
					if (token != NULL ) {
						track_strings[i] = token;
						track_numbers[i] = strtol((char *) track_strings[i], NULL, 10);
						if (track_numbers[i] < 1 || track_numbers[i] > 99) {
							track_out_of_range = 1;
							break;
						}
					}
					i++;
				}
				xmlFree(subtitle_copy);
				free(track_numbers);
				free(track_strings);
				if (track_out_of_range) {
					fprintf(stderr, "%d: Subtitle track number too large "
							"in \"%s\" tag <%s> line number: %ld\n",
							out_count, doc->URL, tag_name[11],
							get_outfile_line_number(doc, out_count, tag_name[11]) );
				} else {
					out_options = xmlStrncat(out_options, (const xmlChar *) " -s ", 4);
					out_options = xmlStrncat(out_options, subtitle, xmlStrlen(subtitle));
				}
			}
		}
	}
	if (badstring) {
		out_options[0] = '\0';
	}
	xmlFree(type);
	xmlFree(name);
	xmlFree(year);
	xmlFree(season);
	xmlFree(episode_number);
	xmlFree(specific_name);
	xmlFree(iso_filename);
	xmlFree(dvdtitle);
	xmlFree(crop);
	xmlFree(chapters);
	xmlFree(audio);
	xmlFree(subtitle);
	return out_options;
}

int validate_file_string(xmlChar * file_string)
{
	xmlChar bad_chars[6] = { '\\', '/', '`', '\"', '!'};

	int i;
	for (i = 0; i<5; i++) {
		const xmlChar *temp;
		if ((temp = xmlStrchr(file_string, bad_chars[i])) != NULL ){
			// return difference between first occurrence and string start
			return temp - file_string;
		}
	}
	return 0;
}

int call_handbrake(xmlChar *hb_command, int out_count, int overwrite)
{
	// separate filename and construct log filename
	const xmlChar *filename_start = xmlStrstr(hb_command, (const xmlChar *) "-o")+4;
	const xmlChar *filename_end = xmlStrstr(filename_start, (const xmlChar *) "\"");
	xmlChar *filename = xmlStrsub(filename_start, 0, filename_end-filename_start);
	xmlChar *log_filename = xmlStrdup(filename);
	log_filename = xmlStrcat(log_filename, (const xmlChar *) ".log");
	if ( access((char *) filename, F_OK ) == 0 ) {
		if ( access((char *) filename, W_OK ) == 0 ) {
			if (overwrite == 0) {
				char c;
				do{
					printf("File: \"%s\" already exists.\n", filename);
					printf("Run hbr with '-y' option to automatically overwrite.\n");
					printf("Do you want to overwrite? (y/n) ");
					scanf(" %c", &c);
					c = toupper(c);
				} while (c != 'N' && c != 'Y');
				if ( c == 'N' ) {
					xmlFree(filename);
					xmlFree(log_filename);
					return 1;
				}else {
					int r = hb_fork(hb_command, log_filename, filename, out_count);
					xmlFree(filename);
					xmlFree(log_filename);
					return r;
				}
			} else if (overwrite == 1) {
				// call handbrake (overwrite file)
				int r = hb_fork(hb_command, log_filename, filename, out_count);
				xmlFree(filename);
				xmlFree(log_filename);
				return r;
			}
		} else {
			fprintf(stderr, "%d: filename: \"%s\" is not writable\n", out_count, filename);
			xmlFree(filename);
			xmlFree(log_filename);
			return 1;
		}
	}
	int r = hb_fork(hb_command, log_filename, filename, out_count);
	xmlFree(filename);
	xmlFree(log_filename);
	return r;
}

int hb_fork(xmlChar *hb_command, xmlChar *log_filename, xmlChar *filename, int out_count)
{
	// check if current working directory is writeable
	char *cwd = getcwd(NULL, 0);
	if ( access((char *) cwd, W_OK|X_OK) == 0 ) {
		errno = 0;
		FILE *logfile = fopen((const char *)log_filename, "w");
		// test logfile was opened
		if (logfile != NULL) {
			int hb_err[2];
			pid_t hb_pid;
			// test pipe was opened
			if (pipe(hb_err)<0){
				fclose(logfile);
				xmlFree(filename);
				xmlFree(log_filename);
				return 1;
			} else {
				// fork to call HandBrakeCLI
				hb_pid = fork();
				if (hb_pid > 0) {
					//close write end of pipe on parent
					close(hb_err[1]);
				} else if (hb_pid == 0) {
					// replace stderr with our pipe for HandBrakeCLI
					close(hb_err[0]);
					close(2);
					dup2(hb_err[1], 2);
					close(hb_err[1]);
					//TODO fix so this errors before handbrake is called and outputs a log file
					execl("/bin/sh", "sh", "-c", hb_command, (char *) NULL);
					_exit(1);
				} else {
					perror("hb_fork(): Failed to fork");
					free(cwd);
					close(hb_err[1]);
					fclose(logfile);
					return 1;
				}
				// buffer output from handbrake and write to logfile
				char *buf = (char *) malloc(80*sizeof(char));
				int bytes;
				while ( (bytes = read(hb_err[0], buf, 80)) != 0) {
					fwrite(buf, bytes, 80, logfile);
				}
				free(buf);
			}
			close(hb_err[1]);
			fclose(logfile);
		} else {
			perror("hb_fork(): Failed to open logfile");
			free(cwd);
			return 1;
		}
	} else {
		fprintf(stderr, "%d: Current directory: \"%s\" is not writable\n", out_count, cwd);
		free(cwd);
		return 1;
	}
	free(cwd);
	return 0;
}
