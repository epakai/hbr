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
    gchar*   format;
    gboolean markers;
    gchar*   input_basedir; // only used for local configs

    gchar*   picture_anamorphic;
    gboolean picture_autocrop;
    gint     picture_loose_crop;
    gint     picture_crop_top;
    gint     picture_crop_bottom;
    gint     picture_crop_left;
    gint     picture_crop_right;

    gchar*   filter_decomb;
    gchar*   filter_deinterlace;
    gchar*   filter_denoise;
    gchar*   filter_grayscale;
    gchar**  filter_rotate;
    gsize    filter_rotate_count;

    gchar*   audio_encoder;
    gint     audio_quality;
    gint     audio_bitrate;

    gchar*   video_encoder;
    gint     video_quality;
    gint     video_bitrate;
    gchar*   video_framerate;
    gchar*   video_framerate_control;
    gboolean video_turbo;
    gboolean video_two_pass;
};

struct config_set {
    gboolean format;
    gboolean markers;
    gboolean input_basedir; // only used for local configs

    gboolean picture_anamorphic;
    gboolean picture_autocrop;
    gboolean picture_loose_crop;
    gboolean picture_crop_top;
    gboolean picture_crop_bottom;
    gboolean picture_crop_left;
    gboolean picture_crop_right;

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

struct outfile_keys {
    gchar* type;
    gchar* iso_filename;
    gint   dvdtitle;
    gchar* name;
    gchar* year;
    gint   season;
    gint   episode_number;
    gchar* specific_name;
    gint   crop_top;
    gint   crop_bottom;
    gint   crop_left;
    gint   crop_right;
    gint   chapters_start;
    gint   chapters_end;
    gint*  audio;
    gsize  audio_count;
    gint*  subtitle;
    gsize  subtitle_count;
};

struct outfile_set {
    gboolean type;
    gboolean iso_filename;
    gboolean dvdtitle;
    gboolean name;
    gboolean year;
    gboolean season;
    gboolean episode_number;
    gboolean specific_name;
    gboolean crop_top;
    gboolean crop_bottom;
    gboolean crop_left;
    gboolean crop_right;
    gboolean chapters_start;
    gboolean chapters_end;
    gboolean audio;
    gboolean subtitle;
};

struct outfile {
    struct outfile_keys key;
    struct outfile_set set;
};

GKeyFile * parse_key_file(char *infile);
GKeyFile * generate_key_file(struct config config);
struct config get_local_config(GKeyFile *keyfile);
struct config get_global_config(GKeyFile *keyfile);
struct config empty_config();
struct config default_config();
struct config merge_configs(struct config pref, struct config alt);
void free_config(struct config c);

gint get_outfile_count(GKeyFile *keyfile);
gchar ** get_outfile_list(GKeyFile *keyfile, gsize *outfile_count);
gchar * get_group_from_episode(GKeyFile *keyfile, int episode_number);
struct outfile get_outfile_from_episode(GKeyFile *keyfile, int episode_number);
struct outfile empty_outfile();
struct outfile get_outfile(GKeyFile *keyfile, gchar *group);
void free_outfile(struct outfile o);

gchar * get_key_value_string(GKeyFile *keyfile, const gchar* group_name,
        const gchar* key, gboolean *set);
gboolean get_key_value_boolean(GKeyFile *keyfile, const gchar* group_name,
        const gchar* key, gboolean *set);
gint get_key_value_integer(GKeyFile *keyfile, const gchar* group_name,
        const gchar* key, gboolean *set);
gdouble get_key_value_double(GKeyFile *keyfile, const gchar* group_name,
        const gchar* key, gboolean *set);
gchar ** get_key_value_string_list(GKeyFile *keyfile, const gchar* group_name,
        const gchar* key, gsize *length, gboolean *set);
gboolean * get_key_value_boolean_list(GKeyFile *keyfile, const gchar* group_name,
        const gchar* key, gsize *length, gboolean *set);
gint * get_key_value_integer_list(GKeyFile *keyfile, const gchar* group_name,
        const gchar* key, gsize *length, gboolean *set);
gdouble *get_key_value_double_list(GKeyFile *keyfile, const gchar* group_name,
        const gchar* key, gsize *length, gboolean *set);
#endif
