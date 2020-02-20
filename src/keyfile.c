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

#include <string.h>
#include <stdio.h>

#include "util.h"
#include "keyfile.h"
#include "validate.h"

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
    gboolean valid = TRUE;
    GKeyFile *keyfile = NULL;
    // check file is readable and does not have duplicate group names
    if (!pre_validate_key_file(infile)) {
        valid = FALSE;
    } else {
        keyfile = parse_key_file(infile);

        if (config == NULL) {
            // validate a global config
            if (!post_validate_config_file(keyfile, infile)) {
                valid = FALSE;
            }
        } else {
            // validate an input file
            if (!post_validate_input_file(keyfile, infile, config)) {
                valid = FALSE;
            }
        }
    }
    if (valid && keyfile) {
        return keyfile;
    } else if (keyfile){
        g_key_file_free(keyfile);
        return NULL;
    } else {
        return NULL;
    }
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
        if (g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            hbr_error("Error loading file", infile, NULL, NULL, NULL);
        } else if (g_error_matches (error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_PARSE)) {
            hbr_error("Error parsing file", infile, NULL, NULL, NULL);
        } else if (g_error_matches (error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_UNKNOWN_ENCODING)) {
            hbr_error("File has unknown encoding", infile, NULL, NULL, NULL);
        }
        g_error_free(error);
        g_key_file_free(keyfile);
        return NULL;
    }
    g_key_file_set_list_separator(keyfile, ',');
    return keyfile;
}

/**
 * @brief Creates a new keyfile with group new_group that is a copy of
 *        keyfile's group
 *
 * @param keyfile   keyfile containing group to be copied
 * @param group     name of group to be copied
 * @param new_group new group name
 *
 * @return new GKeyfile or NULL when group is empty/does not exist
 */
GKeyFile * copy_group_new(GKeyFile *keyfile, gchar *group, gchar *new_group)
{
    GKeyFile *k = g_key_file_new();
    g_key_file_set_list_separator(k, ',');

    // check group exists
    if (!g_key_file_has_group(keyfile, group)) {
        g_key_file_free(k);
        return NULL;
    }

    // get a list of keys
    gsize key_count;
    gchar **key_list = g_key_file_get_keys(keyfile, group, &key_count, NULL);

    // check group is not empty
    // (cannot make a new empty group via GKeyFile API)
    if (key_count == 0) {
        g_key_file_free(k);
        return NULL;
    }

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
 * @brief Merges two keyfile groups into one. If both groups define a key
 *        then p_group's key values are kept.
 *
 * @param pref      keyfile containing preferred group
 * @param p_group   name of group with preferred key values
 * @param alt       keyfile containing alternate group
 * @param a_group   name of group with alternate key values
 * @param new_group name of the new merged group
 *
 * @return new GKeyfile, NULL when p_group or a_group does not exist, or
 *         NULL when p_group and a_group are both empty
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

    // check if both groups are empty (copy_group_new() returns NULL on empty)
    if (k == NULL && key_count == 0) {
        hbr_error("Failed to merge two empty sections \"%s\" and \"%s\".",
                NULL, NULL, NULL, NULL, p_group, a_group);
        return NULL;
    } else if (k == NULL){
        // make a new keyfile if the copy failed, but pref still has good keys
        k = g_key_file_new();
        g_key_file_set_list_separator(k, ',');
    }

    // copy each key and value to new group in new key file overwriting
    // values from alt
    for (gint i = 0; i < key_count; i++) {
        // check new_group for conflicting options and remove them
        /* TODO FIXME merging or removing conflicts may not be working correctly
         * I forgot the exact issue I was having, but I'm pretty sure there was an issue
         * with remove_conflicts not actually removing conflicts
         */
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
 * @param key              The key we are checking has any conflicts
 * @param modified_keyfile Keyfile to be modified (conflicting keys removed)
 * @param mod_group        Group that the keys are in
 * @param checked_keyfile  Keyfile we actually check against.
 *                         We Check against the original keyfile because we don't
 *                         want to detect conflicts at the same level. Conflicts
 *                         at the same level should produce an error during
 *                         validation instead.
 * @param check_group      Group in the checked_keyfile
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
            // skip keys that don't have the necessary specific to be a conflict
            if (conflict.value != NULL) {
                gchar *value = g_key_file_get_value(modified_keyfile, mod_group,
                        conflict.name, NULL);
                if (!g_strcmp0(value, conflict.value) == 0) {
                    g_free(value);
                    continue;
                }
                g_free(value);
            }
            // check if the conflict value matches
            if (conflict.conflict_value != NULL){
                gchar *value = g_key_file_get_value(checked_keyfile, check_group,
                        conflict.conflict_name, NULL);
                if (strcmp(value, conflict.conflict_value) == 0) {
                    hbr_info("Removed conflicting option", NULL, mod_group,
                            conflict.conflict_name, conflict.conflict_value);
                    g_key_file_remove_key(modified_keyfile, mod_group,
                            conflict.conflict_name, NULL);
                }
                g_free(value);
            } else {
                // no value to match, just remove the key
                hbr_info("Dropping conflicting option in", NULL, mod_group,
                        conflict.conflict_name, conflict.conflict_value);
                g_key_file_remove_key(modified_keyfile, mod_group,
                        conflict.conflict_name, NULL);
            }
        }
    } while ((conflict_indexes = g_slist_next(conflict_indexes)));
}

