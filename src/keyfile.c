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
 * @brief Parses the key value file.
 *
 * @param infile path for key value file.
 *
 * @return GKeyfile pointer. NULL on failure. Must be freed by caller.
 */
GKeyFile* parse_key_file(char *infile)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GKeyFile) key_file = g_key_file_new ();

    if (!g_key_file_load_from_file (key_file, infile,
                G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            g_warning ("Error loading file: %s", error->message);
        if (!g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_PARSE))
            g_warning ("Error parsing file: %s", error->message);
        if (!g_error_matches (error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_UNKNOWN_ENCODING))
            g_warning ("File has unknown encoding: %s", error->message);
        return NULL;
    }
    return key_file;
}

struct config get_local_config(GKeyFile* key_file)
{
    const gchar* group = "CONFIG";
    //verify CONFIG group exists in file
    if (!g_key_file_has_group(key_file, group))
    {
        return empty_config();
    }
    struct config c = get_global_config(key_file);

    c.key.input_basedir = get_key_value_string(key_file, group,
            "input_basedir", &c.set.input_basedir);
    return c;
}

struct config get_global_config(GKeyFile* key_file)
{
    const gchar* group = "CONFIG";
    struct config c;
    //verify CONFIG group exists in file
    if (!g_key_file_has_group(key_file, group))
    {
        return empty_config();
    }
    c.key.format= get_key_value_string(key_file, group,
            "format", &c.set.format);
    c.key.markers = get_key_value_boolean(key_file, group,
            "markers", &c.set.markers);

    c.key.picture_anamorphic= get_key_value_string(key_file, group,
            "picture_anamorphic", &c.set.picture_anamorphic);
    c.key.picture_autocrop = get_key_value_boolean(key_file, group,
            "picture_autocrop", &c.set.picture_autocrop);
    c.key.picture_loose_crop = get_key_value_integer(key_file, group,
            "picture_loose_crop", &c.set.picture_loose_crop);

    c.key.filter_decomb = get_key_value_string(key_file, group,
            "filter_decomb", &c.set.filter_decomb);
    c.key.filter_deinterlace = get_key_value_string(key_file, group,
            "filter_deinterlace", &c.set.filter_deinterlace);
    c.key.filter_denoise= get_key_value_string(key_file, group,
            "filter_denoise", &c.set.filter_denoise);
    c.key.filter_grayscale= get_key_value_string(key_file, group,
            "filter_grayscale", &c.set.filter_grayscale);
    c.key.filter_rotate= get_key_value_string(key_file, group,
            "filter_rotate", &c.set.filter_rotate);
    
    c.key.audio_encoder = get_key_value_string(key_file, group,
            "audio_encoder", &c.set.audio_encoder);
    c.key.audio_quality = get_key_value_integer(key_file, group,
            "audio_quality", &c.set.audio_quality );
    c.key.audio_bitrate = get_key_value_integer(key_file, group,
            "audio_bitrate", &c.set.audio_bitrate );

    c.key.video_encoder = get_key_value_string(key_file, group,
            "video_encoder", &c.set.video_encoder);
    c.key.video_quality = get_key_value_integer(key_file, group,
            "video_quality", &c.set.video_quality );
    c.key.video_bitrate = get_key_value_integer(key_file, group,
            "video_bitrate", &c.set.video_bitrate );
    c.key.video_framerate= get_key_value_string(key_file, group,
            "video_framerate", &c.set.video_framerate);
    c.key.video_framerate_control = get_key_value_string(key_file, group,
            "video_framerate_control", &c.set.video_framerate_control);
    c.key.video_turbo = get_key_value_boolean(key_file, group,
            "video_turbo", &c.set.video_turbo);
    c.key.video_two_pass = get_key_value_boolean(key_file, group,
            "video_two_pass", &c.set.video_two_pass);
    return c;
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
    c.set.format                    =   FALSE;
    c.set.markers                   =   FALSE;
    c.set.input_basedir             =   FALSE;

    c.set.picture_anamorphic        =   FALSE;
    c.set.picture_autocrop          =   FALSE;
    c.set.picture_loose_crop        =   FALSE;

    c.set.filter_decomb             =   FALSE;
    c.set.filter_deinterlace        =   FALSE;
    c.set.filter_denoise            =   FALSE;
    c.set.filter_grayscale          =   FALSE;
    c.set.filter_rotate             =   FALSE;

    c.set.audio_encoder             =   FALSE;
    c.set.audio_quality             =   FALSE;
    c.set.audio_bitrate             =   FALSE;

