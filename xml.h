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

#ifndef _xml_h
#define _xml_h

#include <libxml/xpath.h>
#include <libxml/xmlschemas.h>

xmlDocPtr parse_xml(char *);

xmlXPathObjectPtr xpath_get_object(xmlDocPtr doc, xmlChar *xpath_expr);
xmlChar* get_outfile_child_content(xmlDocPtr doc, int out_count, xmlChar *child);
xmlNode* get_outfile(xmlDocPtr doc, int out_count);
long int get_outfile_line_number(xmlDocPtr doc, int out_count, xmlChar *child);
int outfile_count(xmlDocPtr doc);
int get_outfile_from_episode(xmlDocPtr doc, int episode_number);


#endif
