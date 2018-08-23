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
#include <string.h>


/**
 * @brief Parses the key value file.
 *
 * @param infile path for key value file.
 *
 * @return GKeyfile pointer. NULL on failure. Must be freed by caller.
 */
GKeyFile * parse_key_file(char *infile)
{
    GError *error = NULL;
    GKeyFile *keyfile = g_key_file_new();
    if (!g_key_file_load_from_file (keyfile, infile,
                G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            g_warning ("Error loading file: %s", error->message);
        if (!g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_PARSE))
            g_warning ("Error parsing file: %s", error->message);
        if (!g_error_matches (error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_UNKNOWN_ENCODING))
            g_warning ("File has unknown encoding: %s", error->message);
        g_error_free(error);
        g_key_file_free(keyfile);
        return NULL;
    }
    g_key_file_set_list_separator(keyfile, ',');
    return keyfile;
}

/**
 * @brief Generate a config file including comments for all keys
 *
 * @param c
 *
 * @return 
 */
GKeyFile * generate_key_file(struct config c)
{
    GKeyFile *keyfile = g_key_file_new();
    g_key_file_set_list_separator(keyfile, ',');
    g_key_file_set_comment(keyfile, NULL, NULL,
            "# hbr (handbrake runner) config file", NULL);
    // TODO
    return keyfile;
}

/**
 * @brief 
 *
 * @param keyfile
 *
 * @return 
 */
struct config get_local_config(GKeyFile* keyfile)
{
    const gchar group[] = "CONFIG";
    //verify CONFIG group exists in file
    if (!g_key_file_has_group(keyfile, group))
    {
        return empty_config();
    }
    struct config c = get_global_config(keyfile);

    c.key.input_basedir = get_key_value_string(keyfile, group,
            "input_basedir", &c.set.input_basedir);
    return c;
}


/**
 * @brief 
 *
 * @param keyfile
 *
 * @return 
 */
struct config get_global_config(GKeyFile* keyfile)
{
    const gchar group[] = "CONFIG";
    struct config c = empty_config();

    //verify CONFIG group exists in file
    if (!g_key_file_has_group(keyfile, group))
    {
        return c;
    }
    c.key.format = get_key_value_string(keyfile, group,
            "format", &c.set.format);
    c.key.markers = get_key_value_boolean(keyfile, group,
            "markers", &c.set.markers);

    c.key.picture_anamorphic = get_key_value_string(keyfile, group,
            "picture_anamorphic", &c.set.picture_anamorphic);
    c.key.picture_autocrop = get_key_value_boolean(keyfile, group,
            "picture_autocrop", &c.set.picture_autocrop);
    // TODO loose crop defaults to 15 if set, but no value provided
    c.key.picture_loose_crop = get_key_value_integer(keyfile, group,
            "picture_loose_crop", &c.set.picture_loose_crop);
    c.key.picture_crop_top = get_key_value_integer(keyfile, group,
            "picture_crop_top", &c.set.picture_crop_top);
    c.key.picture_crop_bottom = get_key_value_integer(keyfile, group,
            "picture_crop_bottom", &c.set.picture_crop_bottom);
    c.key.picture_crop_left = get_key_value_integer(keyfile, group,
            "picture_crop_left", &c.set.picture_crop_left);
    c.key.picture_crop_right = get_key_value_integer(keyfile, group,
            "picture_crop_right", &c.set.picture_crop_right);

    c.key.filter_decomb = get_key_value_string(keyfile, group,
            "filter_decomb", &c.set.filter_decomb);
    c.key.filter_deinterlace = get_key_value_string(keyfile, group,
            "filter_deinterlace", &c.set.filter_deinterlace);
    c.key.filter_denoise = get_key_value_string(keyfile, group,
            "filter_denoise", &c.set.filter_denoise);
    c.key.filter_grayscale = get_key_value_string(keyfile, group,
            "filter_grayscale", &c.set.filter_grayscale);
    c.key.filter_rotate = get_key_value_string_list(keyfile, group,
            "filter_rotate", &c.key.filter_rotate_count, &c.set.filter_rotate);
    
