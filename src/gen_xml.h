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

#ifndef _gen_xml_h
#define _gen_xml_h

#include "out_options.h"
#include <argp.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libxml/tree.h>

/**
 * @brief Episode structure used to build episode list when -l option is used.
 */
struct episode {
	int number;      /**< Episode number. */
	char * name;     /**< Episode name. */
};

/**
 * @brief Pointer array of episodes with a count
 */
struct episode_list {
	int count;
	struct episode *array;
};

struct episode_list read_episode_list(const char *episode_filename);

void free_episode_list(struct episode_list list);

xmlDocPtr gen_xml(int outfiles_count, int dvdtitle, int season, int video_type,
		bool markers, const char *source, const char *year,
		struct crop vcrop, const char *name, const char *format,
		const char *basedir, const char *episodes);

void create_outfile_section( xmlNodePtr parent, bool comment, int *video_type,
		const char *iso_filename, int *dvdtitle, const char *name,
		const char *year, int *season, int *episode_number,
		const char *specific_name, struct crop *crop, int *chapters_start,
		int *chapters_end, const char *audio, const char *subtitle);

void print_xml(xmlDocPtr doc);

struct crop get_crop(xmlChar * crop_string);


#endif
