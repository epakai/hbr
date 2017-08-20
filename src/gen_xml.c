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
 * @brief Builds an episode_list from a file
 *
 * @param episode_filename Path to the episode list.
 *
 * @return List of episodes, must be free'd with free_episode_list()
 */
struct episode_list read_episode_list(const char *episode_filename)
{
	int max_count = 30;
	struct episode_list list;
	list.count = 0;
	FILE *el_file;
	errno = 0;
	el_file = fopen(episode_filename, "r");
	// Try to open the file
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
			if ((temp = realloc(list.array, (max_count+20) * sizeof(struct episode))) != NULL){
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
 * @param list packed list of episodes, see gen_xml.h
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
 * @param dvdtitle DVD title number
 * @param season Season number
 * @param video_type Indicate a movie or series
 * @param markers Chapter markers
 * @param source Source filename (iso file)
 * @param year Year published
 * @param vcrop Crop amount (T:B:L:R)
 * @param name General title for a series or movie
 * @param format Output file format
 * @param basedir Location for source file
 * @param episodes Path for episode list file. Overides outfiles_count.
 *
 * @return xmlDocPtr to the generated document
 */
xmlDocPtr gen_xml(int outfiles_count, int dvdtitle, int season, int video_type,
		bool markers, const char *source, const char *year,
		struct crop vcrop, const char *name, const char *format,
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
		char *specific_name = "";
		if (episodes != NULL){
			create_outfile_section(root, (i == 0)?true:false, &video_type, source,
					&dvdtitle, name, year, &season,
					&(list.array[i].number),
					list.array[i].name, &vcrop,  NULL, NULL,
					NULL, NULL);
		} else {
			int episode_number = i;
			create_outfile_section(root, (i == 0)?true:false, &video_type, source,
					&dvdtitle, name, year, &season,
					&episode_number, specific_name, &vcrop,
					NULL, NULL, NULL, NULL);
		}
	}
	if (episodes != NULL) {
		free_episode_list(list);
	}
	return doc;
}

/**
 * @brief Create an outfile section in an xml document given the parent xmlNodePtr.
 * All arguments are pointers. NULL can be passed for the element to be left empty.
 *
 * @param parent an xmlNodePtr that is the parent element for this outfile section
 * @param comment whether the output should include comments describing each child element
 * @param video_type 0 for movie, 1 for series, null pointer to remain empty
 * @param iso_filename path and filename for the source ISO
 * @param dvdtitle numeric dvd title (1-99)
 * @param name Series or movie name
 * @param year Release year
 * @param season Season number
 * @param episode_number Episode number
 * @param specific_name Name of the episode or particular movie version
 * @param crop Crop values in a struct crop
 * @param chapters_start Chapter to begin ripping
 * @param chapters_end Final chapter to be ripped
 * @param audio Comma separated list of audio track numbers
 * @param subtitle Comma separated list of subtitle track numbers
 */
void create_outfile_section( xmlNodePtr parent, bool comment,
		int *video_type, const char *iso_filename, int *dvdtitle,
		const char *name, const char *year, int *season,
		int *episode_number, const char *specific_name, struct crop *crop,
		int *chapters_start, int *chapters_end, const char *audio,
		const char *subtitle)
{
	xmlNodePtr outfile_node = xmlNewChild(parent, NULL, BAD_CAST "outfile", NULL);
	// TAG: video_type
	if (comment) {
		xmlAddChild(outfile_node, xmlNewComment(BAD_CAST " type may be series or movie "));
	}
	if ( *video_type == 0) {
		xmlNewChild(outfile_node, NULL, BAD_CAST "type", BAD_CAST "movie");
	} else if ( *video_type == 1) {
		xmlNewChild(outfile_node, NULL, BAD_CAST "type", BAD_CAST "series");
	} else {
		xmlNewChild(outfile_node, NULL, BAD_CAST "type", NULL);
	}
	// TAG: iso_filename
	xmlNewChild(outfile_node, NULL, BAD_CAST "iso_filename", BAD_CAST iso_filename);
	// TAG: dvdtitle
	char title_s[3];
	snprintf(title_s, 3, "%d", *dvdtitle);
	xmlNewChild(outfile_node, NULL, BAD_CAST "dvdtitle", BAD_CAST title_s);
	if (comment) {
		xmlAddChild(outfile_node, xmlNewComment(BAD_CAST " filename depends on outfile type "));
		xmlAddChild(outfile_node, xmlNewComment(BAD_CAST
				" series filename: <name> - s<season>e<episode_number> - <specific_name> "));
		xmlAddChild(outfile_node, xmlNewComment(BAD_CAST " movie filename: <name> (<year>) "));
		xmlAddChild(outfile_node, xmlNewComment(BAD_CAST
				" Replace characters '&', '<', and '>' in specific_name with their xml entities &amp; &lt; &gt; "));
	}
	// TAG: name
	xmlNewTextChild(outfile_node, NULL, BAD_CAST "name", BAD_CAST name);
	// TAG: year
	xmlNewChild(outfile_node, NULL, BAD_CAST "year", BAD_CAST year);
	// TAG: season
	char season_s[3];
	snprintf(season_s, 3, "%d", *season);
	xmlNewChild(outfile_node, NULL, BAD_CAST "season", BAD_CAST season_s);
	// TAG: episode_number
	char ep_num_s[5];
	snprintf(ep_num_s, 5, "%d", *episode_number);
	xmlNewChild(outfile_node, NULL, BAD_CAST "episode_number", BAD_CAST ep_num_s);
	if (comment) {
		xmlAddChild(outfile_node, xmlNewComment(BAD_CAST
					" Replace characters '&', '<', and '>' in specific_name with their xml entities &amp; &lt; &gt; "));
	}
	// TAG: specific_name
	xmlNewTextChild(outfile_node, NULL, BAD_CAST "specific_name", BAD_CAST specific_name);
	// TAG: crop
	xmlNodePtr crop_node = xmlNewChild(outfile_node, NULL, BAD_CAST "crop", NULL);
	char crop_s[5];
	if (crop == NULL) {
		snprintf(crop_s, 4, "%d", 0);
		xmlNewChild(crop_node, NULL, BAD_CAST "top", BAD_CAST crop_s);
		xmlNewChild(crop_node, NULL, BAD_CAST "top", BAD_CAST crop_s);
		xmlNewChild(crop_node, NULL, BAD_CAST "top", BAD_CAST crop_s);
		xmlNewChild(crop_node, NULL, BAD_CAST "top", BAD_CAST crop_s);
	} else {
		snprintf(crop_s, 4, "%d", crop->top);
		xmlNewChild(crop_node, NULL, BAD_CAST "top", BAD_CAST crop_s);
		snprintf(crop_s, 4, "%d", crop->bottom);
		xmlNewChild(crop_node, NULL, BAD_CAST "bottom", BAD_CAST crop_s);
		snprintf(crop_s, 4, "%d", crop->left);
		xmlNewChild(crop_node, NULL, BAD_CAST "left", BAD_CAST crop_s);
		snprintf(crop_s, 4, "%d", crop->right);
		xmlNewChild(crop_node, NULL, BAD_CAST "right", BAD_CAST crop_s);
	}
	// TAG: chapters
	xmlNodePtr chapters_node = xmlNewChild(outfile_node, NULL, BAD_CAST "chapters", NULL);
	if (chapters_start == NULL) {
		xmlNewChild(chapters_node, NULL, BAD_CAST "start", NULL);
	} else {
		char start_s[3];
		snprintf(start_s, 3, "%d", *chapters_start);
		xmlNewChild(chapters_node, NULL, BAD_CAST "start", BAD_CAST start_s);
	}
	if (chapters_end == NULL) {
		xmlNewChild(chapters_node, NULL, BAD_CAST "end", NULL);
	} else {
		char end_s[3];
		snprintf(end_s, 3, "%d", *chapters_end);
		xmlNewChild(chapters_node, NULL, BAD_CAST "end", BAD_CAST end_s);
	}
	// TAG: audio
	if (comment) {
		xmlAddChild(outfile_node, xmlNewComment(BAD_CAST " List of tracks, space separated"));
	}
	xmlNewChild(outfile_node, NULL, BAD_CAST "audio", BAD_CAST audio);
	// TAG: subtitle
	if (comment) {
		xmlAddChild(outfile_node, xmlNewComment(BAD_CAST " List of tracks, space separated"));
	}
	xmlNewChild(outfile_node, NULL, BAD_CAST "subtitle", BAD_CAST subtitle);
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

/**
 * @brief Build a crop object from the Handbrake style string
 *
 * @param crop_string String 4 colon separated integers i.e. 0:0:0:0
 *
 * @return crop object,
 */
struct crop get_crop(xmlChar * crop_string)
{
	if (xmlStrcmp(crop_string, BAD_CAST "") == 0 || crop_string == NULL) {
		struct crop crop = {0, 0, 0, 0};
		return crop;
	}
	xmlChar *token_string = xmlStrdup(crop_string);
	char *crop_str[5];
	// break crop into components
	crop_str[0] = strtok((char *) token_string, ":");
	crop_str[1] = strtok(NULL, ":");
	crop_str[2] = strtok(NULL, ":");
	crop_str[3] = strtok(NULL, "\0");
	// verify each crop is 4 digit max (video isn't that big yet)
	if ((strnlen(crop_str[0], 5) > 4) || (strnlen(crop_str[1], 5) > 4)
			|| (strnlen(crop_str[2], 5) > 4) || (strnlen(crop_str[3], 5) > 4)){
		fprintf(stderr, "Invalid crop (value too large) "
				"'%s:%s:%s:%s'\n",
				crop_str[0], crop_str[1], crop_str[2], crop_str[3]);
		struct crop crop = {0, 0, 0, 0};
		return crop;
	}
	int i;
	// verify all characters are digits
	for (i = 0; i<4; i++){
		int j = 0;
		while (crop_str[i][j] != '\0'){
			if (!isdigit(crop_str[i][j])){
				fprintf(stderr, "Invalid crop (non-digits) "
						"'%s:%s:%s:%s'\n",
						crop_str[0], crop_str[1], crop_str[2], crop_str[3]);
				struct crop crop = {0, 0, 0, 0};
				return crop;
			}
			j++;
		}
	}
	struct crop crop;
	crop.top = atoi(crop_str[0]);
	crop.bottom = atoi(crop_str[1]);
	crop.left = atoi(crop_str[2]);
	crop.right = atoi(crop_str[3]);
	xmlFree(token_string);
	return crop;
}
