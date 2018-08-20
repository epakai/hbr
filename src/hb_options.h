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
#include "keyfile.h"


gchar* hb_options_string(struct config, GKeyFile*);
gchar* get_format(GKeyFile*);
gchar* get_input_basedir(GKeyFile*);

gchar* hb_format(struct config, struct config);
gchar* hb_markers(struct config, struct config);

gchar* hb_picture_anamorphic(struct config, struct config);
gchar* hb_picture_autocrop(struct config, struct config);
gchar* hb_picture_loose_crop(struct config, struct config);

gchar* hb_filter_decomb(struct config, struct config);
gchar* hb_filter_deinterlace(struct config, struct config);
gchar* hb_filter_denoise(struct config, struct config);
gchar* hb_filter_grayscale(struct config, struct config);
gchar* hb_filter_rotate(struct config, struct config);

gchar* hb_audio_encoder(struct config, struct config);
gchar* hb_audio_quality(struct config, struct config);
gchar* hb_audio_bitrate(struct config, struct config);

gchar* hb_video_encoder(struct config, struct config);
gchar* hb_video_quality(struct config, struct config);
gchar* hb_video_bitrate(struct config, struct config);
gchar* hb_video_framerate(struct config, struct config);
gchar* hb_video_framerate_control(struct config, struct config);
gchar* hb_video_turbo(struct config, struct config); // also sets two-pass option

gboolean valid_bit_rate(int bitrate, int minimum, int maximum);

#endif
