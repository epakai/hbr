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

#include "out_options.h"
#include <stdbool.h>
#include <string.h>

/**
 * @brief Build HandBrakeCLI arguments specific to each outfile
 *
 * @param doc XML document to read values from
 * @param out_count Indicates which outfile to generate options from
 *
 *
 * @return String of command line arguments
 */
xmlChar* out_options_string(xmlDocPtr doc, int out_count)
{
	// output filename stuff
	struct tag type = {NULL, (xmlChar *) "type"};
	struct tag name = {NULL, (xmlChar *) "name"};
	struct tag year = {NULL, (xmlChar *) "year"};
	struct tag season = {NULL, (xmlChar *) "season"};
	struct tag episode_number = {NULL, (xmlChar *) "episode_number"};
	struct tag specific_name = {NULL, (xmlChar *) "specific_name"};
	// handbrake options
	struct tag iso_filename = {NULL, (xmlChar *) "iso_filename"};
	struct tag dvdtitle = {NULL, (xmlChar *) "dvdtitle"};
	struct tag crop_top = {NULL, (xmlChar *) "crop/top"};
	struct tag crop_bottom = {NULL, (xmlChar *) "crop/bottom"};
	struct tag crop_left = {NULL, (xmlChar *) "crop/left"};
	struct tag crop_right = {NULL, (xmlChar *) "crop/right"};
	struct tag chapters_start = {NULL, (xmlChar *) "chapters/start"};
	struct tag chapters_end = {NULL, (xmlChar *) "chapters/end"};
	struct tag audio = {NULL, (xmlChar *) "audio"};
	struct tag subtitle = {NULL, (xmlChar *) "subtitle"};

	struct tag *tag_array[] = {&type, &name, &year, &season,
		&episode_number, &specific_name, &iso_filename, &dvdtitle,
		&crop_top, &crop_bottom, &crop_left, &crop_right,
		&chapters_start, &chapters_end, &audio, &subtitle};

	int tag_array_size = sizeof(tag_array)/sizeof(struct tag*);
	int i;
	// Read in each tag's content
	for (i = 0; i<tag_array_size; i++) {
		tag_array[i]->content = get_outfile_child_content(doc,
				out_count, tag_array[i]->tag_name);
		// Test all tag contents for bad characters
		int ch;
		if ((ch = validate_file_string(tag_array[i]->content)) != 0) {
			fprintf(stderr, "%d: Invalid character '%c' in \"%s\" tag "
					"<%s> line number: %ld\n",
					out_count, tag_array[i]->content[ch], doc->URL, tag_array[i]->tag_name,
					get_outfile_line_number(doc, out_count, tag_array[i]->tag_name) );
			int p;
			for (p=0; p<=i; p++) {
				xmlFree(tag_array[p]->content);
			}
			return NULL;
		}
	}

	// full string of options
	xmlChar *out_options = xmlCharStrdup("\0");
	xmlChar *options[10];
	if ( xmlStrcmp(type.content, BAD_CAST "series") == 0 ){
		options[0] = out_series_output(&name, &season, &episode_number, &specific_name, doc, out_count);
	} else if ( xmlStrcmp(type.content, BAD_CAST "movie") == 0 ){
		options[0] = out_movie_output(&name, &year, &specific_name, doc, out_count);
	} else {
		fprintf(stderr, "%d: Invalid type in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, type.tag_name,
				get_outfile_line_number(doc, out_count, type.tag_name) );
		xmlFree(out_options);
		return NULL;
	}
	options[1] = out_input(&iso_filename, doc, out_count);
	options[2] = out_dvdtitle(&dvdtitle, doc, out_count);
	options[3] = out_crop(&crop_top, &crop_bottom, &crop_left, &crop_right);
	options[4] = out_chapters(&chapters_start, &chapters_end);
	options[5] = out_audio(&audio);
	options[6] = out_subtitle(&subtitle);
	
	// concatenate then free each option
	for (i = 0; i<7; i++) {
		// fail on error
		if (options[i] == NULL) {
			for (i=0; i<7; i++) {
				if (options[i] != NULL) {
					xmlFree(options[i]);
				}
			}
			return NULL;
		}
		out_options = xmlStrcat(out_options, options[i]);
		xmlFree(options[i]);
	}

	for (i = 0; i<tag_array_size; i++) {
		xmlFree(tag_array[i]->content);
	}
	return out_options;
}