    c.key.audio_encoder = get_key_value_string(keyfile, group,
            "audio_encoder", &c.set.audio_encoder);
    c.key.audio_quality = get_key_value_integer(keyfile, group,
            "audio_quality", &c.set.audio_quality );
    c.key.audio_bitrate = get_key_value_integer(keyfile, group,
            "audio_bitrate", &c.set.audio_bitrate );

    c.key.video_encoder = get_key_value_string(keyfile, group,
            "video_encoder", &c.set.video_encoder);
    c.key.video_quality = get_key_value_integer(keyfile, group,
            "video_quality", &c.set.video_quality );
    c.key.video_bitrate = get_key_value_integer(keyfile, group,
            "video_bitrate", &c.set.video_bitrate );
    c.key.video_framerate = get_key_value_string(keyfile, group,
            "video_framerate", &c.set.video_framerate);
    c.key.video_framerate_control = get_key_value_string(keyfile, group,
            "video_framerate_control", &c.set.video_framerate_control);
    c.key.video_turbo = get_key_value_boolean(keyfile, group,
            "video_turbo", &c.set.video_turbo);
    c.key.video_two_pass = get_key_value_boolean(keyfile, group,
            "video_two_pass", &c.set.video_two_pass);
    return c;
}

struct config merge_configs(struct config pref, struct config alt)
{
    struct config merged = empty_config();
    if (pref.set.format) {
        merged.set.format = TRUE;
        merged.key.format = g_strdup(pref.key.format);
    } else if (alt.set.format) {
        merged.set.format = TRUE;
        merged.key.format = g_strdup(alt.key.format);
    }
    if (pref.set.markers) {
        merged.set.markers = TRUE;
        merged.key.markers = pref.key.markers;
    } else if (alt.set.markers) {
        merged.set.markers = TRUE;
        merged.key.markers = alt.key.markers;
    }
    if (pref.set.input_basedir) {
        merged.set.input_basedir = TRUE;
        merged.key.input_basedir = g_strdup(pref.key.input_basedir);
    } else if (alt.set.input_basedir) {
        merged.set.input_basedir = TRUE;
        merged.key.input_basedir = g_strdup(alt.key.input_basedir);
    }
    if (pref.set.picture_anamorphic) {
        merged.set.picture_anamorphic = TRUE;
        merged.key.picture_anamorphic = g_strdup(pref.key.picture_anamorphic);
    } else if (alt.set.picture_anamorphic) {
        merged.set.picture_anamorphic = TRUE;
        merged.key.picture_anamorphic = g_strdup(alt.key.picture_anamorphic);
    }
    if (pref.set.picture_autocrop) {
        merged.set.picture_autocrop = TRUE;
        merged.key.picture_autocrop = pref.key.picture_autocrop;
    } else if (alt.set.picture_autocrop) {
        merged.set.picture_autocrop = TRUE;
        merged.key.picture_autocrop = alt.key.picture_autocrop;
    }
    if (pref.set.picture_loose_crop) {
        merged.set.picture_loose_crop = TRUE;
        merged.key.picture_loose_crop = pref.key.picture_loose_crop;
    } else if (alt.set.picture_loose_crop) {
        merged.set.picture_loose_crop = TRUE;
        merged.key.picture_loose_crop = alt.key.picture_loose_crop;
    }
    if (pref.set.picture_crop_top) {
        merged.set.picture_crop_top = TRUE;
        merged.key.picture_crop_top = pref.key.picture_crop_top;
    } else if (alt.set.picture_crop_top) {
        merged.set.picture_crop_top = TRUE;
        merged.key.picture_crop_top = alt.key.picture_crop_top;
    }
    if (pref.set.picture_crop_bottom) {
        merged.set.picture_crop_bottom = TRUE;
        merged.key.picture_crop_bottom = pref.key.picture_crop_bottom;
    } else if (alt.set.picture_crop_bottom) {
        merged.set.picture_crop_bottom = TRUE;
        merged.key.picture_crop_bottom = alt.key.picture_crop_bottom;
    }
    if (pref.set.picture_crop_left) {
        merged.set.picture_crop_left = TRUE;
        merged.key.picture_crop_left = pref.key.picture_crop_left;
    } else if (alt.set.picture_crop_left) {
        merged.set.picture_crop_left = TRUE;
        merged.key.picture_crop_left = alt.key.picture_crop_left;
    }
    if (pref.set.picture_crop_right) {
        merged.set.picture_crop_right = TRUE;
        merged.key.picture_crop_right = pref.key.picture_crop_right;
    } else if (alt.set.picture_crop_right) {
        merged.set.picture_crop_right = TRUE;
        merged.key.picture_crop_right = alt.key.picture_crop_right;
    }
    if (pref.set.filter_decomb) {
        merged.set.filter_decomb = TRUE;
        merged.key.filter_decomb = g_strdup(pref.key.filter_decomb);
    } else if (alt.set.filter_decomb) {
        merged.set.filter_decomb = TRUE;
        merged.key.filter_decomb = g_strdup(alt.key.filter_decomb);
    }
    if (pref.set.filter_deinterlace) {
        merged.set.filter_deinterlace = TRUE;
        merged.key.filter_deinterlace = g_strdup(pref.key.filter_deinterlace);
    } else if (alt.set.filter_deinterlace) {
        merged.set.filter_deinterlace = TRUE;
        merged.key.filter_deinterlace = g_strdup(alt.key.filter_deinterlace);
    }
    if (pref.set.filter_denoise) {
        merged.set.filter_denoise = TRUE;
        merged.key.filter_denoise = g_strdup(pref.key.filter_denoise);
    } else if (alt.set.filter_denoise) {
        merged.set.filter_denoise = TRUE;
        merged.key.filter_denoise = g_strdup(alt.key.filter_denoise);
    }
    if (pref.set.filter_grayscale) {
        merged.set.filter_grayscale = TRUE;
        merged.key.filter_grayscale = g_strdup(pref.key.filter_grayscale);
    } else if (alt.set.filter_grayscale) {
        merged.set.filter_grayscale = TRUE;
        merged.key.filter_grayscale = g_strdup(alt.key.filter_grayscale);
    }
    if (pref.set.filter_rotate) {
        merged.set.filter_rotate = TRUE;
        merged.key.filter_rotate = g_malloc(sizeof(gchar*)*pref.key.filter_rotate_count);
        merged.key.filter_rotate_count = pref.key.filter_rotate_count;
        for (int i = 0; i < pref.key.filter_rotate_count; i++) {
            merged.key.filter_rotate[i] = g_strdup(pref.key.filter_rotate[i]);
        }
    } else if (alt.set.filter_rotate) {
        merged.set.filter_rotate = TRUE;
        merged.key.filter_rotate = g_malloc(sizeof(gchar*)*alt.key.filter_rotate_count);
        merged.key.filter_rotate_count = alt.key.filter_rotate_count;
        for (int i = 0; i < alt.key.filter_rotate_count; i++) {
            merged.key.filter_rotate[i] = g_strdup(alt.key.filter_rotate[i]);
        }
    }
    if (pref.set.audio_encoder) {
        merged.set.audio_encoder = TRUE;
        merged.key.audio_encoder = g_strdup(pref.key.audio_encoder);
    } else if (alt.set.audio_encoder) {
        merged.set.audio_encoder = TRUE;
        merged.key.audio_encoder = g_strdup(alt.key.audio_encoder);
    }
    gint audio_quality;
    if (pref.set.audio_quality) {
        merged.set.audio_quality = TRUE;
        merged.key.audio_quality = pref.key.audio_quality;
    } else if (alt.set.audio_quality) {
        merged.set.audio_quality = TRUE;
        merged.key.audio_quality = alt.key.audio_quality;
    }
    if (pref.set.audio_bitrate) {
        merged.set.audio_bitrate = TRUE;
        merged.key.audio_bitrate = pref.key.audio_bitrate;
    } else if (alt.set.audio_bitrate) {
        merged.set.audio_bitrate = TRUE;
        merged.key.audio_bitrate = alt.key.audio_bitrate;
    }
    if (pref.set.video_encoder) {
        merged.set.video_encoder = TRUE;
        merged.key.video_encoder = g_strdup(pref.key.video_encoder);
    } else if (alt.set.video_encoder) {
        merged.set.video_encoder = TRUE;
        merged.key.video_encoder = g_strdup(alt.key.video_encoder);
    }
    if (pref.set.video_quality) {
        merged.set.video_quality = TRUE;
        merged.key.video_quality = pref.key.video_quality;
    } else if (alt.set.video_quality) {
        merged.set.video_quality = TRUE;
        merged.key.video_quality = alt.key.video_quality;
    }
    if (pref.set.video_bitrate) {
        merged.set.video_bitrate = TRUE;
        merged.key.video_bitrate = pref.key.video_bitrate;
    } else if (alt.set.video_bitrate) {
        merged.set.video_bitrate = TRUE;
        merged.key.video_bitrate = alt.key.video_bitrate;
    }
    if (pref.set.video_framerate) {
        merged.set.video_framerate = TRUE;
        merged.key.video_framerate = g_strdup(pref.key.video_framerate);
    } else if (alt.set.video_framerate) {
        merged.set.video_framerate = TRUE;
        merged.key.video_framerate = g_strdup(alt.key.video_framerate);
    }
    if (pref.set.video_framerate_control) {
        merged.set.video_framerate_control = TRUE;
        merged.key.video_framerate_control = g_strdup(pref.key.video_framerate_control);
    } else if (alt.set.video_framerate_control) {
        merged.set.video_framerate_control = TRUE;
        merged.key.video_framerate_control = g_strdup(alt.key.video_framerate_control);
    }
    if (pref.set.video_turbo) {
        merged.set.video_turbo = TRUE;
        merged.key.video_turbo = pref.key.video_turbo;
    } else if (alt.set.video_turbo) {
        merged.set.video_turbo = TRUE;
        merged.key.video_turbo = alt.key.video_turbo;
    }
    if (pref.set.video_two_pass) {
        merged.set.video_two_pass = TRUE;
        merged.key.video_two_pass = pref.key.video_two_pass;
    } else if (alt.set.video_two_pass) {
        merged.set.video_two_pass = TRUE;
        merged.key.video_two_pass = alt.key.video_two_pass;
    }
    return merged;
}

