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
#ifndef _out_options_h
#define _out_options_h

#include <libxml/xpath.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "xml.h"
#include "hb_options.h"

struct tag {
	xmlChar *content;
	xmlChar *tag_name;
};

xmlChar* out_options_string(xmlDocPtr doc, int out_count);
int validate_file_string(xmlChar * file_string);

xmlChar* out_series_output(struct tag *name, struct tag *season,
		struct tag *episode_number, struct tag *specific_name,
		xmlDocPtr doc, int out_count);
xmlChar* out_movie_output(struct tag *name, struct tag *year,
		struct tag *specific_name, xmlDocPtr doc, int out_count);
xmlChar* out_input(struct tag *iso_filename, xmlDocPtr doc, int out_count);
xmlChar* out_dvdtitle(struct tag *dvdtitle, xmlDocPtr doc, int out_count);
xmlChar* out_crop(struct tag *crop, xmlDocPtr doc, int out_count);
xmlChar* out_chapters(struct tag *chapters, xmlDocPtr doc, int out_count);
xmlChar* out_audio(struct tag *audio, xmlDocPtr doc, int out_count);
xmlChar* out_subtitle(struct tag *subtitle, xmlDocPtr doc, int out_count);

#endif
