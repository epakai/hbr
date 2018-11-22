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
#include "build_args.h"
#include <string.h>
#include <stdio.h>


/**
 * @brief Parses and validates the key value file.
 *
 * @param infile path for key value file.
 * @param config Global config used for validating input files.
 *               If config is NULL then infile is treated as a global config.
 *
 * @return GKeyfile pointer. NULL on failure. Must be freed by caller.
 */
GKeyFile * parse_validate_key_file(char *infile, GKeyFile *config)
{
    if (!pre_validate_key_file(infile)) {
        return NULL;
    }
    GKeyFile *keyfile = parse_key_file(infile);

    if (config == NULL) {
        // validate a global config
        if (!post_validate_config_file(keyfile)) {
            return NULL;
        }
    } else {
        // validate an input file
        if (!post_validate_input_file(keyfile, config)) {
            return NULL;
        }
    }
    return keyfile;
}


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
        if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            fprintf(stderr, "Error loading file (%s): %s\n", infile, error->message);
        } else if (!g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_PARSE)) {
            fprintf(stderr, "Error parsing file (%s): %s\n", infile, error->message);
        } else if (!g_error_matches (error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_UNKNOWN_ENCODING)) {
            fprintf(stderr, "File has unknown encoding (%s): %s\n", infile, error->message);
        }
        g_error_free(error);
        g_key_file_free(keyfile);
        return NULL;
    }
    g_key_file_set_list_separator(keyfile, ',');
    return keyfile;
}

/**
 * @brief
 *
 * @param keyfile
 * @param group
 * @param new_group
 *
 * @return new GKeyfile or NULL when group does not exist
 */
GKeyFile * copy_group_new(GKeyFile *keyfile, gchar *group, gchar *new_group)
{
    GKeyFile *k = g_key_file_new();
    g_key_file_set_list_separator(k, ',');

    // check group exists
    if (!g_key_file_has_group(keyfile, group)) {
        return NULL;
    }

    // get a list of keys
    gsize key_count;
    gchar **key_list = g_key_file_get_keys(keyfile, group, &key_count, NULL);

    // copy each key and value to new group in new key file
    for (gint i = 0; i < key_count; i++) {
        gchar *temp = g_key_file_get_value(keyfile, group, key_list[i], NULL);
        g_key_file_set_value(k, new_group, key_list[i], temp);
        g_free(temp);
    }
    g_strfreev(key_list);
    return k;
}

/**
 * @brief
 *
 * @param pref
 * @param p_group
 * @param alt
 * @param a_group
 *
 * @return new GKeyfile or NULL when p_group or a_group does not exist
 */
GKeyFile * merge_key_group(GKeyFile *pref, gchar *p_group, GKeyFile *alt,
        gchar *a_group, gchar *new_group)
{
    // check groups exist
    if (!g_key_file_has_group(pref, p_group) ||
            !g_key_file_has_group(alt, a_group)) {
        return NULL;
    }

    // create new group with all a_group's keys under new_group
    GKeyFile *k = copy_group_new(alt, a_group, new_group);

    // get a list of keys from pref
    gsize key_count;
    gchar **key_list = g_key_file_get_keys(pref, p_group, &key_count, NULL);

    // copy each key and value to new group in new key file overwriting
    // values from alt
    for (gint i = 0; i < key_count; i++) {
        // check new_group for conflicting options and remove them
        remove_conflicts(key_list[i], k, new_group, alt, a_group);
        gchar *temp = g_key_file_get_value(pref, p_group, key_list[i], NULL);
        g_key_file_set_value(k, new_group, key_list[i], temp);
        g_free(temp);
    }
    g_strfreev(key_list);
    return k;
}

