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
#include <libxml/xmlsave.h>

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
struct episode_list read_episode_list(const char *episode_filename)
{
	int max_count = 30;
	struct episode_list list;
	list.count = 0;
	FILE *el_file;
	errno = 0;
	el_file = fopen(episode_filename, "r");
	if (el_file == NULL)  {
		fprintf(stderr, "\"%s\" : ", episode_filename);
		perror("Failed to open episode list");
		list.count = 0;
		list.array = NULL;
		return list;
	}

	list.array = calloc(max_count, sizeof(struct episode));
	char * buf = malloc(200*sizeof(char));
	while (fgets(buf, 200, el_file) != NULL && list.count < max_count) {
		// null terminate each line
		buf[strcspn(buf, "\n")] = '\0';

		// Check for digit string, null terminate it
		// convert to integer for the episode number
		int n = 0;
		char ep_number[5] = "\0";
		while (isdigit(buf[n]) && n < 5) {
			ep_number[n] = buf[n];
			n++;
		}
		if (n == 0) {
			free_episode_list(list);
			free(buf);
			fclose(el_file);
			fprintf(stderr, "No episode number found on line: %d\n", list.count+1);
			list.count = 0;
			list.array = NULL;
			return list;
		}
		ep_number[n+1] = '\0';
		n++;
		list.array[list.count].number = atoi(ep_number);
		
		// store the rest of the string as the episode name
		list.array[list.count].name =
			malloc( (strnlen(buf+n, 200)+1)*sizeof(char) );
		if (list.array[list.count].name != NULL) {
			strncpy(list.array[list.count].name, buf+n, strnlen(buf+n, 200)+1 );
		} else {
			free_episode_list(list);
			free(buf);
			fclose(el_file);
			fprintf(stderr, "Failed malloc call for episode name.\n");
			list.count = 0;
			list.array = NULL;
			return list;
		}
		// Allocate more space if episodes is about to exceed max_count
		if ( list.count == max_count - 1 ) {
			struct episode *temp;
			if ( (temp = realloc(list.array, (max_count+20) * sizeof(struct episode))) != NULL){
				list.array = temp;
				max_count += 20;
			}
		}
		list.count++;
	}
	free(buf);
	fclose(el_file);
	return list;
}

/**
 * @brief Frees episode_array allocated by read_episode_list()
 *
 * @param episode_array episode_array to be freed.
 * @param count Number of struct episodes in the episode_array
 */
void free_episode_list(struct episode_list list)
{
	int i;
	for ( i = 0; i < list.count; i++) {
		free(list.array[i].name);
	}
	free(list.array);
	list.count = 0;
	list.array = NULL;
}

/**
 * @brief Generates xmlDoc object outline for hbr input
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
 *
 * @return xmlDocPtr to the generated document
 */
