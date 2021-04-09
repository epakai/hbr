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

#include <assert.h>     // for assert
#include <errno.h>      // for ERANGE, errno
#include <glib/gstdio.h>// for g_access
#include <math.h>       // for fabs
#include <stdio.h>      // for NULL
#include <string.h>     // for strnlen, strrchr, strncmp, strcmp
#include <sys/param.h>  // for MAXPATHLEN

#include "util.h"
#include "validate.h"
#include "keyfile.h"

extern option_data_t option_data;

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
    // check file is readable
    GDataInputStream * datastream = open_datastream(infile);

    if (!datastream) {
        return FALSE;
    }

    gboolean valid = TRUE;
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
        return FALSE; // errors printed during has_required_keys()
    }

    // check required keys exist (requires)
    if (!has_requires(input_keyfile, infile, config_keyfile)) {
        return FALSE; // errors printed during has_requires()
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
        // TODO
    }
    // TODO check keys that don't belong in local config
    if (checking_local_config) {
        // TODO
    }
    // TODO check required keys exist (there aren't really any, but having none
    //      between hbr.conf and infile is not workable)

    /* TODO check depends (done without merging, depends are cases where one
     * option's value affects which values are valid for another option
     */

    // Make sure merging is necessary before implementing any merge.
    // It would be preferable to avoid it here if at all possible.

    // TODO check quantity matches (can be handled with merging)
    // TODO check negation type conflicts (requires merging for local configs)

    /* TODO check conflicts (only requires checking each group because the function
     * merge_key_group removes conflicts during the merge)
     */
    // call valid_option function (requires merging for complex validation)

    i = 0;
    option_t *options = option_data.options;
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
    if (merged_configs == NULL) {
        return FALSE;
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
                        g_hash_table_lookup(option_data.options_index,
                            keys[j]));
                if (option_data.options[option_index].key_type == k_boolean &&
                        option_data.options[option_index].negation_option) {
                    if (g_key_file_get_boolean(test_keyfile, group_names[i],
                                keys[j], NULL) == FALSE) {
                        j++;
                        continue;
                    }
                }
                GSList* requires_list =
                    g_hash_table_lookup(option_data.requires_index, keys[j]);
                /* iterate over gslist (pointers are actually int which
                   are the index to the requires table) */
                while (requires_list != NULL) {
                    gint index = GPOINTER_TO_INT(requires_list->data);
                    // check if require is defined
                    if (!g_key_file_has_key(test_keyfile, group_names[i],
                                option_data.requires[index].require_name,
                                NULL)) {
                        gchar *value = g_key_file_get_value(
                                test_keyfile, group_names[i], keys[j], NULL);
                        hbr_error("Key \"%s\" requires \"%s\" but it is not"
                                " set", infile, group_names[i], keys[j], value,
                                keys[j], option_data.requires[index].require_name);
                        valid = FALSE;
                        g_free(value);
                    } else {
                        // if require has specific value check it
                        gchar *requires_value = g_key_file_get_value(
                                test_keyfile, group_names[i],
                                option_data.requires[index].require_name, NULL);
                        if (option_data.requires[index].require_value != NULL &&
                                strcmp(requires_value,
                                    option_data.requires[index].require_value) != 0) {
                            hbr_error("Key \"%s\" requires setting \"%s=%s\"",
                                    infile, group_names[i], keys[j], NULL,
                                    keys[j], option_data.requires[index].require_name,
                                    option_data.requires[index].require_value);
                            valid = FALSE;
                        }
                        g_free(requires_value);
                    }
                    requires_list = requires_list->next;
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
    if (merged_configs == NULL) {
        return FALSE;
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
            if (!g_hash_table_contains(option_data.options_index, keys[j])) {
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

/**
 * @brief Validates custom formats used for some HandBrakeCLI options
 *
 * @param config Keyfile to pull custom_format from
 * @param group  group to pull custom_format from
 * @param option option type to pull custom_format from
 *
 * @return true when all keys in custom_format are parts of keys, and all
 *         values can be interpreted as the appropriate key_type
 */
gboolean check_custom_format (GKeyFile *config, const gchar *group, option_t *option,
        const gchar *config_path)
{
    gint custom_index = GPOINTER_TO_INT(
            g_hash_table_lookup(option_data.customs_index, option->name));
    gboolean valid = TRUE;
    g_key_file_set_list_separator(config, ':');
    GError *error = NULL;
    gsize key_count = 0;
    gchar **custom_format = g_key_file_get_string_list(config, group,
            option->name, &key_count, &error);
    if (error != NULL) {
        valid = FALSE;
        g_error_free(error);
    } else {
        for (gsize i = 0; i < key_count; i++) {
            g_strstrip(custom_format[i]);
            // split string on '=' into 2 tokens
            gchar **tokens = g_strsplit(custom_format[i], "=", 2);
            if (tokens[0] == NULL) {
                valid = FALSE;
            } else if (tokens[1] == NULL) {
                valid = FALSE;
                g_strfreev(tokens);
            } else {
                // verify first token is one of filter_names
                int j = 0;
                custom_key_t *custom_keys =
                    (custom_key_t *) option_data.customs[custom_index].key;
                while (custom_keys[j].key_name != NULL) {
                    if (strcmp(tokens[0], custom_keys[j].key_name) == 0) {
                        break;
                    }
                    j++;
                }
                if (custom_keys[j].key_name == NULL) {
                    // got to the end without token 0 matching a custom key
                    hbr_error("Unsupported custom filter", config_path, group,
                            option->name, custom_format[i]);
                    valid = FALSE;
                    continue;
                }
                // verify second token is a valid type
                switch (custom_keys[j].key_type) {
                    case k_integer:
                        {
                            int k = 0;
                            while (tokens[1][k] != '\0') {
                                if (!g_ascii_isdigit(tokens[1][k])) {
                                    valid = FALSE;
                                }
                                k++;
                            }
                            gchar *endptr = NULL;
                            gint64 number = g_ascii_strtoll(tokens[1], &endptr, 10);
                            if ((number == G_MAXINT64 || number == G_MININT64)
                                    && errno == ERANGE) {
                                // value would cause overflow
                                valid = FALSE;
                            }
                            if (number == 0 && endptr == tokens[1]) {
                                // string conversion failed
                                valid = FALSE;
                            }
                            break;
                        }
                    default:
                        hbr_error("Incomplete implementation of "
                                "validate.c:check_custom_format()",
                                NULL, NULL, NULL, NULL);
                }
                g_strfreev(tokens);
            }
        }
    }
    g_strfreev(custom_format);
    // reset separator
    g_key_file_set_list_separator(config, ',');
    return valid;
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

gboolean valid_type(option_t *option, const gchar *group, GKeyFile *config,
        const gchar *config_path)
{
    // check type is series or movie (defined in option_t for "type")
    // valid_string_list_set() will print errors
    gboolean valid = valid_string_list_set(option, group, config, config_path);
    gchar *type = g_key_file_get_value(config, group, option->name, NULL);
    gboolean has_year = g_key_file_has_key(config, group, "year", NULL);
    gboolean has_season = g_key_file_has_key(config, group, "season", NULL);
    gboolean has_episode = g_key_file_has_key(config, group, "episode", NULL);

    /* Nothing below invalidates the key, we just present warnings where a
     * user may forget options that affect the filename
     */

    // cases where relevant options are all defined in the same group
    if (strcmp(type, "series") == 0) {
        if (has_season && has_episode) {
            g_free(type);
            return valid;
        }
    } else if (strcmp(type, "movie") == 0) {
        if (has_year) {
            g_free(type);
            return valid;
        }
    }

    if (strcmp(group, "CONFIG") == 0) {
        type_config_warnings(type, has_season, has_episode, has_year, config,
                config_path);
    }

    if (strncmp(group, "OUTFILE", 7) == 0) {
        type_outfile_warnings(type, has_season, has_episode, has_year, group,
                config, config_path);
    }

    g_free(type);
    return valid;
}

/**
 * @brief Print warnings related to type key in CONFIG sections
 *
 * @param type value of type key (movie/series)
 * @param has_season season key is defined
 * @param has_episode episode key is defined
 * @param has_year year key is defined
 * @param config KeyFile to pull values from
 * @param config_path KeyFile path for error printing
 */
void type_config_warnings(gchar *type, gboolean has_season,
        gboolean has_episode, gboolean has_year, GKeyFile *config,
        const gchar *config_path) {
    int i = 0;
    gchar **group_names = (g_key_file_get_groups(config, NULL));
    while (group_names[i] != NULL) {
        if (strncmp(group_names[i], "OUTFILE", 7) == 0) {
            // if outfile has type defined, ignore it, valid_type will
            // be called on it later
            if (g_key_file_has_key(config, group_names[i], "type", NULL)) {
                i++;
                continue;
            }
            // check that season/episode is defined for series
            if (strcmp(type, "series") == 0) {
                gboolean outfile_has_season = (has_season ||
                        g_key_file_has_key(config, group_names[i], "season", NULL));
                gboolean outfile_has_episode = (has_episode ||
                        g_key_file_has_key(config, group_names[i], "episode", NULL));
                if (!outfile_has_season && !outfile_has_episode) {
                    hbr_warn("Season and episode number not specified",
                            config_path, group_names[i], NULL, NULL);
                } else {
                    if (!outfile_has_season) {
                        hbr_warn("Season number not specified", config_path,
                                group_names[i], NULL, NULL);
                    }
                    if (!outfile_has_episode) {
                        hbr_warn("Episode number not specified", config_path,
                                group_names[i], NULL, NULL);
                    }
                }
            }
            // check that year is defined for series
            if (strcmp(type, "movie") == 0) {
                has_year = (has_year ||
                        g_key_file_has_key(config, group_names[i], "year", NULL));
                if (!has_year) {
                    hbr_warn("Year not specified", config_path,
                            group_names[i], NULL, NULL);
                }
            }
        }
        i++;
    }
    g_strfreev(group_names);
}

/**
 * @brief Print warnings related to type key in OUTFILE sections
 *
 * @param type value of type key (movie/series)
 * @param has_season season key is defined
 * @param has_episode episode key is defined
 * @param has_year year key is defined
 * @param group group to pull values from
 * @param config KeyFile to pull values from
 * @param config_path KeyFile path for error printing
 */
void type_outfile_warnings(gchar *type, gboolean has_season,
        gboolean has_episode, gboolean has_year, const gchar *group,
        GKeyFile *config, const gchar *config_path) {
    if (strcmp(type, "series") == 0) {
        has_season = (has_season ||
                g_key_file_has_key(config, "CONFIG", "season", NULL));
        has_episode = (has_episode ||
                g_key_file_has_key(config, "CONFIG", "episode", NULL));
        if (!has_season && !has_episode) {
            hbr_warn("Season and episode number not specified",
                    config_path, group, NULL, NULL);
        } else {
            if (!has_season) {
                hbr_warn("Season number not specified", config_path,
                        group, NULL, NULL);
            }
            if (!has_episode) {
                hbr_warn("Episode number not specified", config_path,
                        group, NULL, NULL);
            }
        }
    }
    // check that year is defined for series
    if (strcmp(type, "movie") == 0) {
        has_year = (has_year ||
                g_key_file_has_key(config, "CONFIG", "year", NULL));
        if (!has_year) {
            hbr_warn("Year not specified", config_path,
                    group, NULL, NULL);
        }
    }
}

gboolean valid_readable_path(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    gboolean valid = TRUE;
    GError *error = NULL;
    gchar *value = g_key_file_get_value(config, group, option->name, &error);
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

gboolean valid_writable_path(option_t *option, const gchar *group,
        GKeyFile *config, const gchar *config_path)
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

gboolean valid_filename_component(option_t *option, const gchar *group,
        GKeyFile *config,  const gchar *config_path)
{
    gboolean valid = TRUE;
    GError *error = NULL;
    gchar *component = g_key_file_get_value(config, group, option->name, &error);

    if (error) {
        valid = FALSE;
        hbr_error(error->message, config_path, group, option->name, NULL);
    } else {
        gsize component_length = strnlen(component, MAXPATHLEN);
        if ( component_length < 1 || component_length >= MAXPATHLEN) {
            valid = FALSE;
            hbr_error("Invalid component length", config_path, group,
                    option->name, component);
        }
        for (gsize i = 0; i < component_length; i++) {
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

gboolean valid_boolean(option_t *option, const gchar *group, GKeyFile *config,
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

/* Not currently used for anything, do you want valid_positive_integer?
gboolean valid_integer(option_t *option, const gchar *group, GKeyFile *config,
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
*/

gboolean valid_integer_set(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_integer_set: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_integer_list(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    gboolean valid = TRUE;
    GError *error = NULL;
    gsize count;
    gint *values = g_key_file_get_integer_list(config, group, option->name,
            &count, &error);
    if (values == NULL || error != NULL) {
        valid = FALSE;
        gchar* string_value = g_key_file_get_value(config, group, option->name, NULL);
        if (g_error_matches(error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_INVALID_VALUE)) {
            hbr_error("Value should be comma-separated integer list", config_path, group,
                    option->name, string_value);
        } else {
            // use glib error message for non-user error
            hbr_error(error->message, config_path, group,
                    option->name, string_value);
        }
        g_error_free(error);
        g_free(string_value);
    }
    g_free(values);
    return valid;
}

gboolean valid_integer_list_set(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_integer_list_set: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_positive_integer(option_t *option, const gchar *group,
        GKeyFile *config, const gchar *config_path)
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

gboolean valid_double_list(option_t *option, const gchar *group,
        GKeyFile *config,  const gchar *config_path) {
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

gboolean valid_positive_double_list(option_t *option, const gchar *group,
        GKeyFile *config,  const gchar *config_path)
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
        for (gsize i = 0; i < value_count; i++) {
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

gboolean valid_string(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    g_print("valid_string: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_string_set(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    gboolean valid = TRUE;
    gchar *value = g_key_file_get_value(config, group, option->name, NULL);
    if (option->valid_values_count == 0) {
        valid = FALSE;
    }
    int i = 0;
    while (i < option->valid_values_count) {
        assert(option->valid_values_count != 0 && option->valid_values != NULL);
        if (strcmp(value, ((gchar **)option->valid_values)[i]) == 0) {
            // found a match, stop searching
            valid = TRUE;
            break;
        } else {
            valid = FALSE;
        }
        i++;
    }
    if (valid == FALSE) {
        hbr_error("Invalid key value", config_path, group, option->name, value);
    }
    g_free(value);
    return valid;
}

/**
 * @brief Check that option consists of a valid list of strings from the
 *        valid_values array
 *
 * @return TRUE when all values come from the valid_values
 */
gboolean valid_string_list_set(option_t *option, const gchar *group, GKeyFile *config,
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
        for (gsize i = 0; i < list_count; i++) {
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

gboolean valid_string_list(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    g_print("valid_string_list: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_filename_exists(option_t *option, const gchar *group, GKeyFile *config,
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

gboolean valid_filename_exists_list(option_t *option, const gchar *group, GKeyFile *config,
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
        for (gsize i = 0; i < filename_count; i++) {
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

gboolean valid_filename_dne(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_filename_dne: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_startstop_at(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_startstop_at: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_previews(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_previews: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_audio(option_t *option, const gchar *group, GKeyFile *config,
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
        gsize i = 0;
        gint temp;
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

gboolean valid_audio_encoder(option_t *option, const gchar *group, GKeyFile *config,
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
    /* TODO:
     * I ignored when audio_encoder_count is 1, but in this case HandBrakeCLI
     * just uses its default encoder for all subsequent audio tracks. My desired
     * behavior is to use the singly specified encoder for ALL tracks
     * I don't want to modify values during validate though. It should probably
     * happen elsewhere, but where?
     */
    if (audio_encoder_count != 1 && audio_encoder_count != audio_count) {
        hbr_error("Number of audio encoders (%lu) specified does not match the"
                " number of audio tracks (%lu)", config_path, group,
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

gboolean valid_audio_quality(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_audio_quality: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_audio_bitrate(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    if (!valid_integer_list(option, group, config, config_path)) {
        return FALSE;
    }

    // this list must be ordered. we use first and last index later for bounds
    int valid_bitrates[] = {6, 12, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128,
        160, 192, 224, 256, 320, 384, 448, 512, 576, 640, 768, 960, 1152, 1344,
        1536, 2304, 3072, 4608, 6144};
    int valid_bitrates_count = sizeof(valid_bitrates)/sizeof(valid_bitrates[0]);

    // get bitrates
    gsize bitrate_count, aencoder_count;
    // errors ignored because valid_integer_list above should fail and return
    gint *bitrates = g_key_file_get_integer_list(config, group, option->name,
            &bitrate_count, NULL);
    gchar *bitrates_string = g_key_file_get_value(config, group,
            option->name, NULL);

    gboolean valid = TRUE;
    GError *error = NULL;
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
        hbr_error("Number of track bitrates (%lu) specified does not match the"
                " number of audio tracks (%lu)", config_path, group,
                option->name, bitrates_string, bitrate_count, audio_count);
        valid = FALSE;
    }
    // verify bitrates
    for (gsize i = 0; i < bitrate_count && i < aencoder_count; i++) {
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
        gint lower_bitrate = valid_bitrates[0];
        gint upper_bitrate = valid_bitrates[valid_bitrates_count-1];
        gboolean bounded = FALSE;
        if (strcmp("av_aac", audio_encoders[i]) == 0) {
            lower_bitrate = 64;
            upper_bitrate = 512;
            bounded = TRUE;
        } else if (strcmp("ac3", audio_encoders[i]) == 0) {
            lower_bitrate = 96;
            upper_bitrate = 640;
            bounded = TRUE;
        } else if (strcmp("eac3", audio_encoders[i]) == 0) {
            lower_bitrate = 96;
            upper_bitrate = 6144;
            bounded = TRUE;
        } else if (strcmp("mp3", audio_encoders[i]) == 0) {
            lower_bitrate = 12;
            upper_bitrate = 320;
            bounded = TRUE;
        } else if (strcmp("vorbis", audio_encoders[i]) == 0) {
            lower_bitrate = 32;
            upper_bitrate = 448;
            bounded = TRUE;
        } else if (strcmp("opus", audio_encoders[i]) == 0) {
            lower_bitrate = 12;
            upper_bitrate = 512;
            bounded = TRUE;
        }
        if (bounded) {
            if (bitrates[i] < lower_bitrate || bitrates[i] > upper_bitrate) {
                gchar *bad_bitrate = g_strdup_printf("%d", bitrates[i]);
                hbr_error("Bitrate outside range [%d,%d] for encoder %s",
                        config_path, group, option->name, bad_bitrate,
                        lower_bitrate, upper_bitrate, audio_encoders[i]);
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

gboolean valid_audio_compression(option_t *option, const gchar *group, GKeyFile *config,
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
        g_free (compressions_value);
        g_free(encoders);
        g_error_free(error);
        return FALSE;
    }
    if ( compression_count != 1 && compression_count != encoder_count) {
        // error on mismatched counts
        hbr_error("Number of compression values (%lu) specified does not match"
                " the number of audio encoders (%lu)", config_path, group,
                option->name, compressions_value, compression_count,
                encoder_count);
        valid = FALSE;
    } else {
        gsize count = 0, i = 0, zero = 0;
        gsize *j;
        if (compression_count == 1 && encoder_count > 1) {
            //try to validate compression against all encoders
            count = encoder_count;
            j = &zero; // always reference first compression
        } else {
            // try to validate each compression against each encoder
            count = compression_count;
            j = &i; // reference compression with same index as encoder
        }
        for (i = 0; i < count; i++) {
            g_strstrip(encoders[i]);
            gdouble lower_compression = 0.0, upper_compression = 0.0;
            gboolean compressible = FALSE;
            gchar *value = g_strdup_printf("%f", compressions[*j]);
            if (fabs(compressions[*j] - (-1.0)) < 0.001) {
                // default value, ignored
            } else if (strcmp(encoders[i], "flac") == 0 ||
                    strcmp(encoders[i], "flac24") == 0) {
                lower_compression = 0.0;
                upper_compression = 12.0;
                compressible = TRUE;
            } else if (strcmp(encoders[i], "mp3") == 0) {
                lower_compression = 0.0;
                upper_compression = 9.0;
                compressible = TRUE;
            } else if (strcmp(encoders[i], "opus") == 0) {
                lower_compression = 0.0;
                upper_compression = 10.0;
                compressible = TRUE;
            } else {
                valid = FALSE;
                hbr_error("Compression value cannot apply to encoder %s",
                        config_path, group, option->name, value, encoders[i]);
            }
            if ((compressions[*j] < lower_compression ||
                    compressions[*j] > upper_compression) && compressible) {
                valid = FALSE;
                hbr_error("Compression value outside range for %s [%f,%f]",
                        config_path, group, option->name, value, encoders[i],
                        lower_compression, upper_compression);
            }
            g_free(value);
        }
    }
    g_free(compressions_value);
    g_free(compressions);
    g_free(encoders);
    return valid;
}

gboolean valid_video_quality(option_t *option, const gchar *group, GKeyFile *config,
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
        gint lower_value = 0, upper_value = 0;
        if (strcmp(encoder, "x264") == 0 || strcmp(encoder, "x265") == 0 ) {
            lower_value = 0;
            upper_value = 51;
        } else if (strcmp(encoder, "x264_10bit") == 0 ||
                   strcmp(encoder, "x265_10bit") == 0 ) {
            lower_value = -12;
            upper_value = 51;
        } else if (strcmp(encoder, "x265_12bit") == 0 ) {
            lower_value = -24;
            upper_value = 51;
        } else if (strcmp(encoder, "mpeg4") == 0 ||
                   strcmp(encoder, "mpeg2") == 0 ) {
            lower_value = 1;
            upper_value = 31;
        } else if (strcmp(encoder, "VP8") == 0 ||
                   strcmp(encoder, "VP9") == 0 ||
                   strcmp(encoder, "theora") == 0 ) {
            lower_value = 0;
            upper_value = 63;
        }
        if (value < lower_value || value > upper_value) {
            valid = FALSE;
            hbr_error("Value outside range [%d,%d] for encoder %s",
                      config_path, group, option->name, string,
                      lower_value, upper_value, encoder);
        }
    }
    g_free(encoder);
    g_free(string);
    return valid;
}

gboolean valid_video_bitrate(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    gboolean valid = TRUE;
    GError *error = NULL;
    gint value = g_key_file_get_integer(config, group, option->name, &error);
    if (value == 0 && error != NULL) {
        valid = FALSE;
        gchar* string_value = g_key_file_get_value(config, group, option->name, NULL);
        hbr_error("Value should be integer in range [0,1000000]", config_path,
                group, option->name, string_value);
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

gboolean valid_video_framerate(option_t *option, const gchar *group,
        GKeyFile *config, const gchar *config_path)
{
    g_print("valid_video_framerate: %s\n", option->name);  //TODO REMOVE
    // try to interpret it with valid_string_set style lookup
    // if that fails grab a double and verify in range 1-1000
    return FALSE; //TODO incomplete
}

gboolean valid_chroma(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_chroma: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_crop(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    /* default is to autocrop
     * to disable crop should be 0:0:0:0
     * to override set top:bottom:left:right
     */

    /* HandBrakeCLI behavior is to autocrop when --crop= is specified
     * but this may be accidental and could change (--crop alone causes no crop)
     * currently relying on this behavior by passing the empty key through to
     * override CONFIG level crop.
     * It might be prudent to just drop the argument, but that's extra complexity
     * in build args so I'm ok with this.
     */

    // accept empty key
    GError *error = NULL;
    gchar* value = g_key_file_get_string(config, group, option->name, &error);
    if (error == NULL && strcmp(value, "") == 0) {
        g_free(value);
        return TRUE;
    }

    // check crop values
    error = NULL;
    gsize crop_count;
    g_key_file_set_list_separator(config, ':');
    gint *crop = g_key_file_get_integer_list(config, group, option->name,
            &crop_count, &error);
    g_key_file_set_list_separator(config, ',');
    if (error != NULL) {
        g_error_free(error);
    } else {
        if (crop_count == 4 &&
                crop[0] >= 0 && crop[1] >= 0 && crop[2] >= 0 && crop[3] >= 0) {
            g_free(crop);
            return TRUE;
        }
    }
    g_free(crop);

    hbr_error("Crop should be 4 colon separated positive integers"
            " (top:bottom:left:right), or empty for autocrop.",
            config_path, group, option->name, value);
    g_free(value);

    return FALSE;
}

gboolean valid_pixel_aspect(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_pixel_aspect: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_combined_decomb_deblock_deinterlace_comb_detect(option_t *option,
    const gchar *group, GKeyFile *config, const gchar *config_path)
{
    // check for boolean value
    GError *error = NULL;
    g_key_file_get_boolean(config, group, option->name, &error);
    if (error != NULL) {
        g_error_free(error);
    } else {
        return TRUE;
    }

    // check for filter
    if (check_custom_format(config, group, option, config_path)) {
        return TRUE;
    }

    // check for presets (set in handbrake/options*.h)
    // we do the preset check last so that it does not give errors if
    // the key value is a boolean or custom format
    if (valid_string_set(option, group, config, config_path)) {
        return TRUE;
    }

    return FALSE;
}

gboolean valid_decomb(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    gboolean valid = valid_combined_decomb_deblock_deinterlace_comb_detect(
            option, group, config, config_path);

    if (!valid) {
        GError *error = NULL;
        gchar* value = g_key_file_get_string(config, group, option->name, &error);
        hbr_error("Invalid decomb option", config_path, group, option->name,
                value);
        g_free(value);
    }

    return valid;
}

gboolean valid_denoise(option_t *option, const gchar *group, GKeyFile *config,
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
    if (error != NULL) {
        valid_custom = FALSE;
        g_error_free(error);
    } else {
        const gchar *filter_names[] = { "y-spatial", "cb-spatial", "cr-spatial",
            "y-temporal", "cb-temporal", "cr-temporal", NULL};
        for (gsize i = 0; i < filter_count; i++) {
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

gboolean valid_deblock(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    gboolean valid = valid_combined_decomb_deblock_deinterlace_comb_detect(
            option, group, config, config_path);

    if (!valid) {
        GError *error = NULL;
        gchar* value = g_key_file_get_string(config, group, option->name, &error);
        hbr_error("Invalid deblock option", config_path, group, option->name,
                value);
        g_free(value);
    }

    return valid;
}

gboolean valid_deinterlace(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    gboolean valid = valid_combined_decomb_deblock_deinterlace_comb_detect(
            option, group, config, config_path);

    if (!valid) {
        GError *error = NULL;
        gchar* value = g_key_file_get_string(config, group, option->name, &error);
        hbr_error("Invalid deinterlace option", config_path, group, option->name,
                value);
        g_free(value);
    }

    return valid;
}

gboolean valid_detelecine(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_detelecine: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

/**
 * @brief Set of iso_639_2 language codes for checking languages against.
 *        Used in valid_iso_639() and valid_iso_639_list
 */
const gchar *iso_639_2[] = {
    "aar", "abk", "ace", "ach", "ada", "ady", "afa", "afh", "afr", "ain", "aka",
    "akk", "alb", "ale", "alg", "alt", "amh", "ang", "anp", "apa", "ara", "arc",
    "arg", "arm", "arn", "arp", "art", "arw", "asm", "ast", "ath", "aus", "ava",
    "ave", "awa", "aym", "aze", "bad", "bai", "bak", "bal", "bam", "ban", "baq",
    "bas", "bat", "bej", "bel", "bem", "ben", "ber", "bho", "bih", "bik", "bin",
    "bis", "bla", "bnt", "bod", "bos", "bra", "bre", "btk", "bua", "bug", "bul",
    "bur", "byn", "cad", "cai", "car", "cat", "cau", "ceb", "cel", "ces", "cha",
    "chb", "che", "chg", "chi", "chk", "chm", "chn", "cho", "chp", "chr", "chu",
    "chv", "chy", "cmc", "cnr", "cop", "cor", "cos", "cpe", "cpf", "cpp", "cre",
    "crh", "crp", "csb", "cus", "cym", "cze", "dak", "dan", "dar", "day", "del",
    "den", "deu", "dgr", "din", "div", "doi", "dra", "dsb", "dua", "dum", "dut",
    "dyu", "dzo", "efi", "egy", "eka", "ell", "elx", "eng", "enm", "epo", "est",
    "eus", "ewe", "ewo", "fan", "fao", "fas", "fat", "fij", "fil", "fin", "fiu",
    "fon", "fra", "fre", "frm", "fro", "frr", "frs", "fry", "ful", "fur", "gaa",
    "gay", "gba", "gem", "geo", "ger", "gez", "gil", "gla", "gle", "glg", "glv",
    "gmh", "goh", "gon", "gor", "got", "grb", "grc", "gre", "grn", "gsw", "guj",
    "gwi", "hai", "hat", "hau", "haw", "heb", "her", "hil", "him", "hin", "hit",
    "hmn", "hmo", "hrv", "hsb", "hun", "hup", "hye", "iba", "ibo", "ice", "ido",
    "iii", "ijo", "iku", "ile", "ilo", "ina", "inc", "ind", "ine", "inh", "ipk",
    "ira", "iro", "isl", "ita", "jav", "jbo", "jpn", "jpr", "jrb", "kaa", "kab",
    "kac", "kal", "kam", "kan", "kar", "kas", "kat", "kau", "kaw", "kaz", "kbd",
    "kha", "khi", "khm", "kho", "kik", "kin", "kir", "kmb", "kok", "kom", "kon",
    "kor", "kos", "kpe", "krc", "krl", "kro", "kru", "kua", "kum", "kur", "kut",
    "lad", "lah", "lam", "lao", "lat", "lav", "lez", "lim", "lin", "lit", "lol",
    "loz", "ltz", "lua", "lub", "lug", "lui", "lun", "luo", "lus", "mac", "mad",
    "mag", "mah", "mai", "mak", "mal", "man", "mao", "map", "mar", "mas", "may",
    "mdf", "mdr", "men", "mga", "mic", "min", "mis", "mkd", "mkh", "mlg", "mlt",
    "mnc", "mni", "mno", "moh", "mon", "mos", "mri", "msa", "mul", "mun", "mus",
    "mwl", "mwr", "mya", "myn", "myv", "nah", "nai", "nap", "nau", "nav", "nbl",
    "nde", "ndo", "nds", "nep", "new", "nia", "nic", "niu", "nld", "nno", "nob",
    "nog", "non", "nor", "nqo", "nso", "nub", "nwc", "nya", "nym", "nyn", "nyo",
    "nzi", "oci", "oji", "ori", "orm", "osa", "oss", "ota", "oto", "paa", "pag",
    "pal", "pam", "pan", "pap", "pau", "peo", "per", "phi", "phn", "pli", "pol",
    "pon", "por", "pra", "pro", "pus", "que", "raj", "rap", "rar", "roa", "roh",
    "rom", "ron", "rum", "run", "rup", "rus", "sad", "sag", "sah", "sai", "sal",
    "sam", "san", "sas", "sat", "scn", "sco", "sel", "sem", "sga", "sgn", "shn",
    "sid", "sin", "sio", "sit", "sla", "slk", "slo", "slv", "sma", "sme", "smi",
    "smj", "smn", "smo", "sms", "sna", "snd", "snk", "sog", "som", "son", "sot",
    "spa", "sqi", "srd", "srn", "srp", "srr", "ssa", "ssw", "suk", "sun", "sus",
    "sux", "swa", "swe", "syc", "syr", "tah", "tai", "tam", "tat", "tel", "tem",
    "ter", "tet", "tgk", "tgl", "tha", "tib", "tig", "tir", "tiv", "tkl", "tlh",
    "tli", "tmh", "tog", "ton", "tpi", "tsi", "tsn", "tso", "tuk", "tum", "tup",
    "tur", "tut", "tvl", "twi", "tyv", "udm", "uga", "uig", "ukr", "umb", "und",
    "urd", "uzb", "vai", "ven", "vie", "vol", "vot", "wak", "wal", "war", "was",
    "wel", "wen", "wln", "wol", "xal", "xho", "yao", "yap", "yid", "yor", "ypk",
    "zap", "zbl", "zen", "zgh", "zha", "zho", "znd", "zul", "zun", "zxx", "zza",
    NULL
};

gboolean valid_iso_639(option_t *option, const gchar *group, GKeyFile *config,
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

gboolean valid_iso_639_list(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_iso639_list: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}


gboolean valid_native_dub(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_native_dub: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_subtitle(option_t *option, const gchar *group, GKeyFile *config,
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

gboolean valid_gain(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    gboolean valid = valid_double_list(option, group, config, config_path);

    GError *error = NULL;
    gsize gain_count = 0;
    gdouble *gains = g_key_file_get_double_list(config, group, option->name,
            &gain_count, &error);
    if (error != NULL) {
        hbr_error(error->message, config_path, group, option->name, NULL);
        g_error_free(error);
    }
    for (gsize i = 0; i < gain_count; i++) {
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
        hbr_error("Number of audio tracks (%lu) specified does not match the"
                " number of gain tracks (%lu)", config_path, group,
                option->name, gains_string, audio_count, gain_count);
        valid = FALSE;
    }
    g_free(gains_string);
    return valid;
}

gboolean valid_drc(option_t *option, const gchar *group, GKeyFile *config,
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
        for (gsize i = 0; i < drc_count; i++) {
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

gboolean valid_chapters(option_t *option, const gchar *group, GKeyFile *config,
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

gboolean valid_encopts(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_encopts: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_encoder_preset(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    gboolean valid = TRUE;

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

            const gchar *group_1_encoder[] = { "x264", "x264_10bit", "x265",
                "x265_10bit", "x265_12bit", NULL };
            if (g_strv_contains(group_1_encoder, encoder)) {
                const gchar *group_1_presets[] = {"ultrafast", "superfast",
                    "veryfast", "faster", "fast", "medium", "slow", "slower",
                    "veryslow", "placebo", NULL };
                if (g_strv_contains(group_1_presets, preset)) {
                    group_1_valid = TRUE;
                }
            }

            const gchar *group_2_encoder[] = { "VP8", "VP9", NULL };
            if (g_strv_contains(group_2_encoder, encoder)) {
                const gchar *group_2_presets[] = { "veryfast", "faster", "fast",
                    "medium", "slow", "slower", "veryslow", NULL };
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

gboolean valid_encoder_tune(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_encoder_tune: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_encoder_profile(option_t *option, const gchar *group, GKeyFile *config,
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

gboolean valid_encoder_level(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_encoder_level: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_nlmeans(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_nlmeans: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_nlmeans_tune(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_nlmeans_tune: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_dither(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    gboolean valid = valid_string_list_set(option, group, config,
            config_path);

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
        hbr_error("Number of audio tracks (%lu) specified does not match the"
                " number of dither tracks (%lu)", config_path, group,
                option->name, gains_string, audio_count, dither_count);
        valid = FALSE;
    }
    g_free(gains_string);

    return valid;
}

gboolean valid_subtitle_forced(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_subtitle_forced: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_subtitle_burned(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_subtitle_burned: %s\n", option->name);  //TODO REMOVE
    // only applies to 1.2.0 and newer, otherwise it's just an integer
    return FALSE; //TODO incomplete
}

gboolean valid_subtitle_default(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_subtitle_default: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_codeset(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_codeset: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_rotate(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    gboolean valid_bool = TRUE, valid_custom = TRUE;
    // check for boolean value
    GError *error = NULL;
    g_key_file_get_boolean(config, group, option->name, &error);
    if (error != NULL) {
        valid_bool = FALSE;
        g_error_free(error);
    }
    // check for custom format
    error = NULL;
    gsize filter_count = 0;

    g_key_file_set_list_separator(config, ':');
    gchar **filters = g_key_file_get_string_list(config, group,
            option->name, &filter_count, &error);
    g_key_file_set_list_separator(config, ',');

    if (error != NULL) {
        valid_custom = FALSE;
        hbr_error(error->message, config_path, group, option->name, NULL);
        g_error_free(error);
    } else {
        //note: we do not use check_custom_format() here because rotate
        //      uses specific values for angle
        const gchar *filter_names[] = { "angle", "hflip", "disable", NULL};
        for (gsize i = 0; i < filter_count; i++) {
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
    if (!valid_bool && !valid_custom) {
        hbr_error("Invalid rotate option", config_path, group, option->name,
                value);
    }
    g_free(value);
    return (valid_bool || valid_custom);
}

gboolean valid_qsv_decoding(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_qsv_decoding: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_comb_detect(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    gboolean valid = valid_combined_decomb_deblock_deinterlace_comb_detect(
            option, group, config, config_path);

    if (!valid) {
        GError *error = NULL;
        gchar* value = g_key_file_get_string(config, group, option->name, &error);
        hbr_error("Invalid comb_detect option", config_path, group, option->name,
                value);
        g_free(value);
    }

    return valid;
}

gboolean valid_pad(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_pad: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_unsharp(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_unsharp: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_filespec(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_filespec: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}

gboolean valid_preset_name(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path)
{
    g_print("valid_preset_name: %s\n", option->name);  //TODO REMOVE
    return FALSE; //TODO incomplete
}