/**
 * @brief Construct a config with all members unset
 *        Variables are uninitialized. Must always check
 *        if a member is set before accessing it's contents.
 *
 * @return struct config
 */
struct config empty_config()
{
    struct config c;
    c.set.format                  = FALSE;
    c.set.markers                 = FALSE;
    c.set.input_basedir           = FALSE; // local config only

    c.set.picture_anamorphic      = FALSE;
    c.set.picture_autocrop        = FALSE;
    c.set.picture_loose_crop      = FALSE;
    c.set.picture_crop_top        = FALSE; // local config only
    c.set.picture_crop_bottom     = FALSE; // local config only
    c.set.picture_crop_left       = FALSE; // local config only
    c.set.picture_crop_right      = FALSE; // local config only

    c.set.filter_decomb           = FALSE;
    c.set.filter_deinterlace      = FALSE;
    c.set.filter_denoise          = FALSE;
    c.set.filter_grayscale        = FALSE;
    c.set.filter_rotate           = FALSE;

    c.set.audio_encoder           = FALSE;
    c.set.audio_quality           = FALSE;
    c.set.audio_bitrate           = FALSE;

    c.set.video_encoder           = FALSE;
    c.set.video_quality           = FALSE;
    c.set.video_bitrate           = FALSE;
    c.set.video_framerate         = FALSE;
    c.set.video_framerate_control = FALSE;
    c.set.video_turbo             = FALSE;
    c.set.video_two_pass          = FALSE;
    return c;
}

