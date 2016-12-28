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
	struct tag crop = {NULL, (xmlChar *) "crop"};
	struct tag chapters = {NULL, (xmlChar *) "chapters"};
	struct tag audio = {NULL, (xmlChar *) "audio"};
	struct tag subtitle = {NULL, (xmlChar *) "subtitle"};

	struct tag *tag_array[] = {&type, &name, &year, &season,
		&episode_number, &specific_name, &iso_filename, &dvdtitle,
		&crop, &chapters, &audio, &subtitle};

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
	xmlChar *options[7];
	if ( xmlStrcmp(type.content, (const xmlChar *) "series") == 0 ){
		options[0] = out_series_output(&name, &season, &episode_number, &specific_name, doc, out_count);
	} else if ( xmlStrcmp(type.content, (const xmlChar *) "movie") == 0 ){
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
	options[3] = out_crop(&crop, doc, out_count);
	options[4] = out_chapters(&chapters, doc, out_count);
	options[5] = out_audio(&audio, doc, out_count);
	options[6] = out_subtitle(&subtitle, doc, out_count);
	
	// stop if there is no input or output filename (all others have defaults)
	if (options[0] == NULL || options[1] == NULL ) {
		for (i=2; i<7; i++) {
			if (options[i] != NULL) {
				xmlFree(options[i]);
			}
		}
		return NULL;
	}

	for (i = 0; i<7; i++) {
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
	arg = xmlStrcat(arg, (const xmlChar *) "/");

	// Verify <name> is non-empty
	int name_len = xmlStrlen(name->content);
	if ( name_len == 0 ) {
		fprintf(stderr, "%d: Empty name in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, name->tag_name,
				get_outfile_line_number(doc, out_count, name->tag_name) );
		return NULL;
	}
	arg = xmlStrncat(arg, name->content, name_len);
	arg = xmlStrncat(arg, (const xmlChar *) " - ", 3);
	// add season if episode also exists
	// Season is a one or two digit number, left padded with a 0 if one digit
	if (season->content[0] != '\0' && episode_number->content[0] != '\0') {
		if (xmlStrlen(season->content) <= 2 && isdigit(season->content[0])
				&& (isdigit(season->content[1]) || season->content[1] == '\0')) {
			arg = xmlStrncat(arg, (const xmlChar *) "s", 1);
			if (xmlStrlen(season->content) == 1){
				arg = xmlStrncat(arg, (const xmlChar *) "0", 1);
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
			arg = xmlStrncat(arg, (const xmlChar *) "e", 1);
			if (i == 1) {
				arg = xmlStrncat(arg, (const xmlChar *) "00", 2);
			}
			if (i == 2) {
				arg = xmlStrncat(arg, (const xmlChar *) "0", 1);
			}
			arg = xmlStrncat(arg, episode_number->content, xmlStrlen(episode_number->content));
			arg = xmlStrncat(arg, (const xmlChar *) " - ", 3);
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
	arg = xmlStrncat(arg, (const xmlChar *) ".", 1);

	xmlChar *format = get_format(doc);
	arg = xmlStrncat(arg, (const xmlChar *) format, 3);
	xmlFree(format);

	arg = xmlStrncat(arg, (const xmlChar *) "\"", 1);
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
	arg = xmlStrcat(arg, (const xmlChar *) "/");
	
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
			arg = xmlStrncat(arg, (const xmlChar *) " (", 2);
			arg = xmlStrncat(arg, year->content, xmlStrlen(year->content));
			arg = xmlStrncat(arg, (const xmlChar *) ")", 1);
		} else {
			fprintf(stderr,
					"%d: Invalid year in \"%s\" tag <%s> line number: %ld\n",
					out_count, doc->URL, year->tag_name,
					get_outfile_line_number(doc, out_count, year->tag_name) );
			return NULL;
		}
	}
	if (specific_name->content[0] != '\0') {
		arg = xmlStrncat(arg, (const xmlChar *) " - ", 3);
		arg = xmlStrncat(arg, specific_name->content, xmlStrlen(specific_name->content));
	}
	if (xmlStrlen(arg) > 249){
		xmlChar * temp_arg = xmlStrndup(arg, 249);
		temp_arg[249] = '\0';
		xmlFree(arg);
		arg = temp_arg;
	}
	arg = xmlStrncat(arg, (const xmlChar *) ".", 1);
	
	xmlChar *format = get_format(doc);
	arg = xmlStrncat(arg, (const xmlChar *) format, 3);
	xmlFree(format);
	
	arg = xmlStrncat(arg, (const xmlChar *) "\"", 1);
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
		full_path = xmlStrncat(full_path, (const xmlChar *) "/", 1);
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
	arg = xmlStrncat(arg, (const xmlChar *) "\"", 1);
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
 * @param crop dimensions to be cropped
 * @param doc xml document
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for crop
 */
xmlChar* out_crop(struct tag *crop, xmlDocPtr doc, int out_count)
{
	if (crop == '\0') {
		return NULL;
	}
	xmlChar *token_string = xmlStrdup(crop->content);
	char *crop_str[5];
	// break crop into components
	crop_str[0] = strtok((char *) token_string, ":");
	crop_str[1] = strtok(NULL, ":");
	crop_str[2] = strtok(NULL, ":");
	crop_str[3] = strtok(NULL, "\0");
	// verify each crop is 4 digit max (video isn't that big yet)
	if ((strnlen(crop_str[0], 5) > 4) || (strnlen(crop_str[1], 5) > 4)
			|| (strnlen(crop_str[2], 5) > 4) || (strnlen(crop_str[3], 5) > 4)){
		fprintf(stderr, "%d: Invalid crop (values too large) "
				"'%s:%s:%s:%s' in \"%s\" line number: %ld\n",
				out_count, crop_str[0], crop_str[1], crop_str[2], crop_str[3],
				doc->URL, get_outfile_line_number(doc, out_count, crop->tag_name) );
		return NULL;
	}
	int i;
	// verify all characters are digits
	for (i = 0; i<4; i++){
		int j = 0;
		while (crop_str[i][j] != '\0'){
			if (!isdigit(crop_str[i][j])){
				fprintf(stderr, "%d: Invalid crop (non-digits) "
						"'%s:%s:%s:%s' in \"%s\" line number: %ld\n",
						out_count, crop_str[0], crop_str[1], crop_str[2], crop_str[3],
						doc->URL, get_outfile_line_number(doc, out_count, crop->tag_name) );
				return NULL;
			}
			j++;
		}
	}
	xmlFree(token_string);
	xmlChar *arg = xmlCharStrndup(" --crop ", 8);
	//re-grab the property because strtok destroyed it
	arg = xmlStrncat(arg, crop->content, xmlStrlen(crop->content));
	return arg;
}

/**
 * @brief Build the chapters option for an outfile (-c)
 *
 * @param chapters
 * @param doc xml document
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for chapters
 */
xmlChar* out_chapters(struct tag *chapters, xmlDocPtr doc, int out_count)
{
	int dash_count = 0;
	int non_digit_dash_found = 0;
	// Check for digits and count '-' characters
	int i;
	for (i = 0; i< xmlStrlen(chapters->content); i++) {
		if (!isdigit(chapters->content[i]) && chapters->content[i] != '-' ) {
			non_digit_dash_found = 1;
		}
		if (chapters->content[i] == '-') {
			dash_count++;
		}
	}
	// Ensure only digits and '-'
	if (non_digit_dash_found) {
		fprintf(stderr, "%d: Invalid character(s) in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, chapters->tag_name,
				get_outfile_line_number(doc, out_count, chapters->tag_name) );
		return NULL;
		// Ensure single '-' and '-' is not first or last character
	} else if (dash_count > 1 || chapters->content[0] == '-' ||
			chapters->content[xmlStrlen(chapters->content)] == '-') {
		fprintf(stderr, "%d: Bad usage of '-' (%s) in \"%s\" tag <%s> line number: %ld\n",
				out_count, chapters->content, doc->URL, chapters->tag_name,
				get_outfile_line_number(doc, out_count, chapters->tag_name) );
		return NULL;
	} else {
		// convert to longs for testing
		int first = strtol((const char *) chapters->content, NULL, 10);
		if (first > 98 || first < 1) {
			fprintf(stderr, "%d: Chapter value outside range (1-98) "
					"(%s) in \"%s\" tag <%s> line number: %ld\n",
					out_count, chapters->content, doc->URL, chapters->tag_name,
					get_outfile_line_number(doc, out_count, chapters->tag_name) );
			return NULL;
		}
		// Deal with second digit if a single '-' was found
		if (dash_count == 1) {
			int second;
			//substring starts with -, increment pointer once
			const xmlChar *temp = xmlStrstr(chapters->content, (const xmlChar *) "-") + 1;
			second = strtol((const char *) temp, NULL, 10);
			if (first > second) {
				printf("first: %d second: %d\n", first, second);
				fprintf(stderr, "%d: Initial chapter is after final chapter "
						"(%s) in \"%s\" tag <%s> line number: %ld\n",
						out_count, chapters->content, doc->URL, chapters->tag_name,
						get_outfile_line_number(doc, out_count, chapters->tag_name) );
				return NULL;
			}
			if (second > 98 ) {
				fprintf(stderr, "%d: Max chapter value of 98 exceeded "
						"(%s) in \"%s\" tag <%s> line number: %ld\n",
						out_count, chapters->content, doc->URL, chapters->tag_name,
						get_outfile_line_number(doc, out_count, chapters->tag_name) );
				return NULL;
			}
		}
		xmlChar *arg = xmlCharStrndup(" -c ", 4);
		arg = xmlStrncat(arg, chapters->content, xmlStrlen(chapters->content));
		return arg;
	}
}

/**
 * @brief Build the audio tracks option for an outfile (-a)
 *
 * @param audio
 * @param doc xml document
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for audio tracks
 */
xmlChar* out_audio(struct tag *audio, xmlDocPtr doc, int out_count)
{
	if (audio->content[0] == '\0') {
		return NULL;
	}
	if (audio->content[0] == ',' || audio->content[xmlStrlen(audio->content)] == ',') {
		fprintf(stderr,
				"%d: Extra leading or trailing ',' (%s) in \"%s\" tag "
				"<%s> line number: %ld\n",
				out_count, audio->content, doc->URL, audio->tag_name,
				get_outfile_line_number(doc, out_count, audio->tag_name) );
		return NULL;
	}
	int comma_count = 0;
	int i;
	for (i = 1; i < xmlStrlen(audio->content); i++) {
		if (audio->content[i] == ',') {
			if ( audio->content[i-1] == ',' ) {
				fprintf(stderr,
						"%d: Extra comma(s) in \"%s\" tag <%s> line number: %ld\n",
						out_count, doc->URL, audio->tag_name,
						get_outfile_line_number(doc, out_count, audio->tag_name) );
				return NULL;
			}
			comma_count++;
		}
		if (!isdigit(audio->content[i]) &&  audio->content[i] != ',') {
			fprintf(stderr,
					"%d: Invalid character(s) in \"%s\" tag <%s> line number: %ld\n",
					out_count, doc->URL, audio->tag_name,
					get_outfile_line_number(doc, out_count, audio->tag_name) );
			return NULL;
		}
	}
	// verify each track number is between 1 and 99
	int *track_numbers = (int *) malloc((comma_count+1)*sizeof(int));
	char **track_strings = (char **) malloc((comma_count+1)*sizeof(char *));
	char *token;
	int track_out_of_range = 0;
	xmlChar *audio_copy = xmlStrdup(audio->content);
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
				out_count, doc->URL, audio->tag_name,
				get_outfile_line_number(doc, out_count, audio->tag_name) );
		return NULL;
	} else {
		xmlChar *arg = xmlCharStrndup(" -a ", 4);
		arg = xmlStrncat(arg, audio->content, xmlStrlen(audio->content));
		return arg;
	}
}


/**
 * @brief Build the subtitle tracks option for an outfile (-s)
 *
 * @param subtitle
 * @param doc xml document
 * @param out_count outfile tag being evaluated
 *
 * @return command line option for subtitle tracks
 */
xmlChar* out_subtitle(struct tag *subtitle, xmlDocPtr doc, int out_count)
{
	if (subtitle->content[0] == '\0') {
		return NULL;
	}
	if (subtitle->content[0] == ',' || subtitle->content[xmlStrlen(subtitle->content)] == ',') {
		fprintf(stderr, "%d: Extra leading or trailing ',' (%s) in \"%s\" tag "
				"<%s> line number: %ld\n",
				out_count, subtitle->content, doc->URL, subtitle->tag_name,
				get_outfile_line_number(doc, out_count, subtitle->tag_name) );
		return NULL;
	}
	int comma_count = 0;
	int i;
	for (i = 0; i < xmlStrlen(subtitle->content); i++) {
		if (subtitle->content[i] == ',') {
			if ( subtitle->content[i-1] == ',' ) {
				fprintf(stderr, "%d: Extra comma(s) in \"%s\" tag "
						"<%s> line number: %ld\n",
						out_count, doc->URL, subtitle->tag_name,
						get_outfile_line_number(doc, out_count, subtitle->tag_name) );
				return NULL;
			}
			comma_count++;
		}
		if (!isdigit(subtitle->content[i]) &&  subtitle->content[i] != ',') {
			fprintf(stderr, "%d: Invalid character(s) in \"%s\" tag "
					"<%s> line number: %ld\n",
					out_count, doc->URL, subtitle->tag_name,
					get_outfile_line_number(doc, out_count, subtitle->tag_name) );
			return NULL;
		}
	}
	int *track_numbers = (int *) malloc((comma_count+1)*sizeof(int));
	char **track_strings = (char **) malloc((comma_count+1)*sizeof(char *));
	char *token;
	int track_out_of_range = 0;
	xmlChar *subtitle_copy = xmlStrdup(subtitle->content);
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
				out_count, doc->URL, subtitle->tag_name,
				get_outfile_line_number(doc, out_count, subtitle->tag_name) );
		return NULL;
	} else {
		xmlChar *arg= xmlCharStrndup(" -s ", 4);
		arg = xmlStrncat(arg, subtitle->content, xmlStrlen(subtitle->content));
		return arg;
	}
}