    c.set.video_encoder             =   FALSE;
    c.set.video_quality             =   FALSE;
    c.set.video_bitrate             =   FALSE;
    c.set.video_framerate           =   FALSE;
    c.set.video_framerate_control   =   FALSE;
    c.set.video_turbo               =   FALSE;
    c.set.video_two_pass            =   FALSE;
    return c;
}

void free_config(struct config c)
{
    // free all potentially allocated strings
    if (c.set.format)
        g_free(c.key.format);
    if (c.set.audio_encoder)
        g_free(c.key.audio_encoder);
    if (c.set.filter_decomb)
        g_free(c.key.filter_decomb);
    if (c.set.filter_deinterlace)
        g_free(c.key.filter_deinterlace);
    if (c.set.filter_denoise)
        g_free(c.key.filter_denoise);
    if (c.set.filter_grayscale)
        g_free(c.key.filter_grayscale);
    if (c.set.filter_rotate)
        g_free(c.key.filter_rotate);
    if (c.set.picture_anamorphic)
        g_free(c.key.picture_anamorphic);
    if (c.set.video_encoder)
        g_free(c.key.video_encoder);
    if (c.set.video_framerate)
        g_free(c.key.video_framerate);
    if (c.set.video_framerate_control)
        g_free(c.key.video_framerate_control);
}


/**
 * @brief Get key value of type string
 *
 * @param key_file Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return string value or NULL if not found
 */
gchar* get_key_value_string(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    g_autoptr(GError) error = NULL;
    gchar *val = g_key_file_get_string (key_file, group_name, key, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    }
    return val;
}

/**
 * @brief Get key value of type boolean
 *
 * @param key_file Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return boolean value or NULL if not found
 */
gboolean get_key_value_boolean(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    g_autoptr(GError) error = NULL;
    gboolean val = g_key_file_get_boolean(key_file, group_name, key, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    }
    return val;
}

/**
 * @brief Get key value of type integer
 *
 * @param key_file Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return integer value or NULL if not found
 */
gint get_key_value_integer(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    g_autoptr(GError) error = NULL;
    gint val = g_key_file_get_integer(key_file, group_name, key, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    }
    return val;

}

/**
 * @brief Get key value of type double
 *
 * @param key_file Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return double value or NULL if not found
 */
gdouble get_key_value_double(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    g_autoptr(GError) error = NULL;
    gdouble val = g_key_file_get_double(key_file, group_name, key, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    }
    return val;
}

/**
 * @brief Get key value of type string list
 *
 * @param key_file Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return string list value or NULL if not found
 */
gchar** get_key_value_string_list(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    g_autoptr(GError) error = NULL;
    gchar** val = g_key_file_get_string_list(key_file, group_name, key, length, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    }
    return val;
}

/**
 * @brief Get key value of type gboolean list
 *
 * @param key_file Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return gboolean list value or NULL if not found
 */
gboolean* get_key_value_boolean_list(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    g_autoptr(GError) error = NULL;
    gboolean* val = g_key_file_get_boolean_list(key_file, group_name, key, length, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    }
    return val;
}

/**
 * @brief Get key value of type integer list
 *
 * @param key_file Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return integer list value or NULL if not found
 */
gint* get_key_value_integer_list(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    g_autoptr(GError) error = NULL;
    gint* val = g_key_file_get_integer_list(key_file, group_name, key, length, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    }
    return val;
}

/**
 * @brief Get key value of type double list
 *
 * @param key_file Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return double list value or NULL if not found
 */
gdouble* get_key_value_double_list(GKeyFile *key_file, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    g_autoptr(GError) error = NULL;
    gdouble* val = g_key_file_get_double_list(key_file, group_name, key, length, &error);
    if (error == NULL && set != NULL)
    {
        *set = TRUE;
    }
    return val;
}

void set_key_value_string(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gchar* value, gboolean *set)
{
    //TODO
}

void set_key_value_boolean(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gboolean value, gboolean *set)
{
    //TODO
}

void set_key_value_integer(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gint value, gboolean *set)
{
    //TODO
}

void set_key_value_double(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gdouble value, gboolean *set)
{ 
    //TODO
}

void set_key_value_string_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gchar **values, gsize *length, gboolean *set)
{ 
    //TODO
}

void set_key_value_boolean_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gboolean *values, gsize *length, gboolean *set)
{ 
    //TODO
}

void set_key_value_integer_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gint *values, gsize *length, gboolean *set)
{ 
    //TODO
}

void set_key_value_double_list(GKeyFile *key_file, const gchar* group_name,
        const gchar* key, gdouble *values, gsize *length, gboolean *set)
{ 
    //TODO
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
 * @return Outfile number of matching outfile. Returns -1 if no match.
 */
int get_outfile_from_episode(int episode_number)
{
    // TODO

}
