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

#include <ctype.h>
#include <glib.h>


gchar* hb_options_string(GKeyFile* keyfile);
gchar* get_format(GKeyFile*);
gchar* get_input_basedir(GKeyFile*);

gchar* hb_format(gchar* );
gchar* hb_video_encoder(gchar* );
gchar* hb_video_quality(gchar* , GKeyFile*);
gchar* hb_audio_encoder(gchar* );
gchar* hb_audio_quality(gchar* , GKeyFile*);
gchar* hb_audio_bitrate(gchar* , GKeyFile*);
gchar* hb_markers(gchar* );
gchar* hb_anamorphic(gchar* );
gchar* hb_deinterlace(gchar* );
gchar* hb_decomb(gchar* );
gchar* hb_denoise(gchar* );

gboolean valid_bit_rate(int bitrate, int minimum, int maximum);

#endif
