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
    gchar*     format;
    gboolean   markers;
    gchar*     input_basedir; // only used for local configs

    gchar*     picture_anamorphic;
    gboolean   picture_autocrop;
    gint       picture_loose_crop;

    gchar*     filter_decomb;
    gchar*     filter_deinterlace;
    gchar*     filter_denoise;
    gchar*     filter_grayscale;
    gchar*     filter_rotate;

    gchar*     audio_encoder;
    gint       audio_quality;
    gint       audio_bitrate;

    gchar*     video_encoder;
    gint       video_quality;
    gint       video_bitrate;
    gchar*     video_framerate;
    gchar*     video_framerate_control;
    gboolean   video_turbo;
    gboolean   video_two_pass;
};

struct config_set {
    gboolean format;
    gboolean markers;
    gboolean input_basedir; // only used for local configs

    gboolean picture_anamorphic;
    gboolean picture_autocrop;
    gint     picture_loose_crop;

    gboolean filter_decomb;
    gboolean filter_deinterlace;
    gboolean filter_denoise;
    gboolean filter_grayscale;
    gboolean filter_rotate;

    gboolean audio_encoder;
    gboolean audio_quality;
    gboolean audio_bitrate;

    gboolean video_encoder;
    gboolean video_quality;
    gboolean video_bitrate;
    gboolean video_framerate;
    gboolean video_framerate_control;
    gboolean video_turbo;
    gboolean video_two_pass;
};

struct config {
    struct config_keys key;
    struct config_set set;
};

GKeyFile *parse_key_file(char *infile);
struct config get_local_config(GKeyFile *key_file);
struct config get_global_config(GKeyFile *key_file);
struct config empty_config();
void free_config(struct config c);
gchar* get_key_value_string(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gboolean *set);
gboolean get_key_value_boolean(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gboolean *set);
gint get_key_value_integer(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gboolean *set);
gdouble get_key_value_double(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gboolean *set);
gchar ** get_key_value_string_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gsize *length, gboolean *set);
gboolean * get_key_value_boolean_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gsize *length, gboolean *set);
gint * get_key_value_integer_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gsize *length, gboolean *set);
gdouble *get_key_value_double_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gsize *length, gboolean *set);

void set_key_value_string(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gchar* value, gboolean *set);
void set_key_value_boolean(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gboolean value, gboolean *set);
void set_key_value_integer(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gint value, gboolean *set);
void set_key_value_double(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gdouble value, gboolean *set);
void set_key_value_string_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gchar **values, gsize *length, gboolean *set);
void set_key_value_boolean_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gboolean *values, gsize *length, gboolean *set);
void set_key_value_integer_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gint *values, gsize *length, gboolean *set);
void set_key_value_double_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gdouble *values, gsize *length, gboolean *set);

int outfile_count(void);
int get_outfile_from_episode(int episode_number);

#endif