/**
 * @brief Generate a config file based on HandBrake's "High Profile"
 */
GKeyFile * generate_default_key_file(void)
{
    GKeyFile *k = g_key_file_new();
    g_key_file_set_list_separator(k, ',');
    const gchar *g = "CONFIG";
    g_key_file_set_comment(k, NULL, NULL,
            " hbr (handbrake runner) config file\n"
            " Options follow the naming from HandBrakeCLI --help\n", NULL);
    /*
     * These options are based on "CLI Default" preset in HandBrake 1.2.2
     */
    g_key_file_set_string(k, g, "audio-copy-mask",
            "copy:aac,copy:ac3,copy:eac3,copy:dtshd,copy:dts,copy:mp3,"
            "copy:truehd,copy:flac");
    g_key_file_set_string(k, g, "audio-fallback", "av_aac");
    g_key_file_set_integer(k, g, "ab", 128);
    g_key_file_set_double(k, g, "ac", -1.0);
    g_key_file_set_string(k, g, "adither", "auto");
    g_key_file_set_string(k, g, "aencoder", "av_aac");
    g_key_file_set_string(k, g, "mixdown", "dpl2");
    g_key_file_set_string(k, g, "arate", "auto");
    g_key_file_set_double(k, g, "gain", 0.0);
    g_key_file_set_boolean(k, g, "markers", TRUE);
    g_key_file_set_string(k, g, "format", "av_mp4");
    g_key_file_set_boolean(k, g, "crop", TRUE);
    g_key_file_set_string(k, g, "deblock", "qp=0:mode=2");
    g_key_file_set_boolean(k, g, "itu-par", FALSE);
    g_key_file_set_boolean(k, g, "keep-display-aspect", FALSE);
    g_key_file_set_integer(k, g, "modulus", 2);
    g_key_file_set_boolean(k, g, "loose-anamorphic", TRUE);
    g_key_file_set_integer(k, g, "height", 720);
    g_key_file_set_integer(k, g, "width", 853);
    g_key_file_set_string(k, g, "rotate", "angle=0:hflip=0");
    g_key_file_set_integer(k, g, "vb", 6000);
    g_key_file_set_string(k, g, "encoder", "x264");
    g_key_file_set_boolean(k, g, "vfr", TRUE);
    g_key_file_set_string(k, g, "encoder-preset", "medium");
    g_key_file_set_string(k, g, "encoder-profile", "auto");
    g_key_file_set_double(k, g, "quality", 22.0);
    return k;
}

/**
 * @brief Counts number of outfile sections
 *
 * @param keyfile Keyfile to count outfile sections in
 *
 * @return Number of outfile sections (0 or greater)
 */
gint get_outfile_count(GKeyFile *keyfile)
{
    gsize count = 0;
    gsize len = 0;
    gchar **groups = g_key_file_get_groups(keyfile, &len);
    for (gint i = 0; i < len; i++) {
        if (strncmp("OUTFILE", groups[i], sizeof("OUTFILE")-1) == 0) {
            count++;
        }
    }
    g_strfreev(groups);
    return count;
}

/**
 * @brief Get a list of outfile names in an array of strings
 *
 * @param keyfile keyfile to check
 * @param outfile_count output parameter for number of outfile sections found
 *
 * @return list of outfile section names
 */
gchar ** get_outfile_list(GKeyFile *keyfile, gsize *outfile_count)
{
    gsize count = 0;
    // Fetch all groups
    gchar **groups = g_key_file_get_groups(keyfile, &count);
    gchar **filter_groups;
    *outfile_count = get_outfile_count(keyfile);
    // allocate extra pointer for NULL terminated array
    /*
     * g_malloc0() is used because clang seems to think some filter_groups
     * values may be uninitialized. I think this is due to the conditional
     * in the following for loop. I could not think of any set of inputs
     * that actually causes uninitialized values so I chose to zero the
     * memory allocation to stop that warning.
     */
    filter_groups = g_malloc0(sizeof(gchar*)*(*outfile_count+1));
    // Copy only outfile groups to new list
    for (int i = 0, j = 0; i < count; i++) {
        if (strncmp("OUTFILE", groups[i], sizeof("OUTFILE")-1) == 0) {
            filter_groups[j] = g_strdup(groups[i]);
            j++;
        }
    }
    g_strfreev(groups);
    // NULL terminate string array so g_strfreev can free it later
    filter_groups[*outfile_count] = NULL;
    return filter_groups;
}

/**
 * @brief Find a group with matching episode number.
 *
 * @param keyfile keyfile to search
 * @param episode episode number to match
 *
 * @return first group name with a matching episode number
 */
gchar * get_group_from_episode(GKeyFile *keyfile, int episode)
{
    gchar *group;
    gsize count = 0;
    gchar **groups = get_outfile_list(keyfile, &count);
    for (int i = 0; i < count; i++) {
        if ( episode ==
                g_key_file_get_integer(keyfile, groups[i], "episode", NULL)) {
            group = g_strdup(groups[i]);
            g_strfreev(groups);
            return group;
        }
    }
    g_strfreev(groups);
    return NULL;
}
