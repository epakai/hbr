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
    gboolean valid = TRUE;
    // check file is readable and does not have duplicate group names
    if (!pre_validate_key_file(infile)) {
        valid = FALSE;
    }
    
    GKeyFile *keyfile = parse_key_file(infile);

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
    if (valid) {
        return keyfile;
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
        g_printerr("Failed to merge two empty sections \"%s\" and \"%s\".\n",
                p_group, a_group);
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
 * @brief Generate a config file based on HandBrake's "High Profile"
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

    /*
     * These options are based on the "High profile" preset which is
     * taken from 'HandBrakeCLI --preset-list' in version 0.10.5
     */
    // TODO use a newer profile, but this is workable for now

    // -e x264
    g_key_file_set_string(k, g, "encoder", "x264");
    // -q 20.0
    g_key_file_set_string(k, g, "quality", "20.0");
    // -a 1,1
    g_key_file_set_string(k, g, "audio", "1,1");
    // -E ffaac,copy:ac3
    g_key_file_set_string(k, g, "aencoder", "ffaac,copy:ac3");
    // -B 160,160
    g_key_file_set_string(k, g, "ab", "160,160");
    // -6 dpl2,none
    g_key_file_set_string(k, g, "mixdown", "dpl2,none");
    // -R Auto,Auto
    g_key_file_set_string(k, g, "arate", "Auto,Auto");
    // -D 0.0,0.0
    g_key_file_set_string(k, g, "drc", "0.0,0.0");
    // --audio-copy-mask aac,ac3,dtshd,dts,mp3
    g_key_file_set_string(k, g, "audio-copy-mask", "aac,ac3,dtshd,dts,mp3");
    // --audio-fallback ffac3
    g_key_file_set_string(k, g, "audio-fallback", "ffac3");
    // -f mp4
    g_key_file_set_string(k, g, "format", "mp4");
    // --decomb
    g_key_file_set_boolean(k, g, "decomb", TRUE);
    // --loose-anamorphic
    g_key_file_set_boolean(k, g, "loose-anamorphic", TRUE);
    // --modulus 2
    g_key_file_set_string(k, g, "modulus", "2");
    // -m
    g_key_file_set_boolean(k, g, "markers", TRUE);
    // --x264-preset medium
    g_key_file_set_string(k, g, "x264-preset", "medium");
    // --h264-profile high
    g_key_file_set_string(k, g, "h264-profile", "high");
    // --h264-level 4.1
    g_key_file_set_string(k, g, "h264-level", "4.1");
    return k;
}

/**
 * @brief Counts number of outfile sections
 *
 * @param keyfile Keyfile to count outfile sections in
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

/**
 * @brief Check that a file can be read, and no group names are duplicate
 *
 * @param infile filename to be checked
 *
 * @return TRUE when a keyfile is readable, and has no duplicate groups
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
    // TODO add a check for duplicate keys in the same group
    g_input_stream_close((GInputStream *)datastream, NULL, NULL);
    g_object_unref(datastream);
    return TRUE;
}


/**
 * @brief Validates an input keyfile. Checks for valid groups, key names,
 *        key values, key dependencies, and key conflicts.
 *
 * @param input_keyfile  keyfile to be validated
 * @param infile         path to keyfile being validated (for error printing)
 * @param config_keyfile global config keyfile, used to check dependencies of
 *                       the input_keyfile
 *
 * @return TRUE when a keyfile is valid
 */
gboolean post_validate_input_file(GKeyFile *input_keyfile, gchar *infile,
        GKeyFile *config_keyfile)
{
    gboolean valid = TRUE;

    // check CONFIG exists
    if (!g_key_file_has_group(input_keyfile, "CONFIG")) {
        valid = FALSE;
        g_printerr("Keyfile (%s) missing [CONFIG] section.\n", infile);
    }
    // validate config section
    if (!post_validate_common(input_keyfile, infile, config_keyfile)) {
        valid = FALSE; // any errors are printed during post_validate_config_section()
    }

    // check at least one OUTFILE section exists
    if (get_outfile_count(input_keyfile) < 1) {
        valid = FALSE;
        g_printerr("Keyfile (%s) must contain at least one OUTFILE section.\n", infile);
    }

    // check for invalid keys in OUTFILE sections
    gchar ** groups = (g_key_file_get_groups(input_keyfile, NULL));
    int i = 0;
    while (groups[i] != NULL) {
        gchar **keys = g_key_file_get_keys(input_keyfile, groups[i], NULL, NULL);
        int j = 0;
        while (keys[j] != NULL) {
            if (!g_hash_table_contains(options_index, keys[j])) {
                g_printerr("Invalid key \"%s\" in section \"%s\" in file \"%s\"\n",
                        keys[j], groups[i], infile);
            }
            j++;
        }
        i++;
    }

    // TODO check required keys exist (type, file naming stuff)
    // TODO check required keys exist (depends)
    // TODO check known keys values
    return valid;
}