/**
 * @brief Construct a config with defaults set where 
 *        applicable.
 *
 * @return struct config
 */
struct config default_config()
{
    struct config c;
    c.set.format                  = TRUE;
    c.key.format                  = g_strdup("mkv");
    c.set.markers                 = TRUE;
    c.key.markers                 = TRUE;
    c.set.input_basedir           = FALSE; // local config only

    c.set.picture_anamorphic      = TRUE;
    c.key.picture_anamorphic      = g_strdup("off");;
    c.set.picture_autocrop        = TRUE;
    c.key.picture_autocrop        = FALSE;
    c.set.picture_loose_crop      = TRUE;
    c.key.picture_loose_crop      = FALSE;
    c.set.picture_crop_top        = FALSE; // local config only
    c.set.picture_crop_bottom     = FALSE; // local config only
    c.set.picture_crop_left       = FALSE; // local config only
    c.set.picture_crop_right      = FALSE; // local config only

    c.set.filter_decomb           = TRUE;
    c.key.filter_decomb           = g_strdup("none");
    c.set.filter_deinterlace      = TRUE;
    c.key.filter_deinterlace      = g_strdup("none");
    c.set.filter_denoise          = TRUE;
    c.key.filter_denoise          = g_strdup("none");
    c.set.filter_grayscale        = TRUE;
    c.key.filter_grayscale        = FALSE;
    c.set.filter_rotate           = TRUE;
    gchar **one                   = g_malloc(sizeof(gchar *));
    one[0]                        = g_strdup("none");
    c.key.filter_rotate           = one;

