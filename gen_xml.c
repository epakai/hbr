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

#include "gen_xml.h"
#include <ctype.h>                      // for isdigit
#include <errno.h>                      // for errno
#include <stdio.h>                      // for printf, fprintf, fclose, etc
#include <string.h>                     // for strncmp, strnlen, strcspn, etc

/**
 * @brief Argp Parse options for xml generation.
 *
 * @param key Key value associated with each object.
 * @param arg Value for the key being passed.
 * @param state Argp state for the parser
 *
 * @return Error status.
 */
error_t parse_gen_opt(int key, char *arg, struct argp_state *state)
{
	struct gen_arguments *gen_arguments = (struct gen_arguments *) state->input;
	switch (key) {
		case '@':
		case '?':
			break;
		case 'l':
			gen_arguments->episodes = arg;
			gen_arguments->generate = 1;
			break;
		case 'g':
			if (arg != NULL) {
				if ( atoi(arg) > 0 ) {
					gen_arguments->generate = atoi(arg);
				}
			} else {
					gen_arguments->generate = 1;
			}
			break;
		case 'f':
			if (strncmp(arg, "mkv", 3) == 0) {
				gen_arguments->format = arg;
			}
			if (strncmp(arg, "mp4", 3) == 0) {
				gen_arguments->format = arg;
			}
			break;
		case 's':
			gen_arguments->source = arg;
			break;
		case 'p':
			if (strncmp(arg, "movie", 5) == 0) {
				gen_arguments->video_type = 0;
			}
			if (strncmp(arg, "series", 6) == 0) {
				gen_arguments->video_type = 1;
			}
			break;
		case 't':
			if ( atoi(arg) >= 1  && atoi(arg) <= 99 ) {
				gen_arguments->title = atoi(arg);
			}
			break;
		case 'y':
			gen_arguments->year = arg;
			break;
		case 'c':
			gen_arguments->crop = arg;
			break;
		case 'n':
			gen_arguments->name = arg;
			break;
		case 'e':
			if ( atoi(arg) > 0 ) {
				gen_arguments->season = atoi(arg);
			}
			break;
		case 'b':
			gen_arguments->basedir = arg;
			break;
		case 'm':
			gen_arguments->markers =  true;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/**
 * @brief Builds an array of episode numbers and titles from a file
 * One line per episode with episode number, a single non-digit separator,
 * and the episode title.
 *
 * @param episode_filename Path to the episode list.
 * @param episode_array Address of a struct episode pointer. Caller should
 * call free_episode_array() once use is complete.
 *
 * @return The number of episodes stored in episode_array
 */
int read_episode_list(const char *episode_filename, struct episode **episode_array)
{
	int max_count = 30;
	int episode_list_count = 0;
	FILE *el_file;
	errno = 0;
	el_file = fopen(episode_filename, "r");
	if (el_file == NULL)  {
		fprintf(stderr, "\"%s\" : ", episode_filename);
		perror("Failed to open episode list");
		return 0;
	}
	*episode_array = malloc(max_count * sizeof(struct episode));
	char * buf = malloc(200*sizeof(char));
	while (fgets(buf, 200, el_file) != NULL && episode_list_count < max_count) {
		buf[strcspn(buf, "\n")] = 0;
		int n = 0;
		char ep_number[5] = "\0";
		while (isdigit(buf[n]) && n < 5) {
			ep_number[n] = buf[n];
			n++;
		}
		if (n == 0) {
			free_episode_array(*episode_array, episode_list_count);
			free(buf);
			fclose(el_file);
			fprintf(stderr, "No episode number found on line: %d\n", episode_list_count+1);
			return 0;
		}
		ep_number[n+1] = '\0';
		n++;
		(*episode_array)[episode_list_count].number = atoi(ep_number);

		(*episode_array)[episode_list_count].name =
			malloc( (strnlen(buf+n, 200)+1)*sizeof(char) );
		if ((*episode_array)[episode_list_count].name != NULL) {
			strncpy((*episode_array)[episode_list_count].name, buf+n, strnlen(buf+n, 200)+1 );
		} else {
			free_episode_array(*episode_array, episode_list_count);
			free(buf);
			fclose(el_file);
			fprintf(stderr, "Failed malloc call for episode name.\n");
			return 0;
		}
		if ( episode_list_count == max_count - 1 ) {
			struct episode *temp;
			if ( (temp = realloc(*episode_array, (max_count+20) * sizeof(struct episode))) != NULL){
				*episode_array = temp;
				max_count += 20;
			}
		}
		episode_list_count++;
	}
	free(buf);
	fclose(el_file);
	return episode_list_count;
}

/**
 * @brief Frees episode_array allocated by read_episode_list()
 *
 * @param episode_array episode_array to be freed.
 * @param count Number of struct episodes in the episode_array
 */
void free_episode_array(struct episode *episode_array, int count) {
	int i;
	for ( i = 0; i < count; i++) {
		free(episode_array[i].name);
	}
	free(episode_array);
	episode_array = NULL;
}

/**
 * @brief Generates xml to standard output based on various options
 *
 * @param outfiles_count Number of outfile sections to generate.
 * Ignored if episodes argument is given.
 * @param title DVD title number
 * @param season Season number
 * @param video_type Indicate a movie or series
 * @param markers Chapter markers
 * @param source Source filename (iso file)
 * @param year Year published
 * @param crop Crop amount (T:B:L:R)
 * @param name General title for a series or movie
 * @param format Output file format
 * @param basedir Location for source file
 * @param episodes Path for episode list file. Overides outfiles_count.
 */
void gen_xml(int outfiles_count, int title, int season, int video_type,
		bool markers, const char *source, const char *year,
		const char *crop, const char *name, const char *format,
		const char *basedir, const char *episodes)
{
	struct episode *episode_array = NULL;
	if (episodes != NULL) {
		outfiles_count = read_episode_list(episodes, &episode_array);
		if (outfiles_count == 0) {
			fprintf(stderr, "Failed to parse episode list.\n");
			return;
		}
	}
	if (outfiles_count <= 0 || outfiles_count > 999) {
		fprintf(stderr, "Invalid number of outfile sections: "
				"%d (should be between 1 and 999).\n", outfiles_count);
		return;
	}
	printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	printf("<!DOCTYPE handbrake_encode [\n");
	printf("<!ELEMENT handbrake_encode (handbrake_options, outfile+)>\n");
	printf("<!ELEMENT handbrake_options EMPTY>\n");
	printf("<!ATTLIST handbrake_options");
	printf("          format (mp4|mkv) \"mkv\"\n");
	printf("          video_encoder (x264|mpeg4|mpeg2|VP8|theora) \"x264\"\n");
	printf("          video_quality CDATA \"18\"\n");
	printf("          audio_encoder (av_aac|fdk_aac|fdk_haac|"
			"copy:aac|ac3|copy:ac3|copy:dts|copy:dtshd|mp3|"
			"copy:mp3|vorbis|flac16|flac24|copy) \"fdk_aac\"\n");
	printf("          audio_bitrate CDATA #IMPLIED\n");
	printf("          audio_quality CDATA #IMPLIED\n");
	printf("          anamorphic (loose|strict) \"strict\"\n");
	printf("          deinterlace (fast|slow|slower|bob|default|none) \"none\"\n");
	printf("          decomb (fast|bob|default|none) \"default\"\n");
	printf("          denoise (ultralight|light|medium|strong|default|none) \"none\"\n");
	printf("          markers (yes|no) \"yes\"\n");
	printf("          input_basedir CDATA #IMPLIED>\n");
	printf("<!ELEMENT outfile (type, iso_filename, dvdtitle, name, year, season, "
			"episode_number, specific_name?, crop?, chapters, audio, audio_names?, subtitle?)>\n");
	printf("<!ELEMENT type (#PCDATA)>\n");
	printf("<!ELEMENT iso_filename (#PCDATA)>\n");
	printf("<!ELEMENT dvdtitle (#PCDATA)>\n");
	printf("<!ELEMENT name (#PCDATA)>\n");
	printf("<!ELEMENT year (#PCDATA)>\n");
	printf("<!ELEMENT season (#PCDATA)>\n");
	printf("<!ELEMENT episode_number (#PCDATA)>\n");
	printf("<!ELEMENT specific_name (#PCDATA)>\n");
	printf("<!ELEMENT crop (#PCDATA)>\n");
	printf("<!ELEMENT chapters (#PCDATA)>\n");
	printf("<!ELEMENT audio (#PCDATA)>\n");
	printf("<!ELEMENT subtitle (#PCDATA)>\n");
	printf("]>\n");
	printf("<handbrake_encode>\n \t<handbrake_options \n"
			"\t\t\tformat=\"%s\"\n \t\t\tvideo_encoder=\"x264\"\n"
			"\t\t\tvideo_quality=\"18\"\n \t\t\taudio_encoder=\"fdk_aac\" \n"
			"\t\t\taudio_bitrate=\"192\" \n \t\t\taudio_quality=\"\"\n"
			"\t\t\tanamorphic=\"strict\"\n \t\t\tdeinterlace=\"none\"\n"
			"\t\t\tdecomb=\"default\"\n \t\t\tdenoise=\"none\"\n"
			"\t\t\tinput_basedir=\"%s\"\n", format, basedir);
	if ( markers ) {
		printf("\t\t\tmarkers=\"yes\"\n");
	} else {
		printf("\t\t\tmarkers=\"no\"\n");
	}
	printf("\t\t\t/>\n");
	int i;
	for (i = 0; i< outfiles_count; i++) {
		if ( video_type == 0) {
			printf("\t<outfile>\n \t\t<type>movie</type>");
		} else if ( video_type == 1) {
			printf("\t<outfile>\n \t\t<type>series</type>");
		} else {
			printf("\t<outfile>\n \t\t<type></type>");
		}
		if (i == 0) {
			printf("\t\t<!-- type may be series or movie -->");
		}
		printf("\n\t\t<iso_filename>%s</iso_filename>\n", source);
		printf("\t\t<dvdtitle>%d</dvdtitle>\n", title);
		if (i == 0) {
			printf("\t\t<!-- filename depends on outfile type -->\n"
					"\t\t<!-- series filename: "
					"\"<name> - s<season>e<episode_number> - <specific_name>\" -->\n"
					"\t\t<!-- movie filename: \"<name> (<year>)\" -->\n");
		}
		printf("\t\t<name>%s</name>\n \t\t<year>%s</year>\n"
				"\t\t<season>%d</season>\n",
				name, year, season);
		if (episodes != NULL){
			printf("\t\t<episode_number>%d</episode_number>\n"
				"\t\t<specific_name>%s</specific_name>\n",
				episode_array[i].number, episode_array[i].name);
		} else {
		printf("\t\t<episode_number></episode_number>\n"
				"\t\t<specific_name></specific_name>\n");
		}
		printf("\t\t<crop>%s</crop>\n", crop);
		printf("\t\t<chapters></chapters>");
		if (i == 0) {
			printf("\t\t<!-- specified as a range \"1-3\" or single chapter \"3\" -->");
		}
		printf("\n\t\t<audio></audio>");
		if (i == 0) {
			printf("\t\t<!-- comma separated list of audio tracks -->");
		}
		printf("\n\t\t<subtitle></subtitle>");
		if (i == 0) {
			printf("\t\t<!-- comma separated list of subtitle tracks -->");
		}
		printf("\n\t</outfile>\n");
	}
	printf("</handbrake_encode>\n");
	
	if (episodes != NULL) {
		free_episode_array(episode_array, outfiles_count);
	}
	
	return;
}
