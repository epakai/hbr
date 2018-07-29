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

#include "keyfile.h"
#include <glib.h>
#include <glib/gprintf.h>


/**
 * @brief Parses the xml file. Validates xml against the DTD.
 *
 * @param infile path for xml file.
 *
 * @return GKeyfile pointer. NULL on failure. Must be freed by caller.
 */
GKeyFile* parse_key_file(char *infile)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GKeyFile) key_file = g_key_file_new ();

    if (!g_key_file_load_from_file (key_file, infile, G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            g_warning ("Error loading key file: %s", error->message);
        if (!g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_PARSE))
            g_warning ("Error parsing key file: %s", error->message);
        if (!g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_UNKNOWN_ENCODING))
            g_warning ("Key file has unknown encoding: %s", error->message);
        return NULL;
    }
    return key_file;
}

struct config get_config(GKeyFile* key_file)
{
    const gchar* group = "CONFIG";
    struct config c;
    //TODO verify CONFIG group exists
    if (!g_key_file_has_group(key_file, group))
    {
        // TODO return empty config with all sets = false
    }
    c.key.markers = get_key_value_boolean(key_file, group, "markers",
            &c.set.markers);
    c.key.autocrop = get_key_value_boolean(key_file, group, "autocrop",
            &c.set.autocrop);
    c.key.loose_crop  = get_key_value_boolean(key_file, group, "loose_crop",
            &c.set.loose_crop);
    c.key.video_turbo = get_key_value_boolean(key_file, group, "video_turbo",
            &c.set.video_turbo);
    c.key.video_two_pass = get_key_value_boolean(key_file, group, "video_two_pass",
            &c.set.video_two_pass);
    c.key.audio_encoder = get_key_value_string(key_file, group, "audio_encoder",
            &c.set.audio_encoder);
    c.key.decomb= get_key_value_string(key_file, group, "decomb", &c.set.decomb);
    c.key.deinterlace= get_key_value_string(key_file, group, "deinterlace",
            &c.set.deinterlace);
    c.key.denoise= get_key_value_string(key_file, group, "denoise", &c.set.denoise);
    c.key.grayscale= get_key_value_string(key_file, group, "grayscale",
            &c.set.grayscale);
    c.key.rotate= get_key_value_string(key_file, group, "rotate", &c.set.rotate);
    c.key.format= get_key_value_string(key_file, group, "format", &c.set.format);
    c.key.anamorphic= get_key_value_string(key_file, group, "anamorphic",
            &c.set.anamorphic);
    c.key.video_encoder = get_key_value_string(key_file, group, "video_encoder",
            &c.set.video_encoder);
    c.key.video_framerate= get_key_value_string(key_file, group, "video_framerate",
            &c.set.video_framerate);
    c.key.video_framerate_control = get_key_value_string(key_file, group,
            "video_framerate_control", &c.set.video_framerate_control);
    c.key.input_basedir = get_key_value_string(key_file, group,
            "input_basedir", &c.set.input_basedir);
    c.key.audio_bitrate = get_key_value_integer(key_file, group, "audio_bitrate",
            &c.set.audio_bitrate );
    c.key.audio_quality = get_key_value_integer(key_file, group, "audio_quality",
            &c.set.audio_quality );
    c.key.video_bitrate = get_key_value_integer(key_file, group, "video_bitrate",
            &c.set.video_bitrate );
    c.key.video_quality = get_key_value_integer(key_file, group, "video_quality",
            &c.set.video_quality );
    return c;
}

void free_config(struct config c)
{
    g_free(c.key.audio_encoder);
    g_free(c.key.decomb);
    g_free(c.key.deinterlace);
    g_free(c.key.denoise);
    g_free(c.key.grayscale);
    g_free(c.key.rotate);
    g_free(c.key.format);
    g_free(c.key.anamorphic);
    g_free(c.key.video_encoder);
    g_free(c.key.video_framerate);
    g_free(c.key.video_framerate_control);
}

gchar* get_key_value_string(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    // TODO
}
gboolean get_key_value_boolean(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    // TODO
}
gint get_key_value_integer(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    // TODO
}
gdouble get_key_value_double(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    // TODO
}

gchar** get_key_value_string_list(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    // TODO
}
gboolean* get_key_value_boolean_list(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    // TODO
}
gint* get_key_value_integer_list(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    // TODO
}
gdouble* get_key_value_double_list(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    // TODO
}

/**
 * @brief Counts number of outfile sections
 *
 * @return Number of outfile sections
 */
int outfile_count()
{
    int count = 0;
    // TODO
	return count;
}

/**
 * @brief Find first outfile with matching episode number.
 *
 * @param episode_number Episode number to match.
 *
 * @return Index of the first matching outfile. Returns -1 if no match.
 */
int get_outfile_from_episode(int episode_number)
{
    // TODO
}
