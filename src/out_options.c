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
 * @param keyfile parsed key value file to read values from
 * @param out_count Indicates which outfile to generate options from
 *
 *
 * @return String of command line arguments
 */
gchar* out_options_string(GKeyFile* keyfile, int out_count)
{
	// output filename stuff
	struct tag type = {NULL, (gchar *) "type"};
	struct tag name = {NULL, (gchar *) "name"};
	struct tag year = {NULL, (gchar *) "year"};
	struct tag season = {NULL, (gchar *) "season"};
	struct tag episode_number = {NULL, (gchar *) "episode_number"};
	struct tag specific_name = {NULL, (gchar *) "specific_name"};
	// handbrake options
	struct tag iso_filename = {NULL, (gchar *) "iso_filename"};
	struct tag dvdtitle = {NULL, (gchar *) "dvdtitle"};
	struct tag crop_top = {NULL, (gchar *) "crop/top"};
	struct tag crop_bottom = {NULL, (gchar *) "crop/bottom"};
	struct tag crop_left = {NULL, (gchar *) "crop/left"};
	struct tag crop_right = {NULL, (gchar *) "crop/right"};
	struct tag chapters_start = {NULL, (gchar *) "chapters/start"};
	struct tag chapters_end = {NULL, (gchar *) "chapters/end"};
	struct tag audio = {NULL, (gchar *) "audio"};
	struct tag subtitle = {NULL, (gchar *) "subtitle"};

	struct tag *tag_array[] = {&type, &name, &year, &season,
		&episode_number, &specific_name, &iso_filename, &dvdtitle,
		&crop_top, &crop_bottom, &crop_left, &crop_right,
		&chapters_start, &chapters_end, &audio, &subtitle};

	int tag_array_size = sizeof(tag_array)/sizeof(struct tag*);
	int i;
	// Read in each tag's content
	for (i = 0; i<tag_array_size; i++) {
		tag_array[i]->content = get_outfile_child_content(keyfile,
				out_count, tag_array[i]->tag_name);
		// Test all tag contents for bad characters
		int ch;
		if ((ch = validate_file_string(tag_array[i]->content)) != 0) {
			fprintf(stderr, "%d: Invalid character '%c' in \"%s\" tag "
					"<%s> line number: %ld\n",
					out_count, tag_array[i]->content[ch], doc->URL, tag_array[i]->tag_name,
					get_outfile_line_number(keyfile, out_count, tag_array[i]->tag_name) );
			int p;
			for (p=0; p<=i; p++) {
				g_free(tag_array[p]->content);
			}
			return NULL;
		}
	}

	// full string of options
	gchar *out_options = g_strdup("\0");
	gchar *options[10];
	if ( strcmp(type.content, "series") == 0 ){
		options[0] = out_series_output(&name, &season, &episode_number, &specific_name, keyfile, out_count);
	} else if ( strcmp(type.content, "movie") == 0 ){
		options[0] = out_movie_output(&name, &year, &specific_name, keyfile, out_count);
	} else {
		fprintf(stderr, "%d: Invalid type in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, type.tag_name,
				get_outfile_line_number(keyfile, out_count, type.tag_name) );
		g_free(out_options);
		return NULL;
	}
	options[1] = out_input(&iso_filename, keyfile, out_count);
	options[2] = out_dvdtitle(&dvdtitle, keyfile, out_count);
	options[3] = out_crop(&crop_top, &crop_bottom, &crop_left, &crop_right);
	options[4] = out_chapters(&chapters_start, &chapters_end);
	options[5] = out_audio(&audio);
	options[6] = out_subtitle(&subtitle);
	
	// concatenate then free each option
	for (i = 0; i<7; i++) {
		// fail on error
		if (options[i] == NULL) {
			for (; i<7; i++) {
				if (options[i] != NULL) {
					g_free(options[i]);
				}
			}
			g_free(out_options);
			return NULL;
		}
		out_options = strcat(out_options, options[i]);
		g_free(options[i]);
	}

	for (i = 0; i<tag_array_size; i++) {
		g_free(tag_array[i]->content);
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
int validate_file_string(gchar * file_string)
{
	gchar bad_chars[] = { '\\', '/', '`', '\"', '!'};

	int i;
	for (i = 0; i<(sizeof(bad_chars)/sizeof(gchar)); i++) {
		const gchar *temp;
		if ((temp = strchr(file_string, bad_chars[i])) != NULL ){
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
 * @param keyfile parsed key value pair
 * @param infile path to the input file associated with keyfile
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for output filename
 */
gchar* out_series_output(struct tag *name, struct tag *season,
		struct tag *episode_number, struct tag *specific_name,
		GKeyFile* keyfile, gchar *infile, int out_count)
{
	// series name = -o <name> - s<season>e<episode_number> - <specific_name>
	gchar *arg = g_strdup(" -o \"");
	gchar *cwd = (gchar *) getcwd(NULL, 0);
	arg = strcat(arg, cwd);
	g_free(cwd);
	arg = strcat(arg, "/");

	// Verify <name> is non-empty
	int name_len = strlen(name->content);
	if ( name_len == 0 ) {
		fprintf(stderr, "%d: Empty name in \"%s\" tag <%s> line number: %ld\n",
				out_count, infile, name->tag_name,
				get_outfile_line_number(keyfile, out_count, name->tag_name) );
		return NULL;
	}
	arg = strncat(arg, name->content, name_len);
	arg = strncat(arg, " - ", 3);
	// add season if episode also exists
	// Season is a one or two digit number, left padded with a 0 if one digit
	if (season->content[0] != '\0' && episode_number->content[0] != '\0') {
		if (strlen(season->content) <= 2 && isdigit(season->content[0])
				&& (isdigit(season->content[1]) || season->content[1] == '\0')) {
			arg = strncat(arg, "s", 1);
			if (strlen(season->content) == 1){
				arg = strncat(arg, "0", 1);
			}
			arg = strncat(arg, season->content, strlen(season->content));
		} else {
			fprintf(stderr, "%d: Invalid season in \"%s\" tag <%s> line number: %ld\n",
					out_count, infile, season->tag_name,
					get_outfile_line_number(keyfile, out_count, season->tag_name) );
			return NULL;
		}
	}
	//Verify episode number is at most 3 digits, left pad with 0s
	int i;
	for (i=0; i<4; i++) {
		if ( isdigit(episode_number->content[i]) && i < 3) {
			continue;
		} else if ( episode_number->content[i] == '\0' && i > 0) {
			arg = strncat(arg, "e", 1);
			if (i == 1) {
				arg = strncat(arg, "00", 2);
			}
			if (i == 2) {
				arg = strncat(arg, "0", 1);
			}
			arg = strncat(arg, episode_number->content, strlen(episode_number->content));
			arg = strncat(arg, " - ", 3);
			break;
		} else {
			fprintf(stderr, "%d: Invalid episode number in \"%s\" tag "
					"<%s> line number: %ld\n",
					out_count, infile, episode_number->tag_name,
					get_outfile_line_number(keyfile, out_count, episode_number->tag_name) );
			return NULL;
		}
	}
	if (specific_name->content[0] != '\0') {
		arg = strncat(arg, specific_name->content, strlen(specific_name->content));
	}
	if (strlen(arg) > 249){
		gchar * temp_arg = g_strndup(arg, 249);
		temp_arg[249] = '\0';
		g_free(arg);
		arg = temp_arg;
	}
	arg = strncat(arg, ".", 1);

	gchar *format = get_format(doc);
	arg = strncat(arg, format, 3);
	g_free(format);

	arg = strncat(arg, "\"", 1);
	return arg;
}

/**
 * @brief Builds the path and filename option for movie outfiles (-o)
 *
 * @param name General name
 * @param year Release year
 * @param specific_name Sub-title or revision
 * @param keyfile parsed key value pair file
 * @param infile path to the input file associated with keyfile
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for output filename
 */
gchar* out_movie_output(struct tag *name, struct tag *year,
		struct tag *specific_name, GKeyFile* keyfile, gchar* infile,
        int out_count)
{
	gchar *arg = g_strdup(" -o \"");
	gchar *cwd = (gchar *) getcwd(NULL, 0);
	arg = strcat(arg, cwd);
	g_free(cwd);
	arg = strcat(arg, "/");
	
	// Verify <name> is non-empty
	int name_len = strlen(name->content);
	if ( name_len == 0 ) {
		fprintf(stderr, "%d: Empty name in \"%s\" tag <%s> line number: %ld\n",
				out_count, infile, name->tag_name,
				get_outfile_line_number(keyfile, out_count, name->tag_name) );
		return NULL;
	}
	arg = strncat(arg, name->content, name_len);
	int i;
	for (i=0; i<5; i++) {
		if ( isdigit(year->content[i]) && i < 4) {
			continue;
		} else if ( year->content[i] == '\0' && i == 4) {
			arg = strncat(arg, " (", 2);
			arg = strncat(arg, year->content, strlen(year->content));
			arg = strncat(arg, ")", 1);
		} else {
			fprintf(stderr,
					"%d: Invalid year in \"%s\" tag <%s> line number: %ld\n",
					out_count, infile, year->tag_name,
					get_outfile_line_number(keyfile, out_count, year->tag_name) );
			return NULL;
		}
	}
	if (specific_name->content[0] != '\0') {
		arg = strncat(arg, " - ", 3);
		arg = strncat(arg, specific_name->content, strlen(specific_name->content));
	}
	if (strlen(arg) > 249){
		gchar * temp_arg = g_strndup(arg, 249);
		temp_arg[249] = '\0';
		g_free(arg);
		arg = temp_arg;
	}
	arg = strncat(arg, ".", 1);
	
	gchar *format = get_format(doc);
	arg = strncat(arg, format, 3);
	g_free(format);
	
	arg = strncat(arg, "\"", 1);
	return arg;
}

/**
 * @brief Build the input filename option for an outfile (-i)
 *
 * @param iso_filename ISO filename without path
 * @param keyfile parsed key value pair file
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for input filename
 */
gchar* out_input(struct tag *iso_filename, GKeyFile* keyfile, int out_count)
{
	gchar *input_basedir = get_input_basedir(doc);
	gchar *arg = g_strndup(" -i \"", 5);
	int ib_length = strlen(input_basedir);

	// check input filename exists or error
	gchar* full_path = g_strdup(input_basedir);
	if (input_basedir[ib_length-1] != '/') {
		full_path = strncat(full_path, "/", 1);
	}
	g_free(input_basedir);

	full_path = strncat(full_path, iso_filename->content, strlen(iso_filename->content) );
	errno = 0;
	if ( access((char *) full_path, R_OK) == -1 ) {
		fprintf(stderr, "%d: Invalid input file: \"%s\" tag <%s> line number: %ld\n",
				out_count, full_path, iso_filename->tag_name,
				get_outfile_line_number(keyfile, out_count, iso_filename->tag_name) );
		perror (" arg_string(): Input file was inaccessible");
		return NULL;
	}
	arg = strncat(arg, full_path, strlen(full_path));
	g_free(full_path);
	arg = strncat(arg, "\"", 1);
	return arg;
}

/**
 * @brief Build the dvd title option for an outfile (-t)
 *
 * @param dvdtitle DVD Title number
 * @param keyfile parsed key value pair file
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for dvd title
 */
gchar* out_dvdtitle(struct tag *dvdtitle, GKeyFile* keyfile, gchar* infile,
        int out_count)
{
	gchar *arg;
	if (strlen(dvdtitle->content) <= 2 && isdigit(dvdtitle->content[0])
			&& (isdigit(dvdtitle->content[1]) || dvdtitle->content[1] == '\0')) {
		arg = g_strndup(" -t ", 4);
		arg = strncat(arg, dvdtitle->content, strlen(dvdtitle->content));
	} else {
		fprintf(stderr, "%d: Invalid dvdtitle in \"%s\" tag <%s> line number: %ld\n",
				out_count, infile, dvdtitle->tag_name,
				get_outfile_line_number(keyfile, out_count, dvdtitle->tag_name) );
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
gchar* out_crop(struct tag *crop_top, struct tag *crop_bottom,
		struct tag *crop_left, struct tag *crop_right)
{

	gchar *arg = g_strndup(" --crop ", 8);
	arg = strncat(arg, crop_top->content, strlen(crop_top->content));
	arg = strncat(arg, ":", 2);
	arg = strncat(arg, crop_bottom->content, strlen(crop_bottom->content));
	arg = strncat(arg, ":", 2);
	arg = strncat(arg, crop_left->content, strlen(crop_left->content));
	arg = strncat(arg, ":", 2);
	arg = strncat(arg, crop_right->content, strlen(crop_right->content));
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
gchar* out_chapters(struct tag *chapters_start, struct tag *chapters_end)
{
	gchar *arg = g_strndup(" -c ", 4);
	arg = strncat(arg, chapters_start->content,
			strlen(chapters_start->content));
	arg = strncat(arg, "-", 1);
	arg = strncat(arg, chapters_end->content,
			strlen(chapters_end->content));
	return arg;
}

/**
 * @brief Build the audio tracks option for an outfile (-a)
 *
 * @param audio
 *
 * @return command line option for audio tracks
 */
gchar* out_audio(struct tag *audio)
{
	if (audio->content[0] == '\0') {
		return g_strdup("");
	}
	gchar *comma_string = malloc(strlen(audio->content)*sizeof(gchar));
	int i;
	int cs_last_char = 0;
	bool digit = false;
	for (i=0; i<strlen(audio->content); i++) {
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
	gchar *arg = g_strndup(" -a ", 4);
	arg = strncat(arg, comma_string, strlen(comma_string));
	return arg;
}


/**
 * @brief Build the subtitle tracks option for an outfile (-s)
 *
 * @param subtitle
 *
 * @return command line option for subtitle tracks
 */
gchar* out_subtitle(struct tag *subtitle)
{
	if (subtitle->content[0] == '\0') {
		return g_strdup("");
	}
	gchar *comma_string = malloc(strlen(subtitle->content)*sizeof(gchar));
	int i;
	int cs_last_char = 0;
	bool digit = false;
	for (i=0; i<strlen(subtitle->content); i++) {
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

	gchar *arg= g_strndup(" -s ", 4);
	arg = strncat(arg, comma_string, strlen(comma_string));
	return arg;
}