/**
 * @brief Removes keys from modified_keyfile that conflict with key.
 *        This enables us to override conflicting options.
 *
 * @param key The key we are checking has any conflicts
 * @param modified_keyfile Keyfile to be modified (conflicting keys removed)
 * @param mod_group Group that the keys are in
 * @param checked_keyfile Keyfile we actually check against.
 *                        We Check against the original keyfile because we don't
 *                        want to detect conflicts at the same level. Conflicts
 *                        at the same level should produce an error during
 *                        validation instead.
 * @param check_group Group in the checked_keyfile
 */
void remove_conflicts(gchar *key, GKeyFile *modified_keyfile, gchar *mod_group,
        GKeyFile *checked_keyfile, gchar* check_group)
{
    // TODO this function doesn't remove negation conflicts (--markers --no-markers)
    // get list of indexes for conflicts
    GSList *conflict_indexes = g_hash_table_lookup(conflicts_index, key);
    // return when no conflicts found
    if (conflict_indexes == NULL) {
        return;
    }
    // iterate for each possible conflict
    do {
        conflict_t conflict = conflicts[GPOINTER_TO_INT(conflict_indexes->data)];
        // check if the conflict value exists
        if (g_key_file_has_key(checked_keyfile, check_group,
                    conflict.conflict_name, NULL)) {
            // check if the conflict value matches
            if (conflict.conflict_value != NULL){
                gchar *value = g_key_file_get_value(checked_keyfile, check_group,
                        conflict.conflict_name, NULL);
                if (strcmp(value, conflict.conflict_value) == 0) {
                    g_key_file_remove_key(modified_keyfile, mod_group,
                            conflict.conflict_name, NULL);
                }
                g_free(value);
            } else {
                // no value to match, just remove the key
                g_key_file_remove_key(modified_keyfile, mod_group,
                        conflict.conflict_name, NULL);
            }
        }
    } while ((conflict_indexes = g_slist_next(conflict_indexes)));
}

/**
 * @brief Generate a config file based on HandBrake's Normal profile
 *
 * @param c
 *
 * @return
 */
GKeyFile * generate_default_key_file()
{
    GKeyFile *k = g_key_file_new();
    g_key_file_set_list_separator(k, ',');
    const gchar *g = "CONFIG";
    //TODO update this comment for new style
    g_key_file_set_comment(k, NULL, NULL,
            " hbr (handbrake runner) config file\n"
            " Options follow the naming from HandBrakeCLI --help\n", NULL);

    g_key_file_set_string(k, g, "format", "mp4");
    g_key_file_set_string(k, g, "quality", "20.0");
    g_key_file_set_string(k, g, "x264-preset", "veryfast");
    g_key_file_set_string(k, g, "h264-profile", "main");
    g_key_file_set_string(k, g, "h264-level", "4.0");

    g_key_file_set_boolean(k, g, "markers", TRUE);
    g_key_file_set_boolean(k, g, "loose-anamorphic", TRUE);
    g_key_file_set_string(k, g, "modulus", "2");

    g_key_file_set_string(k, g, "aencoder", "ffaac");
    g_key_file_set_string(k, g, "ab", "160");
    g_key_file_set_string(k, g, "encoder", "x264");
    g_key_file_set_string(k, g, "mixdown", "dpl2");
    g_key_file_set_string(k, g, "arate", "Auto");
    g_key_file_set_string(k, g, "drc", "0.0");
    g_key_file_set_string(k, g, "audio-copy-mask", "aac,ac3,dtshd,dts,mp3");
    g_key_file_set_string(k, g, "audio-fallback", "ffac3");

    return k;
}

/**
 * @brief Counts number of outfile sections
 *
 * @return Number of outfile sections
 */
gint get_outfile_count(GKeyFile *keyfile)
{
    gsize count = 0;
    gsize len = 0;
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
    gsize count = 0;
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
        if ( episode_number == g_key_file_get_integer(keyfile, groups[i],
                    "episode_number", NULL)) {
            group = g_strdup(groups[i]);
            g_strfreev(groups);
            return group;
        }
    }
    g_strfreev(groups);
    return NULL;
}