    c.set.audio_encoder           = TRUE;
    c.key.audio_encoder           = g_strdup("mp3");
    c.set.audio_quality           = FALSE;
    c.set.audio_bitrate           = TRUE;
    c.key.audio_bitrate           = 160;

    c.set.video_encoder           = TRUE;
    c.key.video_encoder           = g_strdup("x264");
    c.set.video_quality           = TRUE;
    c.key.video_quality           = 20;
    c.set.video_bitrate           = FALSE;
    c.set.video_framerate         = TRUE;
    c.key.video_framerate         = g_strdup("source");
    c.set.video_framerate_control = TRUE;
    c.key.video_framerate_control = g_strdup("variable");
    c.set.video_turbo             = TRUE;
    c.key.video_turbo             = FALSE;
    c.set.video_two_pass          = TRUE;
    c.key.video_two_pass          = FALSE;
    return c;
}

/**
 * @brief Frees all allocatable members of a struct config
 *
 * @param c
 */
void free_config(struct config c)
{
    // free all potentially allocated strings
    if (c.set.format)
        g_free(c.key.format);
    if (c.set.input_basedir)
        g_free(c.key.input_basedir);
    
    if (c.set.picture_anamorphic)
        g_free(c.key.picture_anamorphic);

    if (c.set.filter_decomb)
        g_free(c.key.filter_decomb);
    if (c.set.filter_deinterlace)
        g_free(c.key.filter_deinterlace);
    if (c.set.filter_denoise)
        g_free(c.key.filter_denoise);
    if (c.set.filter_grayscale)
        g_free(c.key.filter_grayscale);
    if (c.set.filter_rotate)
        g_strfreev(c.key.filter_rotate);
    
    if (c.set.audio_encoder)
        g_free(c.key.audio_encoder);
   
    if (c.set.video_encoder)
        g_free(c.key.video_encoder);
    if (c.set.video_framerate)
        g_free(c.key.video_framerate);
    if (c.set.video_framerate_control)
        g_free(c.key.video_framerate_control);
}

/**
 * @brief Counts number of outfile sections
 *
 * @return Number of outfile sections
 */
gint get_outfile_count(GKeyFile *keyfile)
{
    gsize count = 0;
    gsize len;
    gchar **groups = g_key_file_get_groups(keyfile, &len);
    for (gint i = 0; i < len; i++) {
        if (strncmp("OUTFILE", groups[i], 7) == 0) {
            count++;
        }
    }
    g_strfreev(groups);
    return count;
}

/**
 * @brief Get a list of outfile names in a string array
 *
 * @param keyfile
 *
 * @return 
 */
