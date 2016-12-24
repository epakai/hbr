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
#ifndef _hb_options_h
#define _hb_options_h

#include <libxml/xpath.h>
#include <ctype.h>

xmlChar* hb_options_string(xmlDocPtr doc);
xmlChar* get_format(xmlDocPtr);
xmlChar* get_input_basedir(xmlDocPtr);

xmlChar* hb_format(xmlNode *);
xmlChar* hb_video_encoder(xmlNode *);
xmlChar* hb_video_quality(xmlNode *, xmlDocPtr);
xmlChar* hb_audio_encoder(xmlNode *);
xmlChar* hb_audio_quality(xmlNode *, xmlDocPtr);
xmlChar* hb_audio_bitrate(xmlNode *, xmlDocPtr);
xmlChar* hb_crop(xmlNode *);
xmlChar* hb_markers(xmlNode *);
xmlChar* hb_anamorphic(xmlNode *);
xmlChar* hb_deinterlace(xmlNode *);
xmlChar* hb_decomb(xmlNode *);
xmlChar* hb_denoise(xmlNode *);

int valid_bit_rate(int bitrate, int minimum, int maximum);

#endif
