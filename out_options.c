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
	xmlChar *format = get_format(doc);
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
		out_options = xmlStrncat(out_options, (const xmlChar *) ".", 1);
		out_options = xmlStrncat(out_options, (const xmlChar *) format, 3);
		out_options = xmlStrncat(out_options, (const xmlChar *) "\"", 1);

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
		out_options = xmlStrncat(out_options, (const xmlChar *) ".", 1);
		out_options = xmlStrncat(out_options, (const xmlChar *) format, 3);
		out_options = xmlStrncat(out_options, (const xmlChar *) "\"", 1);
	} else {
		fprintf(stderr, "%d: Invalid type in \"%s\" tag <%s> line number: %ld\n",
				out_count, doc->URL, tag_name[0],
				get_outfile_line_number(doc, out_count, tag_name[0]) );
		badstring = 1;
	}
	xmlFree(format);

	// Set input filename based off input_basedir
	xmlChar *input_basedir = get_input_basedir(doc);
	out_options = xmlStrncat(out_options, (const xmlChar *) " -i \"", 5);
	int gib_length = xmlStrlen(input_basedir);
	out_options = xmlStrncat(out_options, input_basedir, gib_length);
	if (input_basedir[gib_length-1] != '/') {
		out_options = xmlStrncat(out_options, (const xmlChar *) "/", 1);
	}
	out_options = xmlStrncat(out_options, iso_filename, xmlStrlen(iso_filename) );
	out_options = xmlStrncat(out_options, (const xmlChar *) "\"", 1);

	// check input filename exists or error
	xmlChar* full_path = xmlStrdup(input_basedir);
	if (input_basedir[gib_length-1] != '/') {
		full_path = xmlStrncat(full_path, (const xmlChar *) "/", 1);
	}
	xmlFree(input_basedir);
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