/**
 * @brief Test if file_string contains certain characters
 *
 * @param file_string path to be tested
 *
 * @return 0 if valid, offset of bad character if invalid
 */
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

/**
 * @brief Builds the path and filename option for series outfiles (-o)
 *
 * @param name General name
 * @param season Season number
 * @param episode_number Episode number
 * @param specific_name Specific or episode name
 * @param doc xml document
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for output filename
 */
xmlChar* out_series_output(struct tag *name, struct tag *season,
		struct tag *episode_number, struct tag *specific_name,
		xmlDocPtr doc, int out_count)
{
	// series name = -o <name> - s<season>e<episode_number> - <specific_name>
	xmlChar *arg = xmlCharStrdup(" -o \"");
	xmlChar *cwd = (xmlChar *) getcwd(NULL, 0);
	arg = xmlStrcat(arg, cwd);
	xmlFree(cwd);
	arg = xmlStrcat(arg, BAD_CAST "/");

	// Verify <name> is non-empty
	int name_len = xmlStrlen(name->content);
	if ( name_len == 0 ) {
		fprintf(stderr, "%d: Empty name in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, name->tag_name,
				get_outfile_line_number(doc, out_count, name->tag_name) );
		return NULL;
	}
	arg = xmlStrncat(arg, name->content, name_len);
	arg = xmlStrncat(arg, BAD_CAST " - ", 3);
	// add season if episode also exists
	// Season is a one or two digit number, left padded with a 0 if one digit
	if (season->content[0] != '\0' && episode_number->content[0] != '\0') {
		if (xmlStrlen(season->content) <= 2 && isdigit(season->content[0])
				&& (isdigit(season->content[1]) || season->content[1] == '\0')) {
			arg = xmlStrncat(arg, BAD_CAST "s", 1);
			if (xmlStrlen(season->content) == 1){
				arg = xmlStrncat(arg, BAD_CAST "0", 1);
			}
			arg = xmlStrncat(arg, season->content, xmlStrlen(season->content));
		} else {
			fprintf(stderr, "%d: Invalid season in \"%s\" tag <%s> line number: %ld\n",
					out_count, doc->URL, season->tag_name,
					get_outfile_line_number(doc, out_count, season->tag_name) );
			return NULL;
		}
	}
	//Verify episode number is at most 3 digits, left pad with 0s
	int i;
	for (i=0; i<4; i++) {
		if ( isdigit(episode_number->content[i]) && i < 3) {
			continue;
		} else if ( episode_number->content[i] == '\0' && i > 0) {
			arg = xmlStrncat(arg, BAD_CAST "e", 1);
			if (i == 1) {
				arg = xmlStrncat(arg, BAD_CAST "00", 2);
			}
			if (i == 2) {
				arg = xmlStrncat(arg, BAD_CAST "0", 1);
			}
			arg = xmlStrncat(arg, episode_number->content, xmlStrlen(episode_number->content));
			arg = xmlStrncat(arg, BAD_CAST " - ", 3);
			break;
		} else {
			fprintf(stderr, "%d: Invalid episode number in \"%s\" tag "
					"<%s> line number: %ld\n",
					out_count, doc->URL, episode_number->tag_name,
					get_outfile_line_number(doc, out_count, episode_number->tag_name) );
			return NULL;
		}
	}
	if (specific_name->content[0] != '\0') {
		arg = xmlStrncat(arg, specific_name->content, xmlStrlen(specific_name->content));
	}
	if (xmlStrlen(arg) > 249){
		xmlChar * temp_arg = xmlStrndup(arg, 249);
		temp_arg[249] = '\0';
		xmlFree(arg);
		arg = temp_arg;
	}
	arg = xmlStrncat(arg, BAD_CAST ".", 1);

	xmlChar *format = get_format(doc);
	arg = xmlStrncat(arg, BAD_CAST format, 3);
	xmlFree(format);

	arg = xmlStrncat(arg, BAD_CAST "\"", 1);
	return arg;
}

/**
 * @brief Builds the path and filename option for movie outfiles (-o)
 *
 * @param name General name
 * @param year Release year
 * @param specific_name Sub-title or revision
 * @param doc xml document
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for output filename
 */
