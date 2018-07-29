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

#ifndef _keyfile_h
#define _keyfile_h
#include <glib.h>

struct config_keys {
    gboolean markers, autocrop, loose_crop, video_turbo, video_two_pass;
    gchar *audio_encoder, *decomb, *deinterlace, *denoise, *grayscale, *rotate,
          *format, *anamorphic, *video_encoder, *video_framerate,
          *video_framerate_control, *input_basedir;
    gint audio_bitrate, audio_quality, video_bitrate, video_quality;
};    

struct config_set {
    gboolean markers, autocrop, loose_crop, video_turbo, video_two_pass,
             audio_encoder, decomb, deinterlace, denoise,
             grayscale, rotate, format, anamorphic, video_encoder,
             video_framerate, video_framerate_control, input_basedir,
             audio_bitrate, audio_quality, video_bitrate, video_quality;
};

struct config {
    struct config_keys key;
    struct config_set set;
};

GKeyFile *parse_key_file(char *infile);
struct config get_config(GKeyFile *key_file);
void free_config(struct config c);
gchar *get_key_value_string(GKeyFile *key_file, const gchar *group_name, const gchar *key, gboolean *set);
gboolean get_key_value_boolean(GKeyFile *key_file, const gchar *group_name, const gchar *key, gboolean *set);
gint get_key_value_integer(GKeyFile *key_file, const gchar *group_name, const gchar *key, gboolean *set);
gdouble get_key_value_double(GKeyFile *key_file, const gchar *group_name, const gchar *key, gboolean *set);
gchar **get_key_value_string_list(GKeyFile *key_file, const gchar *group_name, const gchar *key, gsize *length, gboolean *set);
gboolean *get_key_value_boolean_list(GKeyFile *key_file, const gchar *group_name, const gchar *key, gsize *length, gboolean *set);
gint *get_key_value_integer_list(GKeyFile *key_file, const gchar *group_name, const gchar *key, gsize *length, gboolean *set);
gdouble *get_key_value_double_list(GKeyFile *key_file, const gchar *group_name, const gchar *key, gsize *length, gboolean *set);
int outfile_count(void);
int get_outfile_from_episode(int episode_number);

#endif