/**
 * @brief Validates a global config keyfile.
 *
 * @param keyfile global config keyfile to be validated
 * @param infile  path to keyfile being validated (for error printing)
 *
 * @return TRUE when a global config keyfile is valid
 */
gboolean post_validate_config_file(GKeyFile *keyfile, gchar *infile)
{
    gboolean valid = TRUE;
    // check CONFIG exists
    if (!g_key_file_has_group(keyfile, "CONFIG")) {
        valid = FALSE;
        g_printerr("Keyfile (%s) missing [CONFIG] section.\n", infile);
    }

    // check only CONFIG exists
    gchar **group_names = (g_key_file_get_groups(keyfile, NULL));
    int i = 0;
    while (group_names[i] != NULL) {
        if (strcmp(group_names[i], "CONFIG") != 0) {
            valid = FALSE;
            g_printerr("Invalid section \"[%s]\" in %s. hbr config should" \
                    " only contain the [CONFIG] section.\n", group_names[i], infile);
        }
        i++;
    }
    if (!post_validate_common(keyfile, infile, NULL)) {
        valid = FALSE; // any errors are printed during post_validate_config_section()
    }
    return valid;
}


/**
 * @brief Validates items common to global config and input keyfiles. Handles
 *        input keyfiles or global config keyfiles depending on
 *        config_keyfile's value.
 *
 * @param keyfile         keyfile to be validated
 * @param infile          path to keyfile being validated (for error printing)
 * @param config_keyfile  global config keyfile or NULL if keyfile is a global
 *                        config
 *
 * @return TRUE when a config section is valid
 */
gboolean post_validate_common(GKeyFile *keyfile, gchar *infile,
    GKeyFile *config_keyfile)
{
    gboolean valid = TRUE;
    // check for unknown sections
    gchar **group_names = (g_key_file_get_groups(keyfile, NULL));
    int i = 0;
    /*
     * check config_keyfile because we already checked for unwanted sections
     * in the global config (only check input keyfiles)
     */
    while (group_names[i] != NULL && config_keyfile != NULL) {
        if (strcmp(group_names[i], "CONFIG") != 0 && strncmp(group_names[i], "OUTFILE", 7) != 0) {
            valid = FALSE;
            g_printerr("Invalid section \"[%s]\" in %s. Keyfile should" \
                    " only contain the [CONFIG] section and one or more" \
                    " [OUTFILE...] sections.\n", group_names[i], infile);
        }
        i++;
    }
    // check for unknown keys
    if (unknown_keys_exist(keyfile, infile)) {
        valid = FALSE;
    }
    // TODO check keys that don't belong in global config
    // TODO check keys that don't belong in local config
    // TODO check required keys exist (there aren't really any, but having none between hbr.conf and infile is not workable)
    // TODO check known keys values
    // TODO check depends
    // TODO check conflicts
    // TODO check negation type conflicts
    return valid;
}

/**
 * @brief Checks a keyfile for unknown key names and prints errors
 *
 * @param keyfile keyfile to be checked
 * @param infile  path to keyfile (for error printing)
 *
 * @return TRUE if unknown keys exist
 */
gboolean unknown_keys_exist(GKeyFile *keyfile, gchar *infile)
{
    gchar **groups = g_key_file_get_groups(keyfile, NULL);
    int i = 0;
    while (groups[i] != NULL) {
        gchar **keys = g_key_file_get_keys(keyfile, groups[i], NULL, NULL);
        int j = 0;
        while (keys[j] != NULL) {
            if (!g_hash_table_contains(options_index, keys[j])) {
                g_printerr("Invalid key \"%s\" in group \"%s\" in file \"%s\"\n",
                        keys[j], groups[i], infile);
            }
            j++;
        }
        i++;
    }
}

/**
 * @brief Opens a file as a data stream for reading
 *
 * @param infile path of file to be read
 *
 * @return data stream, free'd with g_object_unref()
 */
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


/**
 * @brief Checks a keyfile for duplicate groups via manual parsing
 *        This is necessary because GKeyFile silently merges duplicate groups
 *
 * @param datastream stream of file to be parsed
 * @param infile     path to file (for error messages)
 *
 * @return TRUE if file contains duplicate groups
 */
gboolean has_duplicate_groups(GDataInputStream *datastream, gchar *infile)
{
    // check for duplicate group sections
    gchar *line;
    gint line_count = 0;
    GHashTable *group_hashes = g_hash_table_new_full(g_str_hash, g_str_equal,
            g_free, NULL);

    gboolean duplicates = FALSE;
    // parse each line
    while ((line = g_data_input_stream_read_line_utf8(datastream, NULL, NULL,
                    NULL))) {
        line_count++;
        // get group name
        g_strchug(line); // remove leading whitespace
        gint length = strlen(line);
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
            fprintf(stderr, "Keyfile (%s) contains duplicate group "
                    "\"[%s]\" at line %d\n", infile, group_name, line_count);
            g_free(group_name);
            duplicates = TRUE;
        } else {
            g_hash_table_add(group_hashes, group_name);
        }
    }
    g_hash_table_destroy(group_hashes);
    return duplicates;
}