gchar ** get_outfile_list(GKeyFile *keyfile, gsize *outfile_count)
{
    gsize count;
    gchar **groups = g_key_file_get_groups(keyfile, &count);
    gchar **filter_groups;
    *outfile_count = get_outfile_count(keyfile);
    // Copy outfile groups to new list
    filter_groups = g_malloc(sizeof(gchar*)*(*outfile_count+1));
    for (int i = 0, j = 0; i < count; i++) {
        if (strncmp("OUTFILE", groups[i], 7) == 0) {
            filter_groups[j] = g_strdup(groups[i]);
            j++;
        }
    }
    // NULL terminate string array so g_strfreev can free it later
    filter_groups[*outfile_count] = NULL;
    g_strfreev(groups);
    return filter_groups;
}

/**
 * @brief Find a group with matching episode number.
 *
 * @param episode_number Episode number to match.
 *
 * @return 
 */
gchar * get_group_from_episode(GKeyFile *keyfile, int episode_number)
{
    gchar *group;
    gsize count = 0;
    gchar **groups = get_outfile_list(keyfile, &count);
    for (int i = 0; i < count; i++) {
        if ( episode_number == get_key_value_integer(keyfile, groups[i],
                    "episode_number", NULL)) {
            return g_strdup(groups[i]);
        }
    }
    return NULL;
}

/**
 * @brief Find first outfile with matching episode number.
 *
 * @param episode_number Episode number to match.
 *
 * @return struct outfile with episode data. Empty struct outfile if no match
 *         Caller must check episode_number is set to determine.
 */
struct outfile get_outfile_from_episode(GKeyFile *keyfile, int episode_number)
{
    gchar *group;
    gsize count = 0;
    gchar **groups = get_outfile_list(keyfile, &count);
    for (int i = 0; i < count; i++) {
        if ( episode_number == get_key_value_integer(keyfile, groups[i],
                    "episode_number", NULL)) {
            g_strfreev(groups);
            return get_outfile(keyfile, groups[i]);
        }
    }
    g_strfreev(groups);
    return empty_outfile();
}

/**
 * @brief Construct a outfile with all members unset
 *        Variables are uninitialized. Must always check
 *        if a member is set before accessing it's contents.
 *
 * @return struct outfile
 */
struct outfile empty_outfile()
{
    struct outfile o;
    o.set.type           = FALSE;
    o.set.iso_filename   = FALSE;
    o.set.dvdtitle       = FALSE;
    o.set.name           = FALSE;
    o.set.year           = FALSE;
    o.set.season         = FALSE;
    o.set.episode_number = FALSE;
    o.set.specific_name  = FALSE;
    o.set.crop_top       = FALSE;
    o.set.crop_bottom    = FALSE;
    o.set.crop_left      = FALSE;
    o.set.crop_right     = FALSE;
    o.set.chapters_start  = FALSE;
    o.set.chapters_end    = FALSE;
    o.set.audio          = FALSE;
    o.set.subtitle       = FALSE;
    return o;
}

/**
 * @brief Fetch outfile data for a specific group and build an outfile struct
 *
 * @param keyfile
 * @param group
 *
 * @return 
 */
struct outfile get_outfile(GKeyFile *keyfile, gchar *group)
{
    struct outfile o = empty_outfile();
    //verify CONFIG group exists in file
    if (!g_key_file_has_group(keyfile, group))
    {
        return empty_outfile();
    }
    o.key.type = get_key_value_string(keyfile, group,
            "type", &o.set.type);
    o.key.iso_filename = get_key_value_string(keyfile, group,
            "iso_filename", &o.set.iso_filename);
    o.key.dvdtitle = get_key_value_integer(keyfile, group,
            "dvdtitle", &o.set.dvdtitle);
    o.key.name = get_key_value_string(keyfile, group,
            "name", &o.set.name);
    o.key.year = get_key_value_string(keyfile, group,
            "year", &o.set.year);
    o.key.season = get_key_value_integer(keyfile, group,
            "season", &o.set.season);
    o.key.episode_number = get_key_value_integer(keyfile, group,
            "episode_number", &o.set.episode_number);
    o.key.specific_name = get_key_value_string(keyfile, group,
            "specific_name", &o.set.specific_name);
    o.key.crop_top = get_key_value_integer(keyfile, group,
            "crop_top", &o.set.crop_top);
    o.key.crop_bottom = get_key_value_integer(keyfile, group,
            "crop_bottom", &o.set.crop_bottom);
    o.key.crop_left = get_key_value_integer(keyfile, group,
            "crop_left", &o.set.crop_left);
    o.key.crop_right = get_key_value_integer(keyfile, group,
            "crop_right", &o.set.crop_right);
    o.key.chapters_start = get_key_value_integer(keyfile, group,
            "chapters_start", &o.set.chapters_start);
    o.key.chapters_end = get_key_value_integer(keyfile, group,
            "chapters_end", &o.set.chapters_end);
    o.key.audio = get_key_value_integer_list(keyfile, group,
            "audio", &o.key.audio_count, &o.set.audio);
    o.key.subtitle = get_key_value_integer_list(keyfile, group,
            "subtitle", &o.key.subtitle_count, &o.set.subtitle);
    return o;
}