xmlChar* out_movie_output(struct tag *name, struct tag *year,
		struct tag *specific_name, xmlDocPtr doc, int out_count)
{
	xmlChar *arg = xmlCharStrdup(" -o \"");
	xmlChar *cwd = (xmlChar *) getcwd(NULL, 0);
	arg = xmlStrcat(arg, cwd);
	xmlFree(cwd);
	arg = xmlStrcat(arg, BAD_CAST "/");
	
	// Verify <name> is non-empty
	int name_len = xmlStrlen(name->content);
	if ( name_len == 0 ) {
		fprintf(stderr, "%d: Empty name in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, name->tag_name,
				get_outfile_line_number(doc, out_count, name->tag_name) );
		return NULL;
	}
	arg = xmlStrncat(arg, name->content, name_len);
	int i;
	for (i=0; i<5; i++) {
		if ( isdigit(year->content[i]) && i < 4) {
			continue;
		} else if ( year->content[i] == '\0' && i == 4) {
			arg = xmlStrncat(arg, BAD_CAST " (", 2);
			arg = xmlStrncat(arg, year->content, xmlStrlen(year->content));
			arg = xmlStrncat(arg, BAD_CAST ")", 1);
		} else {
			fprintf(stderr,
					"%d: Invalid year in \"%s\" tag <%s> line number: %ld\n",
					out_count, doc->URL, year->tag_name,
					get_outfile_line_number(doc, out_count, year->tag_name) );
			return NULL;
		}
	}
	if (specific_name->content[0] != '\0') {
		arg = xmlStrncat(arg, BAD_CAST " - ", 3);
		arg = xmlStrncat(arg, specific_name->content, xmlStrlen(specific_name->content));
	}
	if (xmlStrlen(arg) > 249){
		xmlChar * temp_arg = xmlStrndup(arg, 249);
		temp_arg[249] = '\0';
		xmlFree(arg);
		arg = temp_arg;
	}
	arg = xmlStrncat(arg, BAD_CAST ".", 1);
	
	xmlChar *format = get_format(doc);
	arg = xmlStrncat(arg, BAD_CAST format, 3);
	xmlFree(format);
	
	arg = xmlStrncat(arg, BAD_CAST "\"", 1);
	return arg;
}

/**
 * @brief Build the input filename option for an outfile (-i)
 *
 * @param iso_filename ISO filename without path
 * @param doc xml document
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for input filename
 */
xmlChar* out_input(struct tag *iso_filename, xmlDocPtr doc, int out_count)
{
	xmlChar *input_basedir = get_input_basedir(doc);
	xmlChar *arg = xmlCharStrndup(" -i \"", 5);
	int ib_length = xmlStrlen(input_basedir);

	// check input filename exists or error
	xmlChar* full_path = xmlStrdup(input_basedir);
	if (input_basedir[ib_length-1] != '/') {
		full_path = xmlStrncat(full_path, BAD_CAST "/", 1);
	}
	xmlFree(input_basedir);

	full_path = xmlStrncat(full_path, iso_filename->content, xmlStrlen(iso_filename->content) );
	errno = 0;
	if ( access((char *) full_path, R_OK) == -1 ) {
		fprintf(stderr, "%d: Invalid input file: \"%s\" tag <%s> line number: %ld\n",
				out_count, full_path, iso_filename->tag_name,
				get_outfile_line_number(doc, out_count, iso_filename->tag_name) );
		perror (" arg_string(): Input file was inaccessible");
		return NULL;
	}
	arg = xmlStrncat(arg, full_path, xmlStrlen(full_path));
	xmlFree(full_path);
	arg = xmlStrncat(arg, BAD_CAST "\"", 1);
	return arg;
}

/**
 * @brief Build the dvd title option for an outfile (-t)
 *
 * @param dvdtitle DVD Title number
 * @param doc xml document
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for dvd title
 */
xmlChar* out_dvdtitle(struct tag *dvdtitle, xmlDocPtr doc, int out_count)
{
	xmlChar *arg;
	if (xmlStrlen(dvdtitle->content) <= 2 && isdigit(dvdtitle->content[0])
			&& (isdigit(dvdtitle->content[1]) || dvdtitle->content[1] == '\0')) {
		arg = xmlCharStrndup(" -t ", 4);
		arg = xmlStrncat(arg, dvdtitle->content, xmlStrlen(dvdtitle->content));
	} else {
		fprintf(stderr, "%d: Invalid dvdtitle in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, dvdtitle->tag_name,
				get_outfile_line_number(doc, out_count, dvdtitle->tag_name) );
		return NULL;
	}
	return arg;
}