/**
 * @brief TODO
 *
 * @param infile filename to be checked
 *
 * @return
 */
gboolean pre_validate_key_file(gchar *infile)
{
    // check and read file
    GDataInputStream * datastream = open_datastream(infile);
    if (!datastream || has_duplicate_groups(datastream, infile)) {
        g_input_stream_close((GInputStream *)datastream, NULL, NULL);
        g_object_unref(datastream);
        return FALSE;
    }
    g_input_stream_close((GInputStream *)datastream, NULL, NULL);
    g_object_unref(datastream);
    return TRUE;
}

gboolean post_validate_input_file(GKeyFile *input_keyfile, GKeyFile *config_keyfile)
{
    /* POST_PARSING CHECKS */
    // check CONFIG exists
    // check for unknown sections
    // check for unknown keys
    // check required keys exist (independent)
    // check required keys exist (dependent)
    // check known keys values (independent)
    // check known keys values (dependent)
    // check depends
    // check conflicts
    // check negation type conflicts

    /* NOT GLOBAL CONFIG */
    // check at least one OUTFILE exists
    // check required keys exist (type, file naming stuff)
    // check required keys exist (depends)
    // check known keys values (independent)
    // check known keys values (dependent)
    return TRUE;
}

gboolean post_validate_config_file(GKeyFile *keyfile)
{
    /* POST_PARSING CHECKS */
    // check only CONFIG exists
    // check for unknown keys
    // check required keys exist (maybe none)
    // check required keys exist (dependent)
    // check known keys values (independent)
    // check known keys values (dependent)
    // check depends
    // check conflicts
    // check negation type conflicts
    return TRUE;
}

GDataInputStream * open_datastream(gchar *infile)
{
    GFile *file = g_file_new_for_path(infile);
    GError *error = NULL;
    GFileInputStream *filestream = g_file_read(file, NULL, &error);
    g_object_unref(file);
    if (error != NULL) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
            fprintf(stderr, "Error file not found (%s): %s\n",
                    infile, error->message);
        } else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY)) {
            fprintf(stderr, "Error file is a directory (%s): %s\n",
                    infile, error->message);
        } else {
            fprintf(stderr, "Error reading file (%s): %s\n",
                    infile, error->message);
        }
        g_error_free(error);
        return NULL;
    }
    GDataInputStream *datastream = g_data_input_stream_new((GInputStream *) filestream);
    g_object_unref(filestream);
    return datastream;
}

gboolean has_duplicate_groups(GDataInputStream *datastream, gchar *infile)
{
    // check for duplicate group sections
    gchar *line;
    gsize length;
    gint line_count = 0;
    GHashTable *group_hashes = g_hash_table_new_full(g_str_hash, g_str_equal,
            g_free, NULL);
    // parse each line
    while ((line = g_data_input_stream_read_line_utf8(datastream, &length, NULL,
                    NULL))) {
        line_count++;
        // get group name
        g_strchug(line); // remove leading whitespace
        const gchar *group_name_start = NULL;
        const gchar *group_name_end = line+length-1;
        if (line[0] == '[') {
            group_name_start = line+1;
            while (*group_name_end != ']' && group_name_end > group_name_start) {
                group_name_end--;
            }
        } else {
            g_free(line);
            continue;
        }
        gchar *group_name = g_strndup (group_name_start,
                group_name_end - group_name_start);
        g_free(line);
        // check for a duplicate hash of current group
        if (g_hash_table_contains(group_hashes, group_name)) {
            fprintf(stderr, "Config file (%s) contains duplicate group "
                    "\"[%s]\" at line %d\n", infile, group_name, line_count);
            g_hash_table_destroy(group_hashes);
            g_free(group_name);
            return TRUE;
        } else {
            g_hash_table_add(group_hashes, group_name);
        }
    }
    g_hash_table_destroy(group_hashes);
    return FALSE;
}
