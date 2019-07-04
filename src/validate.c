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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <math.h>

#include "util.h"
#include "validate.h"
#include "keyfile.h"

/**
 * @brief Check that a file can be read, and no group names are duplicate, and
 *        no keys within groups are duplicate
 *
 * @param infile filename to be checked
 *
 * @return TRUE when a keyfile is readable, and has no duplicate groups/keys
 */
gboolean pre_validate_key_file(const gchar *infile)
{
    gboolean valid = TRUE;
    // check file is readable
    GDataInputStream * datastream = open_datastream(infile);
    if (datastream) {
        // check for duplicate groups
        if (has_duplicate_groups(infile)) {
            valid = FALSE;
        }
        // check for duplicate keys within a group
        if (has_duplicate_keys(infile)) {
            valid = FALSE;
        }
        g_input_stream_close((GInputStream *)datastream, NULL, NULL);
        g_object_unref(datastream);
    } else {
        valid = FALSE;
    }

    return valid;
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
gboolean post_validate_input_file(GKeyFile *input_keyfile, const gchar *infile,
        GKeyFile *config_keyfile)
{
    gboolean valid = TRUE;

    // check CONFIG exists
    if (!g_key_file_has_group(input_keyfile, "CONFIG")) {
        valid = FALSE;
        hbr_error("Keyfile missing [CONFIG] section", infile, NULL, NULL, NULL);
    }
    // validate config section
    if (!post_validate_common(input_keyfile, infile, config_keyfile)) {
        valid = FALSE; // errors printed during post_validate_config_section()
    }

    // check at least one OUTFILE section exists
    if (get_outfile_count(input_keyfile) < 1) {
        valid = FALSE;
        hbr_error("No OUTFILE sections found", infile, NULL, NULL, NULL);
    }

    // check required keys exist (type, file naming stuff)
    if (!has_required_keys(input_keyfile, infile, config_keyfile)) {
        valid = FALSE; // errors printed during has_required_keys()
    }

    // check required keys exist (requires)
    if (!has_requires(input_keyfile, infile, config_keyfile)) {
        valid = FALSE; // errors printed during has_requires()
    }

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
gboolean post_validate_config_file(GKeyFile *keyfile, const gchar *infile)
{
    gboolean valid = TRUE;
    // check CONFIG exists
    if (!g_key_file_has_group(keyfile, "CONFIG")) {
        valid = FALSE;
        hbr_error("Keyfile missing [CONFIG] section", infile, NULL, NULL, NULL);
    }

    // check only CONFIG exists
    gchar **group_names = g_key_file_get_groups(keyfile, NULL);
    int i = 0;
    while (group_names[i] != NULL) {
        if (strcmp(group_names[i], "CONFIG") != 0) {
            valid = FALSE;
            hbr_error("Invalid section in config file", infile, group_names[i],
                    NULL, NULL);
        }
        i++;
    }
    g_strfreev(group_names);
    if (!post_validate_common(keyfile, infile, NULL)) {
        valid = FALSE; // errors printed during post_validate_config_section()
    }
    // check required keys exist
    if (!has_required_keys(keyfile, infile, NULL)) {
        valid = FALSE; // errors printed during has_required_keys()
    }

    // check required keys exist (requires)
    if (!has_requires(keyfile, infile, NULL)) {
        valid = FALSE; // errors printed during has_requires()
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
gboolean post_validate_common(GKeyFile *keyfile, const gchar *infile,
    GKeyFile *config_keyfile)
{
    gboolean valid = TRUE;
    gboolean checking_local_config = config_keyfile != NULL;
    // check for unknown sections
    // only check input keyfiles (when config_keyfile is a valid pointer)
    gchar **group_names = (g_key_file_get_groups(keyfile, NULL));
    int i = 0;
    while (group_names[i] != NULL && checking_local_config) {
        if (strcmp(group_names[i], "CONFIG") != 0 &&
                strncmp(group_names[i], "OUTFILE", 7) != 0) {
            valid = FALSE;
            hbr_error("Invalid section in config file", infile, group_names[i],
                    NULL, NULL);
        }
        i++;
    }
    // check for unknown keys
    if (unknown_keys_exist(keyfile, infile)) {
        valid = FALSE;
    }
    // TODO check keys that don't belong in global config
    if (!checking_local_config) {
    }
    // TODO check keys that don't belong in local config
    if (checking_local_config) {
    }
    // TODO check required keys exist (there aren't really any, but having none
    //      between hbr.conf and infile is not workable)

    /* TODO check depends (done without merging, depends are cases where one
     * option's value affects which values are valid for another option
     */
    // TODO check quantity matches (can be handled with merging)
    // TODO check negation type conflicts (requires merging for local configs)

    /* TODO check conflicts (only requires checking each group because the function
     * merge_key_group removes conflicts during the merge)
     */
    // call valid_option function (requires merging for complex validation)
    i = 0;
    while (group_names[i] != NULL) {
        int j = 0;
        while (options[j].name != NULL) {
            if (g_key_file_has_key(keyfile, group_names[i], options[j].name, NULL)) {
                if (!options[j].valid_option(&options[j],  group_names[i],
                            keyfile, infile)) {
                    valid = FALSE;
                }
            }
            j++;
        }
        i++;
    }
    g_strfreev(group_names);
    return valid;
}

/**
 * @brief Verifies merged result of each outfile has all keys that any other
 *        defined keys require.
 *
 * @param input_keyfile File to be tested
 * @param infile Path for input_keyfile (for error messages)
 * @param config_keyfile Global config or NULL
 *
 * @return TRUE when all requires are fulfilled
 */
gboolean has_requires(GKeyFile *input_keyfile, const gchar *infile,
        GKeyFile *config_keyfile) {
    gboolean valid = TRUE;
    gchar **group_names = g_key_file_get_groups(input_keyfile, NULL);
    int i = 0;
    // we need to pre-merge config sections with config_keyfile here
    GKeyFile *merged_configs = input_keyfile;
    if (config_keyfile != NULL) {
        merged_configs = merge_key_group(input_keyfile, "CONFIG",
                config_keyfile, "CONFIG", "CONFIG");
    }
    // iterate over each outfile section
    while (group_names[i] != NULL && merged_configs != NULL) {
        GKeyFile *test_keyfile = NULL;
        if (strncmp("OUTFILE", group_names[i], sizeof("OUTFILE")-1) == 0) {
            // merge outfile section with config. maintain same names
            if (g_key_file_has_group(input_keyfile, "CONFIG")) {
                test_keyfile = merge_key_group(input_keyfile, group_names[i],
                        merged_configs, "CONFIG", group_names[i]);
            } else {
                // no config section. just update pointer
                test_keyfile = input_keyfile;
            }
            // check each key for requires
            gchar **keys = g_key_file_get_keys(test_keyfile, group_names[i],
                    NULL, NULL);
            int j = 0;
            while (keys[j] != NULL) {
                /* Special case to skip boolean keys that can be negated
                 * This ignores requires when a option is specified as false
                 * (this should mean it's not enabled, and it's requires are not
                 * necessary)
                 */
                gint option_index = GPOINTER_TO_INT(
                        g_hash_table_lookup(options_index, keys[j]));
                if (options[option_index].key_type == k_boolean &&
                        options[option_index].negation_option) {
                    if (g_key_file_get_boolean(test_keyfile, group_names[i],
                                keys[j], NULL) == FALSE) {
                        j++;
                        continue;
                    }
                }
                GSList* requires_list = g_hash_table_lookup(requires_index, keys[j]);
                if (requires_list != NULL) {
                    /* iterate over gslist (pointers are actually int which
                       are the index to the requires table) */
                    int k = 0;
                    do {
                        gint index = GPOINTER_TO_INT(requires_list->data);
                        // check if require is defined
                        if (!g_key_file_has_key(test_keyfile, group_names[i],
                                    requires[index].require_name,
                                    NULL)) {
                            gchar *value = g_key_file_get_value(
                                    test_keyfile, group_names[i], keys[j], NULL);
                            hbr_error("Key \"%s\" requires \"%s\" but it is not"
                                    " set", infile, group_names[i], keys[j], value,
                                    keys[j], requires[index].require_name);
                            valid = FALSE;
                            g_free(value);
                        } else {
                            // if require has specific value check it
                            gchar *requires_value = g_key_file_get_value(
                                    test_keyfile, group_names[i],
                                    requires[index].require_name, NULL);
                            if (requires[index].require_value != NULL &&
                                strcmp(requires_value,
                                    requires[index].require_value) != 0) {
                                    hbr_error("Key \"%s\" requires setting \"%s=%s\"",
                                            infile, group_names[i], keys[j], NULL,
                                            keys[j], requires[index].require_name,
                                            requires[index].require_value);
                                    valid = FALSE;
                            }
                            g_free(requires_value);
                        }
                        requires_list = requires_list->next;
                    } while (requires_list != NULL);
                }
                j++;
            }
            g_strfreev(keys);
        }
        if (test_keyfile && test_keyfile != input_keyfile) {
            g_key_file_free(test_keyfile);
        }
        i++;
    }
    // Free the merged configs unless it still points to input_keyfile
    if (merged_configs != input_keyfile && merged_configs != NULL) {
        g_key_file_free(merged_configs);
    }
    g_strfreev(group_names);
    return valid;
}

/**
 * @brief Verifies the final merged result of each outfile in
 *        input_keyfile contains the required keys.
 *
 * @param input_keyfile File to be tested
 * @param infile Path for input_keyfile (for error messages)
 * @param config_keyfile Global config or NULL
 *
 * @return TRUE when all required keys are present
 */
gboolean has_required_keys(GKeyFile *input_keyfile, const gchar *infile,
        GKeyFile *config_keyfile)
{
    gboolean valid = TRUE;
    gchar **group_names = g_key_file_get_groups(input_keyfile, NULL);
    int i = 0;
    // we need to pre-merge config sections with config_keyfile here
    GKeyFile *merged_configs = input_keyfile;
    if (config_keyfile != NULL) {
        merged_configs = merge_key_group(input_keyfile, "CONFIG",
                config_keyfile, "CONFIG", "CONFIG");
    }
    // iterate over each outfile section
    while (group_names[i] != NULL && merged_configs != NULL) {
        GKeyFile *test_keyfile = NULL;
        if (strncmp("OUTFILE", group_names[i], sizeof("OUTFILE")-1) == 0) {
            // merge outfile section with config. maintain same names
            if (g_key_file_has_group(input_keyfile, "CONFIG")) {
                test_keyfile = merge_key_group(input_keyfile, group_names[i],
                        merged_configs, "CONFIG", group_names[i]);
            } else {
                // no config section. just update pointer
                test_keyfile = input_keyfile;
            }
            // check we have a type
            if (!g_key_file_has_key(test_keyfile, group_names[i],
                        "type", NULL)) {
                valid = FALSE;
                hbr_error("Missing key definition for \"type\"", infile,
                        group_names[i], NULL, NULL);
            }
            // check we have a iso_filename
            if (!g_key_file_has_key(test_keyfile, group_names[i],
                        "iso_filename", NULL)) {
                valid = FALSE;
                hbr_error("Missing key definition for \"iso_filename\"", infile,
                        group_names[i], NULL, NULL);
            }
            // check we have a name
            if (!g_key_file_has_key(test_keyfile, group_names[i],
                        "name", NULL)) {
                valid = FALSE;
                hbr_error("Missing key definition for \"name\"", infile,
                        group_names[i], NULL, NULL);
            }
            // check we have a title
            if (!g_key_file_has_key(test_keyfile, group_names[i],
                        "title", NULL)) {
                valid = FALSE;
                hbr_error("Missing key definition for \"title\"", infile,
                        group_names[i], NULL, NULL);
            }
        }
        if (test_keyfile && test_keyfile != input_keyfile) {
            g_key_file_free(test_keyfile);
        }
        i++;
    }
    // Free the merged configs unless it still points to input_keyfile
    if (merged_configs != input_keyfile && merged_configs != NULL) {
        g_key_file_free(merged_configs);
    }
    g_strfreev(group_names);
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
gboolean unknown_keys_exist(GKeyFile *keyfile, const gchar *infile)
{
    gchar **groups = g_key_file_get_groups(keyfile, NULL);
    int i = 0;
    gboolean unknown_found = FALSE;
    while (groups[i] != NULL) {
        gchar **keys = g_key_file_get_keys(keyfile, groups[i], NULL, NULL);
        int j = 0;
        while (keys[j] != NULL) {
            if (!g_hash_table_contains(options_index, keys[j])) {
                gchar *value = g_key_file_get_value(keyfile, groups[i], keys[j],
                        NULL);
                hbr_error("Invalid key", infile, groups[i], keys[j], value);
                unknown_found = TRUE;
                g_free(value);
            }
            j++;
        }
        i++;
        g_strfreev(keys);
    }
    g_strfreev(groups);
    return unknown_found;
}

/**
 * @brief Checks a keyfile for duplicate groups via manual parsing.
 *        This is necessary because GKeyFile silently merges duplicate groups.
 *
 * @param infile     path to file (for error messages)
 *
 * @return TRUE if file contains duplicate groups
 */
gboolean has_duplicate_groups(const gchar *infile)
{
    GDataInputStream * datastream = open_datastream(infile);
    // check for duplicate group sections
    gchar *line;
    gsize line_length;
    gint line_count = 0;
    GHashTable *group_hashes = g_hash_table_new_full(g_str_hash, g_str_equal,
            g_free, NULL);

    gboolean duplicates = FALSE;
    // parse each line
    while ((line = g_data_input_stream_read_line_utf8(datastream, &line_length,
                    NULL, NULL))) {
        line_count++;
        // get group name
        g_strchug(line); // remove leading whitespace
        gint length = strnlen(line, line_length);
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
            hbr_error("Duplicate group at line %d", infile,
                    group_name, NULL, NULL, line_count);
            g_free(group_name);
            duplicates = TRUE;
        } else {
            g_hash_table_add(group_hashes, group_name);
        }
    }
    // TODO print errors from g_data_input_stream_read_line_utf8
    g_hash_table_destroy(group_hashes);
    g_input_stream_close((GInputStream *)datastream, NULL, NULL);
    g_object_unref(datastream);
    return duplicates;
}

/**
 * @brief Checks a keyfile for duplicate key definitions via manual parsing.
 *        This is necessary because GKeyFile silently overwrites keys defined
 *        more than once in a group.
 *
 * @param infile     path to file (for error messages)
 *
 * @return TRUE if file contains duplicate keys in the same group
 */
gboolean has_duplicate_keys(const gchar *infile)
{
    GDataInputStream * datastream = open_datastream(infile);
    // check for duplicate group sections
    GHashTable *key_hashes = g_hash_table_new_full(g_str_hash, g_str_equal,
            g_free, NULL);

    gchar *line;
    gsize line_length = 0;
    gint line_count = 0;
    gchar *current_group_name = NULL;
    gboolean duplicates = FALSE;
    // parse each line
    while ((line = g_data_input_stream_read_line_utf8(datastream, &line_length,
                    NULL, NULL))) {
        line_count++;
        // get group name
        g_strchug(line); // remove leading whitespace
        gint length = strnlen(line, line_length);
        const gchar *group_name_start = NULL;
        const gchar *group_name_end = line+length-1;
        if (line[0] == '[') {
            // found a group name
            g_hash_table_destroy(key_hashes);
            key_hashes = g_hash_table_new_full(g_str_hash, g_str_equal,
                    g_free, NULL);
            group_name_start = line+1;
            while (*group_name_end != ']' && group_name_end > group_name_start) {
                group_name_end--;
            }
            g_free(current_group_name);
            current_group_name = g_strndup(group_name_start,
                    group_name_end - group_name_start);
            g_free(line);
        } else if (line[0] == '#' || strnlen(g_strchomp(line), line_length) == 0) {
            // found a comment or blank line
            g_free(line);
            continue;
        } else {
            // get key name
            // line after key name
            gchar *key_name = g_strdup(g_strdelimit(line, "=", 0));
            g_free(line);
            // remove trailing whitespace
            g_strchomp(key_name);
            // check for a duplicate hash of a key name
            if (g_hash_table_contains(key_hashes, key_name)) {
                hbr_error("Duplicate key definition at line %d", infile,
                        current_group_name, key_name, NULL, line_count);
                g_free(key_name);
                duplicates = TRUE;
            } else {
                g_hash_table_add(key_hashes, key_name);
            }
        }
    }
    // TODO print errors from g_data_input_stream_read_line_utf8
    g_free(current_group_name);
    g_hash_table_destroy(key_hashes);
    g_input_stream_close((GInputStream *)datastream, NULL, NULL);
    g_object_unref(datastream);
    return duplicates;
}


//TODO add asserts that valid_values and valid_values_count are NULL/0 (for non-list items)
//TODO add asserts throughout all valid_ functions because there's lots
// of ways to muck those files up
// TODO remove pointless print after testing (in all valid_ functions)

/*
 * TODO FIXME valid_ functions shouldn't have gotten tasked with checking inputs
 * based on other options, or matching quantities with other options.
 * The code that does this should be generalized and handled directly from the
 * post_validate_common function.
 */


gboolean valid_type(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    /*
     * TODO this style of check cannot work because season/episode may be split
     * in different groups. Checking this really shouldn't be handled in the
     * valid_ functions because it gets too complicated.
     * Instead there should be a new depends table and related functions that
     * have knowledge of the outfile/config/global_config trifecta and can
     * check appropriate values for the dependent/dependee combos
     */

    gboolean valid = valid_string_list_set(option, group, config, config_path);
    gchar *type = g_key_file_get_value(config, group, option->name, NULL);
    if (strcmp(type, "series") == 0) {
        gboolean has_season = g_key_file_has_key(config, group, "season", NULL);
        gboolean has_episode = g_key_file_has_key(config, group, "episode", NULL);
        if (!has_season && !has_episode) {
            hbr_warn("Season and episode number not specified", config_path,
                    group, option->name, type);
        } else if (!has_season) {
            hbr_warn("Season number not specified", config_path, group,
                    option->name, type);
        } else if (!has_episode) {
            hbr_warn("Episode number not specified", config_path, group,
                    option->name, type);
        }
    }
    g_free(type);
    return valid;
}

gboolean valid_readable_path(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;
    GError *error = NULL;
    gchar *value = g_key_file_get_value(config, group, option->name, NULL);
    if (error) {
        hbr_error(error->message, config_path, group, option->name, NULL);
        g_error_free(error);
        valid = FALSE;
    } else {
        // check directory can be opened
        error = NULL;
        GDir *directory = g_dir_open(value, 0, &error);
        if (directory == NULL && error) {
            hbr_error("Could not read path specified by key", config_path, group,
                    option->name, value);
            g_free(value);
            g_error_free(error);
            valid = FALSE;
        } else {
            g_dir_close(directory);
            g_free(value);
        }
    }
    return valid;
}

gboolean valid_writable_path(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gchar *orig_path = g_key_file_get_value(config, group, option->name, NULL);

    // check parents
    gchar *path = g_strdup(orig_path);
    while (strcmp(path, "/") != 0 && strcmp(path, "") != 0) {
        // check directory exists and is writeable
        if (g_file_test(path, G_FILE_TEST_IS_DIR) &&
                g_access(path, W_OK|R_OK) == 0) {
            g_free(orig_path);
            g_free(path);
            return TRUE;
        } else {
            // error on regular files
            if (g_file_test("path", G_FILE_TEST_IS_REGULAR)) {
                hbr_error("Regular file specified instead of path", config_path,
                        group, option->name, orig_path);
                g_free(path);
                g_free(orig_path);
                return FALSE;
            }
            // directory doesn't exist, try parent next
            char *lastslash = strrchr(path, G_DIR_SEPARATOR);
            if (lastslash != NULL) {
                // strip last directory from path
                *lastslash = '\0';
            } else {
                break;
            }
        }
    }
    hbr_error("Unwriteable path specified by key", config_path, group,
            option->name, orig_path);
    g_free(path);
    g_free(orig_path);
    return FALSE;
}

gboolean valid_filename_component(option_t *option, gchar *group,
        GKeyFile *config, const gchar *config_path)
{
    gboolean valid = TRUE;
    GError *error = NULL;
    gchar *component = g_key_file_get_value(config, group, option->name, &error);

    if (error) {
        valid = FALSE;
        hbr_error(error->message, config_path, group, option->name, NULL);
    } else {
        size_t component_length = strnlen(component, MAXPATHLEN);
        if ( component_length < 1 || component_length == MAXPATHLEN) {
            valid = FALSE;
            hbr_error("Invalid component component length", config_path, group,
                    option->name, component);
        }
        for (int i = 0; i < component_length; i++) {
            // control chars (ascii 1-31,127)
            if (g_ascii_iscntrl(component[i])) {
                valid = FALSE;
                hbr_error("Filename component contains control character",
                        config_path, group, option->name, NULL);
            }
            // shell metacharacters
            // Found this too annoying, so it's disabled
            /*
            gchar *metachar = "*?:[]\"<>|(){}&'!\\;$";
            size_t metachar_len = strnlen(metachar, sizeof("*?:[]\"<>|(){}&'!\\;$"));
            for (int j = 0; j < metachar_len; j++) {
                if (component[i] == metachar[j]) {
                    hbr_warn("Filename component contains shell metacharacters"
                            " (*?:[]\"<>|(){}&'!\\;$)",
                            config_path, group, option->name, component);
                }
            }
            */
        }
        g_free(component);
    }
    return valid;
}

gboolean valid_boolean(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    gchar *value = g_key_file_get_value(config, group, option->name, NULL);
    if (value != NULL) {
        // remove leading/trailing whitespace
        g_strstrip(value);
        if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
            g_free(value);
            return TRUE;
        }
    }
    hbr_error("Invalid boolean value; Use 'true' or 'false'", config_path,
            group, option->name, value);
    g_free(value);
    return FALSE;
}

gboolean valid_integer(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    gboolean valid = TRUE;
    GError *error = NULL;
    gint value = g_key_file_get_integer(config, group, option->name, &error);
    if (value == 0 && error != NULL) {
        valid = FALSE;
        gchar* string_value = g_key_file_get_value(config, group, option->name, NULL);
        hbr_error("Value should be integer", config_path, group, option->name,
                string_value);
        g_error_free(error);
        g_free(string_value);
    }
    return valid;
}

gboolean valid_integer_set(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_integer_set: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_integer_list(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    gboolean valid = TRUE;
    GError *error = NULL;
    gsize count;
    gint *values = g_key_file_get_integer_list(config, group, option->name,
            &count, &error);
    if (values == NULL && error != NULL) {
        valid = FALSE;
        gchar* string_value = g_key_file_get_value(config, group, option->name, NULL);
        hbr_error("Value should be a comma separated integer list", config_path,
                group, option->name, string_value);
        g_error_free(error);
        g_free(string_value);
    }
    g_free(values);
    return valid;
}

gboolean valid_integer_list_set(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_integer_list_set: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_positive_integer(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    gboolean valid = TRUE;
    GError *error = NULL;
    gint value = g_key_file_get_integer(config, group, option->name, &error);
    if (value == 0 && error != NULL) {
        valid = FALSE;
        gchar* string_value = g_key_file_get_value(config, group, option->name, NULL);
        hbr_error("Value should be a positive integer", config_path, group,
                option->name, string_value);
        g_error_free(error);
        g_free(string_value);
    }
    if (value < 0) {
        valid = FALSE;
        gchar* string_value = g_key_file_get_value(config, group, option->name, NULL);
        hbr_error("Value should be a positive integer", config_path, group,
                option->name, string_value);
        g_free(string_value);
    }
    return valid;
}

gboolean valid_double_list(option_t *option, gchar *group,
        GKeyFile *config, const gchar *config_path) {
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    gboolean valid = TRUE;
    GError *error = NULL;
    gsize value_count = 0;
    gdouble *values = g_key_file_get_double_list(config, group, option->name,
            &value_count, &error);
    if (values == NULL && error != NULL) {
        valid = FALSE;
        gchar* string_value = g_key_file_get_value(config, group, option->name, NULL);
        hbr_error("Value should be a comma separated double list", config_path,
                group, option->name, string_value);
        g_error_free(error);
        g_free(string_value);
    }
    g_free(values);
    return valid;
}

gboolean valid_positive_double_list(option_t *option, gchar *group,
        GKeyFile *config, const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    gboolean valid = TRUE;
    GError *error = NULL;
    gsize value_count = 0;
    gdouble *values = g_key_file_get_double_list(config, group, option->name,
            &value_count, NULL);
    if (values == NULL) {
        valid = FALSE;
        gchar* string_value = g_key_file_get_value(config, group, option->name, NULL);
        hbr_error("Value should be a comma separated positive double list", config_path,
                group, option->name, string_value);
        g_error_free(error);
        g_free(string_value);
    } else {
        for (int i=0; i<value_count; i++) {
            if (values[i] < 0.0) {
                valid = FALSE;
                gchar *value = g_strdup_printf("%f", values[i]);
                hbr_error("Value is not a positive double",
                        config_path, group, option->name, value);
                g_free(value);
            }
        }
    }
    g_free(values);
    return valid;
}

gboolean valid_string(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    g_print("valid_string: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_string_set(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    assert(option->valid_values_count != 0 && option->valid_values != NULL);
    gboolean valid = TRUE;
    gchar *value = g_key_file_get_value(config, group, option->name, NULL);
    for (int i = 0; i < option->valid_values_count; i++) {
        if (strcmp(value, ((gchar **)option->valid_values)[i]) == 0) {
            // found a match, stop searching
            valid = TRUE;
            break;
        } else {
            valid = FALSE;
        }
    }
    if (valid == FALSE) {
        hbr_error("Invalid key value", config_path, group, option->name, value);
    }
    g_free(value);
    return valid;
}

gboolean valid_string_list_set(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    assert(option->valid_values_count != 0 && option->valid_values != NULL);
    GError *error = NULL;
    gsize list_count = 0;
    gchar **string_list = g_key_file_get_string_list(config, group, option->name,
            &list_count, &error);
    gboolean all_valid = TRUE;
    if (error != NULL) {
        all_valid = FALSE;
        gchar *value = g_key_file_get_value(config, group, option->name, NULL);
        /* re-using glib error message because the potential errors should only
         * occur from programmer error
         */
        hbr_error(error->message, config_path, group, option->name, value);
        g_free(value);
        g_error_free(error);
    } else {
        for (int i = 0; i < list_count; i++) {
            gboolean valid = TRUE;
            g_strstrip(string_list[i]);
            for (int j = 0; j < option->valid_values_count; j++) {
                if (strcmp(string_list[i], ((gchar **)option->valid_values)[j]) == 0) {
                    // found a match, stop searching
                    valid = TRUE;
                    break;
                } else {
                    valid = FALSE;
                }
            }
            if (valid == FALSE) {
                all_valid = FALSE;
                hbr_error("Invalid key value", config_path, group, option->name,
                        string_list[i]);
            }
        }
    }
    g_strfreev(string_list);
    return all_valid;
}

gboolean valid_string_list(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    g_print("valid_string_list: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_filename_exists(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;
    g_print("valid_filename_exists: %s\n", option->name);  //TODO REMOVE
    GError *error = NULL;
    gchar *filename = g_key_file_get_value(config, group, option->name, &error);
    // check file exists and can be opened
    if (g_access(filename, R_OK) != 0) {
        valid = FALSE;
        hbr_error("Could not read file specified", config_path, group,
                option->name, filename);
        g_free(filename);
    }
    return valid;
}

gboolean valid_filename_exists_list(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;
    GError *error = NULL;
    gsize filename_count = 0;
    gchar **filenames = g_key_file_get_string_list(config, group, option->name,
            &filename_count, &error);
    if (error) {
        valid = FALSE;
        hbr_error(error->message, config_path, group, option->name, NULL);
        g_error_free(error);
    } else if ( filename_count < 1) {
        valid = FALSE;
        hbr_error("File not specified", config_path, group, option->name,
                NULL);
    } else {
        for (int i = 0; i < filename_count; i++) {
            g_strstrip(filenames[i]);
            if (g_access(filenames[i], R_OK) != 0) {
                valid = FALSE;
                hbr_error("Could not read file specified", config_path, group,
                        option->name, filenames[i]);
            }
        }
        g_strfreev(filenames);
    }
    return valid;
}

gboolean valid_filename_dne(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_filename_dne: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_startstop_at(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_startstop_at: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_previews(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_previews: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_audio(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    // check for 'none'
    gchar *value = g_key_file_get_value(config, group, option->name, NULL);
    if (strcmp(value, "none") == 0) {
        return TRUE;
    }

    gboolean valid = TRUE;
    // try to parse list of integers
    gsize integer_count = 0;
    GError *error = NULL;
    gint *values = g_key_file_get_integer_list(config, group, option->name,
            &integer_count, &error);
    if (error != NULL) {
        // parser failed
        if (g_error_matches(error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_INVALID_VALUE)) {
            hbr_error("Value should be integer", config_path, group,
                    option->name, value);
        } else {
            // use glib error message for non-user error
            hbr_error(error->message, config_path, group, option->name, value);
        }
        valid = FALSE;
        g_error_free(error);
    } else {
        // sort list
        int i = 0, temp;
        while (i < integer_count) {
            if (i == 0 || values[i-1] <= values[i]) {
                i++;
            } else {
                temp = values[i];
                values[i] = values[i-1];
                values[--i] = temp;
            }
        }
        // check integers aren't repeated, but only warn
        i = 0;
        while (i < integer_count-1) {
            if (values[i] == values[i+1]) {
                hbr_warn("Audio track repeated", config_path, group,
                        option->name, value);
            }
            i++;
        }
    }

    g_free(value);
    g_free(values);
    return valid;
}

gboolean valid_audio_encoder(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    // verify audio encoder count is the same as audio track count
    gboolean valid = TRUE;
    gsize audio_count = 0;
    GError *error = NULL;
    if (g_key_file_has_key(config, group, "audio", NULL)) {
        gint *audio_tracks = g_key_file_get_integer_list(config, group,
                "audio", &audio_count, &error);
        if (error != NULL) {
            hbr_error(error->message, config_path, group, option->name, NULL);
            g_error_free(error);
        }
        g_free(audio_tracks);
    }
    error = NULL;
    gsize  audio_encoder_count;
    gchar **audio_encoders = g_key_file_get_string_list(config, group,
            option->name, &audio_encoder_count, &error);
    if (error != NULL) {
        hbr_error(error->message, config_path, group, option->name, NULL);
        g_error_free(error);
    }
    g_strfreev(audio_encoders);
    gchar *aencoder_string = g_key_file_get_value(config, group, option->name,
            NULL);
    if (audio_encoder_count != 1 && audio_encoder_count != audio_count) {
        hbr_error("Number of audio encoders (%d) specified does not match the"
                " number of audio tracks (%d)", config_path, group,
                option->name, aencoder_string, audio_encoder_count, audio_count);
        valid = FALSE;
    }
    g_free(aencoder_string);

    // rest of verification can use valid_string_list_set
    if (!valid_string_list_set(option, group, config, config_path)) {
        valid = FALSE;
    }
    return valid;
}

gboolean valid_audio_quality(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_audio_quality: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_audio_bitrate(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;
    int valid_bitrates[] = {6, 12, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128,
        160, 192, 224, 256, 320, 384, 448, 512, 576, 640, 768, 960, 1152, 1344,
        1536, 2304, 3072, 4608, 6144};
    int valid_bitrates_count = sizeof(valid_bitrates)/sizeof(valid_bitrates[0]);

    // get bitrates
    gsize bitrate_count, aencoder_count;
    GError *error = NULL;
    gint *bitrates = g_key_file_get_integer_list(config, group, option->name,
            &bitrate_count, &error);
    gchar *bitrates_string = g_key_file_get_value(config, group,
            option->name, NULL);
    if (error != NULL) {
        if (g_error_matches(error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_INVALID_VALUE)) {
            hbr_error("Value should be comma-separated integer list", config_path, group,
                    option->name, bitrates_string);
        } else {
            // use glib error message for non-user error
            hbr_error(error->message, config_path, group,
                    option->name, bitrates_string);
        }
        valid = FALSE;
        g_error_free(error);
    }
    error = NULL;

    // get audio encoders
    gchar **audio_encoders = g_key_file_get_string_list(config, group,
            "aencoder", &aencoder_count, &error);
    if (error != NULL) {
        hbr_error("Could not verify audio track bitrates because audio encoders"
                " were not specified", config_path, group, option->name,
                bitrates_string);
        valid = FALSE;
        g_error_free(error);
    }

    // verify matching number of track/rates
    gsize audio_count = 0;
    gint *audio_tracks = g_key_file_get_integer_list(config, group,
            "audio", &audio_count, NULL);
    if ( bitrate_count != 1 && bitrate_count != audio_count) {
        hbr_error("Number of track bitrates (%d) specified does not match the"
                " number of audio tracks (%d)", config_path, group,
                option->name, bitrates_string, bitrate_count, audio_count);
        valid = FALSE;
    }
    for (int i = 0; i < bitrate_count && i < aencoder_count; i++) {
        gboolean valid_bitrate = FALSE;
        // verify value is in list of accepted bitrates
        for (int j = 0; j < valid_bitrates_count; j++) {
            if ( bitrates[i] == valid_bitrates[j] ) {
                valid_bitrate = TRUE;
            }
        }

        // remove leading/trailing whitespace for consistent values
        g_strstrip(audio_encoders[i]);
        // verify value is in range for audio codec
        if (strcmp("av_aac", audio_encoders[i]) == 0) {
            if (bitrates[i] < 64 || bitrates[i] > 512) {
                gchar *bad_bitrate = g_strdup_printf("%d", bitrates[i]);
                hbr_error("Bitrate outside range [64,512] for encoder %s",
                        config_path, group, option->name, bad_bitrate,
                        audio_encoders[i]);
                valid = FALSE;
                g_free(bad_bitrate);
            }
        }
        if (strcmp("ac3", audio_encoders[i]) == 0) {
            if (bitrates[i] < 96 || bitrates[i] > 640) {
                gchar *bad_bitrate = g_strdup_printf("%d", bitrates[i]);
                hbr_error("Bitrate outside range [96,640] for encoder %s",
                        config_path, group, option->name, bad_bitrate,
                        audio_encoders[i]);
                valid = FALSE;
                g_free(bad_bitrate);
            }
        }
        if (strcmp("eac3", audio_encoders[i]) == 0) {
            if (bitrates[i] < 96 || bitrates[i] > 6144) {
                gchar *bad_bitrate = g_strdup_printf("%d", bitrates[i]);
                hbr_error("Bitrate outside range [96,6144] for encoder %s",
                        config_path, group, option->name, bad_bitrate,
                        audio_encoders[i]);
                valid = FALSE;
                g_free(bad_bitrate);
            }
        }
        if (strcmp("mp3", audio_encoders[i]) == 0) {
            if (bitrates[i] < 12 || bitrates[i] > 320) {
                gchar *bad_bitrate = g_strdup_printf("%d", bitrates[i]);
                hbr_error("Bitrate outside range [12,320] for encoder %s",
                        config_path, group, option->name, bad_bitrate,
                        audio_encoders[i]);
                valid = FALSE;
                g_free(bad_bitrate);
            }
        }
        if (strcmp("vorbis", audio_encoders[i]) == 0) {
            if (bitrates[i] < 32 || bitrates[i] > 448) {
                gchar *bad_bitrate = g_strdup_printf("%d", bitrates[i]);
                hbr_error("Bitrate outside range [32,448] for encoder %s",
                        config_path, group, option->name, bad_bitrate,
                        audio_encoders[i]);
                valid = FALSE;
                g_free(bad_bitrate);
            }
        }
        if (strcmp("opus", audio_encoders[i]) == 0) {
            if (bitrates[i] < 12 || bitrates[i] > 512) {
                gchar *bad_bitrate = g_strdup_printf("%d", bitrates[i]);
                hbr_error("Bitrate outside range [12,512] for encoder %s",
                        config_path, group, option->name, bad_bitrate,
                        audio_encoders[i]);
                valid = FALSE;
                g_free(bad_bitrate);
            }
        }

        /* check for copy and non-bitrate based encoders. we just ignore bitrates
         * specified for them because I don't want a wall of text if some invalid
         * placeholder value is used.
         */
        gboolean bitrate_track = TRUE;
        if (strcmp("none", audio_encoders[i]) == 0 ||
                strcmp("copy:aac", audio_encoders[i]) == 0 ||
                strcmp("copy:ac3", audio_encoders[i]) == 0 ||
                strcmp("copy:eac3", audio_encoders[i]) == 0 ||
                strcmp("copy:truehd", audio_encoders[i]) == 0 ||
                strcmp("copy:dts", audio_encoders[i]) == 0 ||
                strcmp("copy:dtshd", audio_encoders[i]) == 0 ||
                strcmp("copy:mp3", audio_encoders[i]) == 0 ||
                strcmp("copy", audio_encoders[i]) == 0 ||
                strcmp("flac16", audio_encoders[i]) == 0 ||
                strcmp("flac24", audio_encoders[i]) == 0) {
            bitrate_track = FALSE;
        }
        // Catch values in range, but otherwise invalid
        if (!valid_bitrate && bitrate_track) {
            gchar *bad_bitrate = g_strdup_printf("%d", bitrates[i]);
            hbr_error("Invalid bitrate specified", config_path, group,
                    option->name, bad_bitrate);
            valid = FALSE;
            g_free(bad_bitrate);
        }
    }
    g_free(bitrates_string);
    g_free(bitrates);
    g_free(audio_tracks);
    g_strfreev(audio_encoders);
    return valid;
}

gboolean valid_audio_compression(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;
    GError *error = NULL;
    gsize compression_count = 0;
    gdouble *compressions = g_key_file_get_double_list(config, group,
            option->name, &compression_count, &error);
    if (compressions == NULL && error != NULL) {
        hbr_error(error->message, config_path, group, option->name, NULL);
        g_error_free(error);
        return FALSE;
    }
    error = NULL;
    gsize encoder_count = 0;
    gchar **encoders = g_key_file_get_string_list(config, group, "aencoder",
            &encoder_count, &error);
    gchar *compressions_value =  g_key_file_get_value(config, group,
            option->name, NULL);
    if (compressions == NULL) {
        hbr_error("Encoder not specified. Unable to verify audio compression ",
                config_path, group, option->name, compressions_value);
        g_free(compressions);
        g_free (compressions_value);
        g_error_free(error);
        return FALSE;
    }
    if ( compression_count != 1 && compression_count != encoder_count) {
        // error on mismatched counts
        hbr_error("Number of compression values (%d) specified does not match"
                " the number of audio encoders (%d)", config_path, group,
                option->name, compressions_value, compression_count,
                encoder_count);
        valid = FALSE;
    } else if (compression_count == 1 && encoder_count > 1) {
        //try to validate compression against all encoders
        for (int i=0; i<encoder_count; i++) {
            g_strstrip(encoders[i]);
            gchar *value = g_strdup_printf("%f", compressions[0]);
            if (fabs(compressions[0] - (-1.0)) < 0.001) {
                // default value, ignored
            } else if (strcmp(encoders[i], "flac") == 0 ||
                    strcmp(encoders[i], "flac24") == 0) {
                if (compressions[0] < 0.0 || compressions[0] > 12.0) {
                    valid = FALSE;
                    hbr_error("Compression value outside range for flac [0,12]",
                            config_path, group, option->name, value);
                }
            } else if (strcmp(encoders[i], "mp3") == 0) {
                if (compressions[0] < 0.0 || compressions[0] > 9.0) {
                    gchar *value = g_strdup_printf("%f", compressions[0]);
                    valid = FALSE;
                    hbr_error("Compression value outside range for mp3 [0,9]",
                            config_path, group, option->name, value);
                }
            } else if (strcmp(encoders[i], "opus") == 0) {
                if (compressions[0] < 0.0 || compressions[0] > 10.0) {
                    valid = FALSE;
                    hbr_error("Compression value outside range for opus [0,10]",
                            config_path, group, option->name, value);
                }
            } else {
                valid = FALSE;
                hbr_error("Compression value cannot apply to encoder %s",
                        config_path, group, option->name, value, encoders[i]);
            }
            g_free(value);
        }
    } else {
        // try to validate each compression against each encoder
        for (int i=0; i<compression_count; i++) {
            gchar *value = g_strdup_printf("%f", compressions[i]);
            g_strstrip(encoders[i]);
            if (fabs(compressions[i] - (-1.0)) < 0.001) {
                // default value, ignored
            } else if (strcmp(encoders[i], "flac") == 0 ||
                    strcmp(encoders[i], "flac24") == 0) {
                if (compressions[i] < 0.0 || compressions[i] > 12.0) {
                    valid = FALSE;
                    hbr_error("Compression value outside range for flac [0,12]",
                            config_path, group, option->name, value);
                }

            } else if (strcmp(encoders[i], "mp3") == 0) {
                if (compressions[i] < 0.0 || compressions[i] > 9.0) {
                    valid = FALSE;
                    hbr_error("Compression value outside range for mp3 [0,9]",
                            config_path, group, option->name, value);
                }
            } else if (strcmp(encoders[i], "opus") == 0) {
                if (compressions[i] < 0.0 || compressions[i] > 10.0) {
                    valid = FALSE;
                    hbr_error("Compression value outside range for opus [0,10]",
                            config_path, group, option->name, value);
                }
            } else {
                valid = FALSE;
                hbr_error("Compression value cannot apply to encoder %s",
                        config_path, group, option->name, value, encoders[i]);
            }
            g_free(value);
        }
    }
    g_free(compressions_value);
    g_free(compressions);
    g_free(encoders);
    return valid;
}

gboolean valid_video_quality(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;
    GError *error = NULL;
    gdouble value = g_key_file_get_double(config, group, option->name,
            &error);
    gchar *string = g_key_file_get_value(config, group, option->name, NULL);
    if (error != NULL) {
        hbr_error("Value should be floating point number", config_path, group,
                option->name, string);
        g_error_free(error);
        valid = FALSE;
    }
    // quality depends on encoder, but is an integer within a range
    error = NULL;
    gchar *encoder = g_key_file_get_string(config, group, "encoder", &error);
    if (error != NULL) {
        valid = FALSE;
        hbr_warn("Encoder not specified. Unable to verify video quality",
                config_path, group, option->name, string);
        g_error_free(error);
    } else {
        if (strcmp(encoder, "x264") == 0 || strcmp(encoder, "x265") == 0 ) {
            if (value > 51 || value < 0) {
                valid = FALSE;
                hbr_error("Value outside range [0,51] for encoder %s",
                        config_path, group, option->name, string, encoder);
            }
        }
        if (strcmp(encoder, "x264_10bit") == 0 || strcmp(encoder, "x265_10bit") == 0 ) {
            if (value > 51 || value < -12) {
                valid = FALSE;
                hbr_error("Value outside range [-12,51] for encoder %s",
                        config_path, group, option->name, string, encoder);
            }
        }
        if (strcmp(encoder, "x265_12bit") == 0 ) {
            if (value > 51 || value < -24) {
                valid = FALSE;
                hbr_error("Value outside range [-24,51] for encoder %s",
                        config_path, group, option->name, string, encoder);
            }
        }
        if (strcmp(encoder, "mpeg4") == 0 || strcmp(encoder, "mpeg2") == 0 ) {
            if (value > 31 || value < 1) {
                valid = FALSE;
                hbr_error("Value outside range [0,31] for encoder %s",
                        config_path, group, option->name, string, encoder);
            }
        }
        if (strcmp(encoder, "VP8") == 0 || strcmp(encoder, "VP9") == 0 ||
                strcmp(encoder, "theora") == 0 ) {
            if (value > 63 || value < 0) {
                valid = FALSE;
                hbr_error("Value outside range [0,63] for encoder %s",
                        config_path, group, option->name, string, encoder);
            }
        }
    }
    g_free(encoder);
    g_free(string);
    return valid;
}

gboolean valid_video_bitrate(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    gboolean valid = TRUE;
    GError *error = NULL;
    gint value = g_key_file_get_integer(config, group, option->name, &error);
    if (value == 0 && error != NULL) {
        valid = FALSE;
        gchar* string_value = g_key_file_get_value(config, group, option->name, NULL);
        hbr_error("Value should be integer", config_path, group, option->name,
                string_value);
        g_error_free(error);
        g_free(string_value);
    }
    if (value < 0 || value > 1000000) {
        valid = FALSE;
        gchar* string_value = g_key_file_get_value(config, group, option->name, NULL);
        hbr_error("Value should be integer in range [0,1000000]", config_path,
                group, option->name, string_value);
        g_free(string_value);
    }
    return valid;
}

gboolean valid_video_framerate(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_video_framerate: %s\n", option->name);  //TODO REMOVE
    // try to interpret it with valid_string_set style lookup
    // if that fails grab a double and verify in range 1-1000
    return FALSE; //TODO incomplete
}

gboolean valid_crop(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid_boolean = TRUE, valid_crop = TRUE;
    // check for boolean value
    GError *error = NULL;
    g_key_file_get_boolean(config, group, option->name, &error);
    if (error != NULL) {
        valid_boolean = FALSE;
        g_error_free(error);
    }

    // check crop values
    error = NULL;
    gsize crop_count;
    g_key_file_set_list_separator(config, ':');
    gint *crop = g_key_file_get_integer_list(config, group, option->name, &crop_count, &error);
    g_key_file_set_list_separator(config, ',');
    if (error != NULL) {
        valid_crop = FALSE;
        g_error_free(error);
    }
    if (crop_count != 4) {
        valid_crop = FALSE;
    } else {
        for (int i = 0; i < crop_count; i++) {
            if (crop[i] < 0) {
                valid_crop = FALSE;
            }
        }
    }
    g_free(crop);

    if (!valid_boolean && !valid_crop) {
        gchar *value = g_key_file_get_value(config, group, option->name, NULL);
        hbr_error("Crop should be true/false or 4 colon separated positive"
                " integers (top:bottom:left:right)", config_path, group,
                option->name, value);
        g_free(value);
    }

    return (valid_boolean || valid_crop);
}

gboolean valid_pixel_aspect(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_pixel_aspect: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_decomb(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid_boolean = TRUE, valid_preset = TRUE, valid_custom = TRUE;
    // check for boolean value
    GError *error = NULL;
    g_key_file_get_boolean(config, group, option->name, &error);
    if (error != NULL) {
        valid_boolean = FALSE;
        g_error_free(error);
    }

    // check for presets (bob, eedi2, eedi2bob)
    error = NULL;
    gchar* value = g_key_file_get_string(config, group, option->name, &error);
    if (error == NULL) {
        if (strcmp(value, "bob") != 0 && strcmp(value, "eedi2") != 0 &&
                strcmp(value, "eedi2bob") != 0) {
            valid_preset = FALSE;
        }
    } else {
        valid_preset = FALSE;
        g_error_free(error);
    }

    // check for filter
    // change separator temporarily to parse colon separated list
    g_key_file_set_list_separator(config, ':');
    error = NULL;
    gsize filter_count = 0;
    gchar **filters = g_key_file_get_string_list(config, group,
            option->name, &filter_count, &error);
    gchar *filter_names[] = { "mode", "magnitude-thresh", "variance-thresh",
        "laplacian-thresh", "dilation-thresh", "erosion-thresh", "noise-thresh",
        "search-distance", "postproc", "parity", NULL};
    if (error != NULL) {
        valid_custom = FALSE;
        g_error_free(error);
    } else {
        for (int i = 0; i < filter_count; i++) {
            g_strstrip(filters[i]);
            // split string on '=' into 2 tokens
            gchar **tokens = g_strsplit(filters[i], "=", 2);
            if (tokens[0] == NULL) {
                valid_custom = FALSE;
            } else if (tokens[1] == NULL) {
                valid_custom = FALSE;
                g_strfreev(tokens);
            } else {
                // verify first token is one of filter_names
                int j = 0;
                while (filter_names[j] != NULL) {
                    if (strcmp(tokens[0], filter_names[j]) == 0) {
                        break;
                    }
                    j++;
                }
                if (filter_names[j] == NULL) {
                    valid_custom = FALSE;
                }
                // verify second token is a valid integer
                int k = 0;
                while (tokens[1][k] != '\0') {
                    if (!g_ascii_isdigit(tokens[1][k])) {
                        valid_custom = FALSE;
                    }
                    k++;
                }
                gchar *endptr = NULL;
                gint64 number = g_ascii_strtoll(tokens[1], &endptr, 10);
                if ((number == G_MAXINT64 || number == G_MININT64)
                        && errno == ERANGE) {
                    // value would cause overflow
                    valid_custom = FALSE;
                }
                if (number == 0 && endptr == tokens[1]) {
                    // string conversion failed
                    valid_custom = FALSE;
                }
                g_strfreev(tokens);
            }
        }
    }
    g_strfreev(filters);
    // reset separator
    g_key_file_set_list_separator(config, ',');

    if (!valid_boolean && !valid_preset && !valid_custom) {
        hbr_error("Invalid decomb option", config_path, group, option->name,
                value);
    }
    g_free(value);
    return (valid_boolean || valid_preset || valid_custom);
}

gboolean valid_denoise(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid_preset = TRUE, valid_custom = TRUE;
    // check for presets (ultralight, light, medium, strong)
    GError *error = NULL;
    gchar* value = g_key_file_get_string(config, group, option->name, &error);
    if (error == NULL) {
        if (strcmp(value, "ultralight") != 0 && strcmp(value, "light") != 0 &&
                strcmp(value, "medium") != 0 && strcmp(value, "strong") != 0) {
            valid_preset = FALSE;
        }
    } else {
        valid_preset = FALSE;
        g_error_free(error);
    }

    // check for filter
    // change separator temporarily to parse colon separated list
    g_key_file_set_list_separator(config, ':');
    error = NULL;
    gsize filter_count = 0;
    gchar **filters = g_key_file_get_string_list(config, group,
            option->name, &filter_count, &error);
    gchar *filter_names[] = { "y-spatial", "cb-spatial", "cr-spatial",
        "y-temporal", "cb-temporal", "cr-temporal", NULL};
    if (error != NULL) {
        valid_custom = FALSE;
        g_error_free(error);
    } else {
        for (int i = 0; i < filter_count; i++) {
            g_strstrip(filters[i]);
            // split string on '=' into 2 tokens
            gchar **tokens = g_strsplit(filters[i], "=", 2);
            if (tokens[0] == NULL) {
                valid_custom = FALSE;
            } else if (tokens[1] == NULL) {
                valid_custom = FALSE;
                g_strfreev(tokens);
            } else {
                // verify first token is one of filter_names
                int j = 0;
                while (filter_names[j] != NULL) {
                    if (strcmp(tokens[0], filter_names[j]) == 0) {
                        break;
                    }
                    j++;
                }
                if (filter_names[j] == NULL) {
                    valid_custom = FALSE;
                }
                // verify second token is a valid integer
                int k = 0;
                while (tokens[1][k] != '\0') {
                    if (!g_ascii_isdigit(tokens[1][k])) {
                        valid_custom = FALSE;
                    }
                    k++;
                }
                gchar *endptr = NULL;
                gint64 number = g_ascii_strtoll(tokens[1], &endptr, 10);
                if ((number == G_MAXINT64 || number == G_MININT64)
                        && errno == ERANGE) {
                    // value would cause overflow
                    valid_custom = FALSE;
                }
                if (number == 0 && endptr == tokens[1]) {
                    // string conversion failed
                    valid_custom = FALSE;
                }
                g_strfreev(tokens);
            }
        }
    }
    g_strfreev(filters);
    // reset separator
    g_key_file_set_list_separator(config, ',');

    if (!valid_preset && !valid_custom) {
        hbr_error("Invalid denoise option", config_path, group, option->name,
                value);
    }
    g_free(value);
    return (valid_preset || valid_custom);
}

gboolean valid_deblock(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid_boolean = TRUE, valid_custom = TRUE;
    // check for boolean value
    GError *error = NULL;
    g_key_file_get_boolean(config, group, option->name, &error);
    if (error != NULL) {
        valid_boolean = FALSE;
        g_error_free(error);
    }

    // check for custom format
    error = NULL;
    gsize filter_count = 0;

    g_key_file_set_list_separator(config, ':');
    gchar **filters = g_key_file_get_string_list(config, group,
            option->name, &filter_count, &error);
    g_key_file_set_list_separator(config, ',');

    gchar *filter_names[] = { "qp", "mode", "disable", NULL};
    if (error != NULL) {
        valid_custom = FALSE;
        g_error_free(error);
    } else {
        for (int i = 0; i < filter_count; i++) {
            g_strstrip(filters[i]);
            // split string on '=' into 2 tokens
            gchar **tokens = g_strsplit(filters[i], "=", 2);
            if (tokens[0] == NULL) {
                valid_custom = FALSE;
            } else if (tokens[1] == NULL) {
                valid_custom = FALSE;
                g_strfreev(tokens);
            } else {
                // verify first token is one of filter_names
                int j = 0;
                while (filter_names[j] != NULL) {
                    if (strcmp(tokens[0], filter_names[j]) == 0) {
                        break;
                    }
                    j++;
                }
                if (filter_names[j] == NULL) {
                    valid_custom = FALSE;
                }
                // verify second token is a valid integer
                int k = 0;
                while (tokens[1][k] != '\0') {
                    if (!g_ascii_isdigit(tokens[1][k])) {
                        valid_custom = FALSE;
                    }
                    k++;
                }
                gchar *endptr = NULL;
                gint64 number = g_ascii_strtoll(tokens[1], &endptr, 10);
                if ((number == G_MAXINT64 || number == G_MININT64)
                        && errno == ERANGE) {
                    // value would cause overflow
                    valid_custom = FALSE;
                }
                if (number == 0 && endptr == tokens[1]) {
                    // string conversion failed
                    valid_custom = FALSE;
                }
                g_strfreev(tokens);
            }
        }
    }
    g_strfreev(filters);

    gchar* value = g_key_file_get_string(config, group, option->name, &error);
    if (!valid_boolean && !valid_custom) {
        hbr_error("Invalid deblock option", config_path, group, option->name,
                value);
    }
    g_free(value);
    return (valid_boolean || valid_custom);
}

gboolean valid_deinterlace(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_deinterlace: %s\n", option->name);  //TODO REMOVE
    // custom isn't really a preset, just designates a custom format was
    // specified, REMOVE IT. NULL is just a terminator
    /*gchar *preset_list[] = {"custom", "default", "skip-spatial", "bob", "qsv",
     *   "fast", "slow", "slower", "qsv", NULL};
     */
    return FALSE; //TODO incomplete
}

gboolean valid_detelecine(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_detelecine: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_iso_639(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;
    GError *error = NULL;
    gchar *value = g_key_file_get_value(config, group, option->name, &error);
    if (error != NULL) {
        valid = FALSE;
        hbr_error(error->message, config_path, group, option->name, NULL);
    }
    int i = 0;
    while (iso_639_2[i] != NULL) {
        if (strcmp(value, iso_639_2[i]) == 0) {
            break;
        }
        i++;
    }
    g_free(value);
    if (iso_639_2[i] == NULL) {
        valid = FALSE;
        hbr_error("Value should be an ISO 639-2 code (three letter language code)",
                config_path, group, option->name, NULL);
    }
    return valid;
}

gboolean valid_iso_639_list(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_iso639_list: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}


gboolean valid_native_dub(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_native_dub: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_subtitle(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = FALSE;
    GError *error = NULL;
    gchar *value = g_key_file_get_value(config, group, option->name, &error);
    if (error != NULL) {
        hbr_error("Invalid subtitle. Should be \"none\", \"scan\", or"
                " a comma-separated list of track numbers",
                config_path, group, option->name, value);
        g_error_free(error);
    } else {
        if (strcmp(value, "scan") == 0 || strcmp(value, "none") == 0) {
            valid = TRUE;
        } else {
            error = NULL;
            gsize subtitle_count = 0;
            gint *subtitles = g_key_file_get_integer_list(config, group,
                    option->name, &subtitle_count, &error);
            if (error == NULL) {
                valid = TRUE;
            } else {
                g_error_free(error);
            }
            g_free(subtitles);
        }
    }
    if (!valid) {
        hbr_error("Invalid subtitle. Should be \"none\", \"scan\", or"
                " a comma-separated list of track numbers",
                config_path, group, option->name, value);
    }
    g_free(value);
    return valid;
}

gboolean valid_gain(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;
    valid = valid_double_list(option, group, config, config_path);

    GError *error = NULL;
    gsize gain_count = 0;
    gdouble *gains = g_key_file_get_double_list(config, group, option->name,
            &gain_count, &error);
    if (error != NULL) {
        hbr_error(error->message, config_path, group, option->name, NULL);
        g_error_free(error);
    }
    for (int i=0; i<gain_count; i++) {
        if (gains[i] < -20.0 || gains[i] > 20.0) {
            gchar *gain = g_strdup_printf("%f", gains[i]);
            hbr_warn("Gain value exceeds +-20dB", config_path, group,
                    option->name, gain);
            g_free(gain);
        }
    }
    g_free(gains);
    // verify gain_count with audio count
    error = NULL;
    gsize audio_count = 0;
    if (g_key_file_has_key(config, group, "audio", NULL)) {
        gint *audio_tracks = g_key_file_get_integer_list(config, group,
                "audio", &audio_count, &error);
        if (error != NULL) {
            hbr_error(error->message, config_path, group, option->name, NULL);
            g_error_free(error);
        }
        g_free(audio_tracks);
    }
    gchar *gains_string = g_key_file_get_value(config, group, option->name,
            NULL);
    if ( gain_count != 1 && gain_count != audio_count) {
        hbr_error("Number of audio tracks (%d) specified does not match the"
                " number of gain tracks (%d)", config_path, group,
                option->name, gains_string, audio_count, gain_count);
        valid = FALSE;
    }
    g_free(gains_string);
    return valid;
}

gboolean valid_drc(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean all_valid = TRUE;
    // try to parse as double list
    gsize drc_count = 0;
    GError *error = NULL;
    gdouble* drc_list = g_key_file_get_double_list(config, group, option->name,
            &drc_count, &error);
    gchar *value = g_key_file_get_value(config, group, option->name, NULL);
    if (error != NULL) {
        all_valid = FALSE;
        gchar *value = g_key_file_get_value(config, group, option->name, NULL);
        if (g_error_matches(error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_INVALID_VALUE)) {
            hbr_error("Value should be decimal number", config_path, group,
                    option->name, value);
        } else {
            // use glib error message for non-user error
            hbr_error(error->message, config_path, group, option->name, value);
        }
        g_error_free(error);
    } else {
        // check range on list items (valid range for dynamic compression is 1.0-4.0)
        for (int i = 0; i < drc_count; i++) {
            if (drc_list[i] <= 1.0 || drc_list[i] >= 4.0) {
                hbr_error("DRC value is outside range 1.0 - 4.0:", config_path,
                        group, option->name, value);
            }
        }
        /* check drc_count == audio_count, but only warn
         * HandBrake behaviour at time of writing was:
         *     audio_count > drc_count: doesn't set drc for last tracks
         *     drc_count > audio_count: drops extra drc
         *     drc_count = 1          : drc is applied to all tracks
         */
        if (drc_count > 1 && g_key_file_has_key(config, group, "audio", NULL)) {
            gsize audio_count = 0;
            g_strfreev(g_key_file_get_string_list(config, group, "audio",
                        &audio_count, NULL));
            if (audio_count > drc_count) {
                hbr_warn("DRC was not specified for all audio tracks:",
                        config_path, group, option->name, value);
            }
            if (audio_count < drc_count) {
                hbr_warn("More DRC values specified than audio tracks:",
                        config_path, group, option->name, value);
            }
        }
    }
    g_free(value);
    g_free(drc_list);
    return all_valid;
}

gboolean valid_chapters(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    gboolean valid = TRUE;
    GError *error = NULL;
    // change separator temporarily to parse dash separated list
    g_key_file_set_list_separator(config, '-');
    gsize chapter_range_count;
    gint *chapter_range = g_key_file_get_integer_list(config, group,
            option->name, &chapter_range_count, &error);
    g_key_file_set_list_separator(config, ',');
    gchar *value = g_key_file_get_value(config, group, option->name, NULL);

    if ((chapter_range == NULL && error != NULL) || chapter_range_count > 2 ||
            chapter_range_count < 1) {
        valid = FALSE;
        hbr_error("Value should be a chapter number or range of chapter numbers"
                " (e.g. 1-12)", config_path, group, option->name, value);
        g_error_free(error);
    }
    if (chapter_range != NULL) {
        if (chapter_range_count == 1 || chapter_range_count == 2) {
            if (chapter_range[0] < 1 || chapter_range[0] > 100) {
                hbr_error("Chapter numbers should be in range 1-100", config_path,
                        group, option->name, value);
            }
        }
        if (chapter_range_count == 2) {
            if (chapter_range[1] < 1 || chapter_range[1] > 100) {
                hbr_error("Chapter numbers should be in range 1-100", config_path,
                        group, option->name, value);
            }
            if (chapter_range[0] > chapter_range[1]) {
                hbr_error("First chapter number should be smaller than second",
                        config_path, group, option->name, value);
            }
        }
    }

    g_free(chapter_range);
    g_free(value);
    return valid;
}

gboolean valid_encopts(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_encopts: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_encoder_preset(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;

    const gchar *group_1_encoder[] = { "x264", "x264_10bit", "x265",
        "x265_10bit", "x265_12bit", NULL };
    const gchar *group_1_presets[] = {"ultrafast", "superfast", "veryfast",
        "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo",
        NULL };

    const gchar *group_2_encoder[] = { "VP8", "VP9", NULL };
    const gchar *group_2_presets[] = { "veryfast", "faster", "fast", "medium",
        "slow", "slower", "veryslow", NULL };

    gboolean group_1_valid = FALSE, group_2_valid = FALSE;
    gchar *preset = NULL;
    GError *error = NULL;
    gchar *encoder = g_key_file_get_value(config, group, "encoder", &error);
    if (error != NULL) {
        hbr_error("Could not verify encoder preset because video encoder was"
                " not specified", config_path, group, option->name, NULL);
        valid = FALSE;
        g_error_free(error);
    } else {
        g_strstrip(encoder);

        error = NULL;
        preset = g_key_file_get_value(config, group, option->name, &error);
        if (error != NULL) {
            hbr_error(error->message, config_path, group, option->name, NULL);
            valid = FALSE;
            g_error_free(error);
        } else {
            g_strstrip(preset);

            if (g_strv_contains(group_1_encoder, encoder)) {
                if (g_strv_contains(group_1_presets, preset)) {
                    group_1_valid = TRUE;
                }
            }
            if (g_strv_contains(group_2_encoder, encoder)) {
                if (g_strv_contains(group_2_presets, preset)) {
                    group_2_valid = TRUE;
                }
            }
        }
    }
    if (!group_1_valid && !group_2_valid) {
        if (encoder != NULL && preset != NULL) {
            hbr_error("Invalid encoder preset for encoder (%s)", config_path,
                    group, option->name, preset, encoder);
        } else {
            hbr_error("Invalid encoder preset", config_path, group, option->name,
                    NULL);
        }
    }

    return valid && (group_1_valid || group_2_valid);
}

gboolean valid_encoder_tune(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_encoder_tune: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_encoder_profile(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;

    gboolean valid_profile = FALSE;
    gchar *profile = NULL;
    GError *error = NULL;
    gchar *encoder = g_key_file_get_value(config, group, "encoder", &error);
    if (error != NULL) {
        hbr_error("Could not verify encoder profile because video encoder was"
                " not specified", config_path, group, option->name, NULL);
        valid = FALSE;
        g_error_free(error);
    } else {
        g_strstrip(encoder);

        error = NULL;
        profile = g_key_file_get_value(config, group, option->name, &error);
        if (error != NULL) {
            hbr_error(error->message, config_path, group, option->name, NULL);
            valid = FALSE;
            g_error_free(error);
        } else {
            g_strstrip(profile);

            if (strcmp(encoder, "x264") == 0) {
                const gchar *profile_list[] = {"auto", "high", "main", "baseline", NULL};
                if (g_strv_contains(profile_list, profile)) {
                    valid_profile = TRUE;
                }
            } else  if (strcmp(encoder, "x264_10bit") == 0) {
                const gchar *profile_list[] = {"auto", "high10", NULL};
                if (g_strv_contains(profile_list, profile)) {
                    valid_profile = TRUE;
                }
            } else if (strcmp(encoder, "x265") == 0) {
                const gchar *profile_list[] = {"auto", "main", "mainstillpicture", NULL};
                if (g_strv_contains(profile_list, profile)) {
                    valid_profile = TRUE;
                }
            } else if (strcmp(encoder, "x265_10bit") == 0) {
                const gchar *profile_list[] = {"auto", "main10", "main10-intra", NULL};
                if (g_strv_contains(profile_list, profile)) {
                    valid_profile = TRUE;
                }
            } else if (strcmp(encoder, "x264_12bit") == 0) {
                const gchar *profile_list[] = {"auto", "main12", "main12-intra", NULL};
                if (g_strv_contains(profile_list, profile)) {
                    valid_profile = TRUE;
                }
            }
        }
    }
    if (!valid_profile) {
        if (encoder != NULL && profile != NULL) {
            hbr_error("Invalid encoder profile for encoder (%s)", config_path,
                    group, option->name, profile, encoder);
        } else {
            hbr_error("Invalid encoder profile", config_path, group, option->name,
                    NULL);
        }
    }
    return valid && valid_profile;
}

gboolean valid_encoder_level(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_encoder_level: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_nlmeans(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_nlmeans: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_nlmeans_tune(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_nlmeans_tune: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_dither(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid = TRUE;
    valid = valid_string_list_set(option, group, config, config_path);

    GError *error = NULL;
    gsize dither_count = 0;
    gchar **dithers = g_key_file_get_string_list(config, group, option->name,
            &dither_count, &error);
    g_strfreev(dithers);
    // get the audio count, verify same count
    error = NULL;
    gsize audio_count = 0;
    if (g_key_file_has_key(config, group, "audio", NULL)) {
        gint *audio_tracks = g_key_file_get_integer_list(config, group,
                "audio", &audio_count, &error);
        if (error != NULL) {
            hbr_error(error->message, config_path, group, option->name, NULL);
            g_error_free(error);
        }
        g_free(audio_tracks);
    }
    gchar *gains_string = g_key_file_get_value(config, group, option->name,
            NULL);
    if (dither_count != 1 && dither_count != audio_count) {
        hbr_error("Number of audio tracks (%d) specified does not match the"
                " number of dither tracks (%d)", config_path, group,
                option->name, gains_string, audio_count, dither_count);
        valid = FALSE;
    }
    g_free(gains_string);

    return valid;
}

gboolean valid_subtitle_forced(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_subtitle_forced: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_subtitle_burned(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_subtitle_burned: %s\n", option->name);  //TODO REMOVE
    // only applies to 1.2.0 and newer, otherwise it's just an integer
    return FALSE; //TODO incomplete
}

gboolean valid_subtitle_default(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_subtitle_default: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_codeset(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_codeset: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_rotate(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid_boolean = TRUE, valid_custom = TRUE;
    // check for boolean value
    GError *error = NULL;
    g_key_file_get_boolean(config, group, option->name, &error);
    if (error != NULL) {
        valid_boolean = FALSE;
        g_error_free(error);
    }
    // check for custom format
    error = NULL;
    gsize filter_count = 0;

    g_key_file_set_list_separator(config, ':');
    gchar **filters = g_key_file_get_string_list(config, group,
            option->name, &filter_count, &error);
    g_key_file_set_list_separator(config, ',');

    gchar *filter_names[] = { "angle", "hflip", "disable", NULL};
    if (error != NULL) {
        valid_custom = FALSE;
        hbr_error(error->message, config_path, group, option->name, NULL);
        g_error_free(error);
    } else {
        for (int i = 0; i < filter_count; i++) {
            g_strstrip(filters[i]);
            // split string on '=' into 2 tokens
            gchar **tokens = g_strsplit(filters[i], "=", 2);
            if (tokens[0] == NULL) {
                valid_custom = FALSE;
            } else if (tokens[1] == NULL) {
                valid_custom = FALSE;
                g_strfreev(tokens);
            } else {
                g_strstrip(tokens[0]);
                g_strstrip(tokens[1]);
                // verify first token is one of filter_names
                int j = 0;
                while (filter_names[j] != NULL) {
                    if (strcmp(tokens[0], filter_names[j]) == 0) {
                        break;
                    }
                    j++;
                }
                // got to end of string without finding valid name
                if (filter_names[j] == NULL) {
                    valid_custom = FALSE;
                }
                if (strcmp(tokens[0], "angle") == 0) {
                    if (strcmp(tokens[1], "0") != 0 &&
                            strcmp(tokens[1], "90") != 0 &&
                            strcmp(tokens[1], "180") != 0 &&
                            strcmp(tokens[1], "270") != 0) {
                        valid_custom = FALSE;
                    }

                } else if (strcmp(tokens[0], "hflip") == 0) {
                    if (strcmp(tokens[1], "0") != 0 &&
                            strcmp(tokens[1], "1") != 0) {
                        valid_custom = FALSE;
                    }
                } else if (strcmp(tokens[0], "disable") == 0) {
                    if (strcmp(tokens[1], "0") != 0 &&
                            strcmp(tokens[1], "1") != 0) {
                        valid_custom = FALSE;
                    }
                } else {
                    valid_custom = FALSE;
                }
                g_strfreev(tokens);
            }
        }
    }
    g_strfreev(filters);

    gchar* value = g_key_file_get_string(config, group, option->name, &error);
    if (!valid_boolean && !valid_custom) {
        hbr_error("Invalid rotate option", config_path, group, option->name,
                value);
    }
    g_free(value);
    return (valid_boolean || valid_custom);
}

gboolean valid_qsv_decoding(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_qsv_decoding: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_comb_detect(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    gboolean valid_boolean = TRUE, valid_preset = TRUE, valid_custom = TRUE;
    // check for boolean value
    GError *error = NULL;
    g_key_file_get_boolean(config, group, option->name, &error);
    if (error != NULL) {
        valid_boolean = FALSE;
        g_error_free(error);
    }

    // check for presets (permissive, fast)
    error = NULL;
    gchar* value = g_key_file_get_string(config, group, option->name, &error);
    if (error == NULL) {
        if (strcmp(value, "permissive") != 0 && strcmp(value, "fast") != 0
                && strcmp(value, "default") != 0 && strcmp(value, "off") != 0
           ) {
            valid_preset = FALSE;
        }
    } else {
        valid_preset = FALSE;
        g_error_free(error);
    }

    // check for filter
    // change separator temporarily to parse colon separated list
    g_key_file_set_list_separator(config, ':');
    error = NULL;
    gsize filter_count = 0;
    gchar **filters = g_key_file_get_string_list(config, group,
            option->name, &filter_count, &error);
    gchar *filter_names[] = {"mode", "spatial-metric", "motion-thresh",
        "spatial-thresh", "filter-mode", "block-thresh", "block-width",
        "block-height", "disable", NULL};
    if (error != NULL) {
        valid_custom = FALSE;
        g_error_free(error);
    } else {
        for (int i = 0; i < filter_count; i++) {
            g_strstrip(filters[i]);
            // split string on '=' into 2 tokens
            gchar **tokens = g_strsplit(filters[i], "=", 2);
            if (tokens[0] == NULL) {
                valid_custom = FALSE;
            } else if (tokens[1] == NULL) {
                valid_custom = FALSE;
                g_strfreev(tokens);
            } else {
                // verify first token is one of filter_names
                int j = 0;
                while (filter_names[j] != NULL) {
                    if (strcmp(tokens[0], filter_names[j]) == 0) {
                        break;
                    }
                    j++;
                }
                if (filter_names[j] == NULL) {
                    valid_custom = FALSE;
                }
                // verify second token is a valid integer
                int k = 0;
                while (tokens[1][k] != '\0') {
                    if (!g_ascii_isdigit(tokens[1][k])) {
                        valid_custom = FALSE;
                    }
                    k++;
                }
                gchar *endptr = NULL;
                gint64 number = g_ascii_strtoll(tokens[1], &endptr, 10);
                if ((number == G_MAXINT64 || number == G_MININT64)
                        && errno == ERANGE) {
                    // value would cause overflow
                    valid_custom = FALSE;
                }
                if (number == 0 && endptr == tokens[1]) {
                    // string conversion failed
                    valid_custom = FALSE;
                }
                g_strfreev(tokens);
            }
        }
    }
    g_strfreev(filters);
    // reset separator
    g_key_file_set_list_separator(config, ',');

    if (!valid_boolean && !valid_preset && !valid_custom) {
        hbr_error("Invalid decomb option", config_path, group, option->name,
                value);
    }
    g_free(value);
    return (valid_boolean || valid_preset || valid_custom);
}

gboolean valid_pad(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_pad: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_unsharp(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_unsharp: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_filespec(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_filespec: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_preset_name(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    g_print("valid_preset_name: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}