/**
 * @brief Build the crop option for an outfile (--crop)
 *
 * @param crop_top dimensions to be cropped
 * @param crop_bottom dimensions to be cropped
 * @param crop_left dimensions to be cropped
 * @param crop_right dimensions to be cropped
 *
 * @return command line option for crop
 */
xmlChar* out_crop(struct tag *crop_top, struct tag *crop_bottom,
		struct tag *crop_left, struct tag *crop_right)
{

	xmlChar *arg = xmlCharStrndup(" --crop ", 8);
	arg = xmlStrncat(arg, crop_top->content, xmlStrlen(crop_top->content));
	arg = xmlStrncat(arg, BAD_CAST ":", 2);
	arg = xmlStrncat(arg, crop_bottom->content, xmlStrlen(crop_bottom->content));
	arg = xmlStrncat(arg, BAD_CAST ":", 2);
	arg = xmlStrncat(arg, crop_left->content, xmlStrlen(crop_left->content));
	arg = xmlStrncat(arg, BAD_CAST ":", 2);
	arg = xmlStrncat(arg, crop_right->content, xmlStrlen(crop_right->content));
	return arg;
}

/**
 * @brief Build the chapters option for an outfile (-c)
 *
 * @param chapters_start chapters start tag
 * @param chapters_end chapters end tag
 *
 * @return command line option for chapters
 */
xmlChar* out_chapters(struct tag *chapters_start, struct tag *chapters_end)
{
	xmlChar *arg = xmlCharStrndup(" -c ", 4);
	arg = xmlStrncat(arg, chapters_start->content,
			xmlStrlen(chapters_start->content));
	arg = xmlStrncat(arg, BAD_CAST "-", 1);
	arg = xmlStrncat(arg, chapters_end->content,
			xmlStrlen(chapters_end->content));
	return arg;
}

/**
 * @brief Build the audio tracks option for an outfile (-a)
 *
 * @param audio
 *
 * @return command line option for audio tracks
 */
xmlChar* out_audio(struct tag *audio)
{
	if (audio->content[0] == '\0') {
		return xmlCharStrdup("");
	}
	xmlChar *comma_string = malloc(xmlStrlen(audio->content)*sizeof(xmlChar));
	int i;
	int cs_last_char = 0;
	bool digit = false;
	for (i=0; i<xmlStrlen(audio->content); i++) {
		if ( isdigit(audio->content[i]) ) {
			digit = true;
			comma_string[cs_last_char] = audio->content[i];
			cs_last_char++;
		}
		if (audio->content[i] == '\t' || audio->content[i] == ' ') {
			if (digit && isdigit(audio->content[i+1])) {
				comma_string[cs_last_char] = ',';
				cs_last_char++;
			}
		}
	}
	comma_string[cs_last_char] = '\0';
	xmlChar *arg = xmlCharStrndup(" -a ", 4);
	arg = xmlStrncat(arg, comma_string, xmlStrlen(comma_string));
	return arg;
}


/**
 * @brief Build the subtitle tracks option for an outfile (-s)
 *
 * @param subtitle
 *
 * @return command line option for subtitle tracks
 */
xmlChar* out_subtitle(struct tag *subtitle)
{
	if (subtitle->content[0] == '\0') {
		return xmlCharStrdup("");
	}
	xmlChar *comma_string = malloc(xmlStrlen(subtitle->content)*sizeof(xmlChar));
	int i;
	int cs_last_char = 0;
	bool digit = false;
	for (i=0; i<xmlStrlen(subtitle->content); i++) {
		if ( isdigit(subtitle->content[i]) ) {
			digit = true;
			comma_string[cs_last_char] = subtitle->content[i];
			cs_last_char++;
		}
		if (subtitle->content[i] == '\t' || subtitle->content[i] == ' ') {
			if (digit && isdigit(subtitle->content[i+1])) {
				comma_string[cs_last_char] = ',';
				cs_last_char++;
			}
		}
	}
	comma_string[cs_last_char] = '\0';

	xmlChar *arg= xmlCharStrndup(" -s ", 4);
	arg = xmlStrncat(arg, comma_string, xmlStrlen(comma_string));
	return arg;
}