xmlDocPtr gen_xml(int outfiles_count, int title, int season, int video_type,
		bool markers, const char *source, const char *year,
		struct crop crop, const char *name, const char *format,
		const char *basedir, const char *episodes)
{
	struct episode_list list;
	list.count = 0;
	list.array = NULL;
	if (episodes != NULL) {
		list = read_episode_list(episodes);
		outfiles_count = list.count;
		if (outfiles_count == 0) {
			fprintf(stderr, "Failed to parse episode list.\n");
			free_episode_list(list);
			return NULL;
		}
	}
	if (outfiles_count <= 0 || outfiles_count > 999) {
		fprintf(stderr, "Invalid number of outfile sections: "
				"%d (should be a value of 1 to 999).\n", outfiles_count);
		return NULL;
	}
	
	xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
	xmlNodePtr root = xmlNewNode(NULL, BAD_CAST "handbrake_encode");
	xmlDocSetRootElement(doc, root);
	
	// create handbrake_options with defaults
	xmlNodePtr opt_node = xmlNewChild(root, NULL, BAD_CAST "handbrake_options", NULL);
	xmlNewProp(opt_node, BAD_CAST "format", BAD_CAST format);
	xmlNewProp(opt_node, BAD_CAST "video_encoder", BAD_CAST "x264");
	xmlNewProp(opt_node, BAD_CAST "video_quality", BAD_CAST "18");
	xmlNewProp(opt_node, BAD_CAST "audio_encoder", BAD_CAST "fdk_aac");
	xmlNewProp(opt_node, BAD_CAST "audio_bitrate", BAD_CAST "192");
	xmlNewProp(opt_node, BAD_CAST "audio_quality", BAD_CAST "");
	xmlNewProp(opt_node, BAD_CAST "anamorphic", BAD_CAST "strict");
	xmlNewProp(opt_node, BAD_CAST "deinterlace", BAD_CAST "none");
	xmlNewProp(opt_node, BAD_CAST "decomb", BAD_CAST "default");
	xmlNewProp(opt_node, BAD_CAST "denoise", BAD_CAST "none");
	xmlNewProp(opt_node, BAD_CAST "input_basedir", BAD_CAST basedir);
	if (markers) {
		xmlNewProp(opt_node, BAD_CAST "markers", BAD_CAST "yes");
	} else {
		xmlNewProp(opt_node, BAD_CAST "markers", BAD_CAST "no");
	}
	int i;
	for (i = 0; i< outfiles_count; i++) {
		xmlNodePtr outfile_node = xmlNewChild(root, NULL, BAD_CAST "outfile", NULL);

		if (i == 0) {
			xmlAddChild(outfile_node, xmlNewComment(BAD_CAST " type may be series or movie "));
		}
		if ( video_type == 0) {
			xmlNewChild(outfile_node, NULL, BAD_CAST "type", BAD_CAST "movie");
		} else if ( video_type == 1) {
			xmlNewChild(outfile_node, NULL, BAD_CAST "type", BAD_CAST "series");
		} else {
			xmlNewChild(outfile_node, NULL, BAD_CAST "type", NULL);
		}
		xmlNewChild(outfile_node, NULL, BAD_CAST "iso_filename", BAD_CAST source);
		char title_s[3];
		snprintf(title_s, 3, "%d", title);
		xmlNewChild(outfile_node, NULL, BAD_CAST "dvdtitle", BAD_CAST title_s);
		if (i == 0) {
			xmlAddChild(outfile_node, xmlNewComment(BAD_CAST " filename depends on outfile type "));
			xmlAddChild(outfile_node, xmlNewComment(BAD_CAST
						" series filename: <name> - s<season>e<episode_number> - <specific_name> "));
			xmlAddChild(outfile_node, xmlNewComment(BAD_CAST " movie filename: <name> (<year>) "));
			xmlAddChild(outfile_node, xmlNewComment(BAD_CAST
						" Replace characters '&<>' in name with their xml entities &amp; &lt; &gt; "));
		}
		xmlNewTextChild(outfile_node, NULL, BAD_CAST "name", BAD_CAST name);
		xmlNewChild(outfile_node, NULL, BAD_CAST "year", BAD_CAST year);
		char season_s[3];
		snprintf(season_s, 3, "%d", season);
		xmlNewChild(outfile_node, NULL, BAD_CAST "season", BAD_CAST season_s);
		if (episodes != NULL){
			char ep_num_s[5];
			snprintf(ep_num_s, 5, "%d", list.array[i].number);
			xmlChar *specific_name = xmlCharStrdup(list.array[i].name);
			xmlNewChild(outfile_node, NULL, BAD_CAST "episode_number", BAD_CAST ep_num_s);
			if (i == 0) {
				xmlAddChild(outfile_node, xmlNewComment(BAD_CAST
							" Replace characters '&<>' in specific_name with their xml entities &amp; &lt; &gt; "));
			}
			xmlNewTextChild(outfile_node, NULL, BAD_CAST "specific_name", specific_name);
			xmlFree(specific_name);
		} else {
			xmlNewChild(outfile_node, NULL, BAD_CAST "episode_number", NULL);
			if (i == 0) {
				xmlAddChild(outfile_node, xmlNewComment(BAD_CAST
							" Replace characters '&<>' in specific_name with their xml entities &amp; &lt; &gt; "));
			}
			xmlNewChild(outfile_node, NULL, BAD_CAST "specific_name", NULL);
		}
		xmlNodePtr crop_node = xmlNewChild(outfile_node, NULL, BAD_CAST "crop", NULL);
		char crop_s[5];
		snprintf(crop_s, 4, "%d", crop.top);
		xmlNewChild(crop_node, NULL, BAD_CAST "top", BAD_CAST crop_s);
		snprintf(crop_s, 4, "%d", crop.bottom);
		xmlNewChild(crop_node, NULL, BAD_CAST "bottom", BAD_CAST crop_s);
		snprintf(crop_s, 4, "%d", crop.left);
		xmlNewChild(crop_node, NULL, BAD_CAST "left", BAD_CAST crop_s);
		snprintf(crop_s, 4, "%d", crop.right);
		xmlNewChild(crop_node, NULL, BAD_CAST "right", BAD_CAST crop_s);
		xmlNodePtr chapters_node = xmlNewChild(outfile_node, NULL, BAD_CAST "chapters", NULL);
		xmlNewChild(chapters_node, NULL, BAD_CAST "start", NULL);
		xmlNewChild(chapters_node, NULL, BAD_CAST "end", NULL);
		if (i == 0) {
			xmlAddChild(outfile_node, xmlNewComment(BAD_CAST " List of tracks, space separated"));
		}
		xmlNewChild(outfile_node, NULL, BAD_CAST "audio", NULL);
		if (i == 0) {
			xmlAddChild(outfile_node, xmlNewComment(BAD_CAST " List of tracks, space separated"));
		}
		xmlNewChild(outfile_node, NULL, BAD_CAST "subtitle", NULL);
	}
	

	if (episodes != NULL) {
		free_episode_list(list);
	}
	
	return doc;
}

/**
 * @brief Writes an xmlDoc to file descriptor 1 (stdout)
 * with formatting and no short tags
 *
 * @param doc pointer to an xmlDoc to be printed
 */
void print_xml(xmlDocPtr doc)
{
	xmlSaveCtxtPtr save_ctxt = xmlSaveToFd(1, "UTF-8", XML_SAVE_FORMAT | XML_SAVE_NO_EMPTY );
	xmlSaveDoc(save_ctxt, doc);
	xmlSaveClose(save_ctxt);
	xmlFreeDoc(doc);
	xmlCleanupParser();
}