/**
 * @brief Frees all allocatable members of a struct outfile
 *
 * @param o
 */
void free_outfile(struct outfile o)
{
    if (o.set.type)
        g_free(o.key.type);
    if (o.set.iso_filename)
        g_free(o.key.iso_filename);
    if (o.set.name)
        g_free(o.key.name);
    if (o.set.year)
        g_free(o.key.year);
    if (o.set.specific_name)
        g_free(o.key.specific_name);
    if (o.set.audio)
        g_free(o.key.audio);
    if (o.set.subtitle)
        g_free(o.key.subtitle);
}

/**
 * @brief Get key value of type string
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return string value or NULL if not found
 */
gchar* get_key_value_string(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    GError *error = NULL;
    gchar *val = g_key_file_get_string (keyfile, group_name, key, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    } else if (error != NULL) {
        g_error_free(error);
    }
    return val;
}

/**
 * @brief Get key value of type boolean
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return boolean value or NULL if not found
 */
gboolean get_key_value_boolean(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    GError *error = NULL;
    gboolean val = g_key_file_get_boolean(keyfile, group_name, key, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    } else if (error != NULL) {
        g_error_free(error);
    }
    return val;
}

/**
 * @brief Get key value of type integer
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return integer value or NULL if not found
 */
gint get_key_value_integer(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    GError *error = NULL;
    gint val = g_key_file_get_integer(keyfile, group_name, key, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    } else if (error != NULL) {
        g_error_free(error);
    }
    return val;

}

/**
 * @brief Get key value of type double
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return double value or NULL if not found
 */
gdouble get_key_value_double(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    GError *error = NULL;
    gdouble val = g_key_file_get_double(keyfile, group_name, key, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    } else if (error != NULL) {
        g_error_free(error);
    }
    return val;
}

/**
 * @brief Get key value of type string list
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return string list value or NULL if not found
 */
gchar** get_key_value_string_list(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    GError *error = NULL;
    gchar** val = g_key_file_get_string_list(keyfile, group_name, key, length, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    } else if (error != NULL) {
        g_error_free(error);
    }
    return val;
}

/**
 * @brief Get key value of type gboolean list
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return gboolean list value or NULL if not found
 */
gboolean* get_key_value_boolean_list(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    GError *error = NULL;
    gboolean* val = g_key_file_get_boolean_list(keyfile, group_name, key, length, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    } else if (error != NULL) {
        g_error_free(error);
    }
    return val;
}

/**
 * @brief Get key value of type integer list
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return integer list value or NULL if not found
 */
gint* get_key_value_integer_list(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    GError *error = NULL;
    gint* val = g_key_file_get_integer_list(keyfile, group_name, key, length, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    } else if (error != NULL) {
        g_error_free(error);
    }
    return val;
}

/**
 * @brief Get key value of type double list
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return double list value or NULL if not found
 */
gdouble* get_key_value_double_list(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    GError *error = NULL;
    gdouble* val = g_key_file_get_double_list(keyfile, group_name, key, length, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    } else if (error != NULL) {
        g_error_free(error);
    }
    return val;
}
