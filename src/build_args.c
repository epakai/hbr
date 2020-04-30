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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "build_args.h"
#include "validate.h"
#include "handbrake/options-0.9.9.h"
#include "handbrake/options-0.10.0.h"
#include "handbrake/options-0.10.3.h"
#include "handbrake/options-1.0.0.h"
#include "handbrake/options-1.1.0.h"
#include "handbrake/options-1.2.0.h"

/*
 * hbr specific options tables
 * These aren't valid options to pass to HandBrakeCLI.
 * They are used by hbr to generate file names or specify file locations
 */

static option_t hbr_options[] =
{
    { "type", hbr_only, k_string, FALSE, valid_type, 2, (gchar*[]){"series", "movie"}},
    // TODO document add_year (it should add a (year) to the directory name if type is movie)
    { "add_year", hbr_only, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "input_basedir", hbr_only, k_string, FALSE, valid_readable_path, 0, NULL},
    { "output_basedir", hbr_only, k_string, FALSE, valid_writable_path, 0, NULL},
    { "iso_filename", hbr_only, k_string, FALSE, valid_filename_component, 0, NULL},
    { "name", hbr_only, k_string, FALSE, valid_filename_component, 0, NULL},
    { "year", hbr_only, k_integer, FALSE, valid_integer, 0, NULL},
    { "season", hbr_only, k_integer, FALSE, valid_integer, 0, NULL},
    { "episode", hbr_only, k_integer, FALSE, valid_integer, 0, NULL},
    { "specific_name", hbr_only, k_string, FALSE, valid_filename_component, 0, NULL},
    { "preview", hbr_only, k_boolean, FALSE, valid_boolean, 0, NULL },
    // TODO document extra (it causes subdirectories to be created for extras)
    { "extra", hbr_only, k_string, FALSE, valid_string_set, 8,
        (gchar*[]){"behindthescenes", "deleted", "featurette", "interview",
            "scene", "short", "trailer", "other"}},
    { "debug", hbr_only, k_boolean, FALSE, valid_boolean, 0, NULL},
    { NULL, 0, 0, 0, NULL, 0}
};

static require_t hbr_requires[] =
{
    { "extra", "type", "movie"},
    { "year", "type", "movie"},
    { "season", "type", "series"},
    { "episode", "type", "series"},
    { NULL, NULL, NULL}
};

static conflict_t hbr_conflicts[] =
{
    { "add_year", "true", "type", "series" },
    { "type", "series", "add_year", "true" },
    { NULL, NULL, NULL}
};

/**
 * @brief Produce options to be passed to HandBrakeCLI
 *
 * @param config Keyfile to build options from
 * @param group Name of group in keyfile that contains key and values
 * @param quoted Determines if filenames should be quoted (for debug mode)
 *
 * @return GPtrArray pointer with one option per element
 */
GPtrArray * build_args(GKeyFile *config, gchar *group, gboolean quoted)
{
    GPtrArray *args = g_ptr_array_new_full(32, g_free);

    int i = 0;
    while (options[i].name != NULL) {
        // Skip hbr_only keys
        if (options[i].arg_type == hbr_only) {
            i++;
            continue;
        }

        // Skip keys not used in the current outfile
        if (!g_key_file_has_key(config, group, options[i].name, NULL)) {
            i++;
            continue;
        }

       /*
         * TODO add a check for empty or blank keys. this is probably where you
         * need to check option.arg_type, but I think some arg_type values taken
         * from test.c may be incorrect.
         * example:
         * aencoder=
         */

        /*
         * Check for valid input. Failure here means we failed to validate
         * in keyfile.c:parse_validate_key_file()
         */
        /*
         * TODO probably unnecessary assert. used for verification during
         * implementation
         */
        //assert(options[i].valid_option(&options[i], config, group));

        gchar *string_value;
        gint integer_value;
        gdouble double_value;
        gchar **string_list_values;
        gint *integer_list_values;
        gdouble *double_list_values;
        gsize count;
        GString *arg;
        switch (options[i].key_type) {
            case k_string:
                // handle boolean values for keys without optional arguments
                if (options[i].arg_type == optional_argument) {
                    GError *error = NULL;
                    gboolean b = g_key_file_get_boolean(config, group,
                            options[i].name, &error);
                    if (error == NULL) {
                        if (b == TRUE) {
                            g_ptr_array_add(args, g_strdup_printf("--%s",
                                        options[i].name));
                        } else if (options[i].negation_option){
                            gchar * negation_name = g_strdup_printf("no-%s",
                                    options[i].name);
                            if (g_key_file_has_key(config, group, negation_name,
                                        NULL)) {
                                if (g_key_file_get_boolean(config, group,
                                            negation_name, NULL)) {
                                    g_ptr_array_add(args,
                                            g_strdup_printf("--%s",
                                                negation_name));
                                }
                            }
                            g_free(negation_name);
                        }
                        break;
                    } else {
                        g_error_free(error);
                    }
                }
                // regular string option
                string_value = g_key_file_get_string(config, group,
                    options[i].name, NULL);
                if (string_value != NULL) {
                    g_ptr_array_add(args, g_strdup_printf("--%s=%s",
                                options[i].name, string_value));
                }
                g_free(string_value);
                break;
            case k_boolean:
                // check for affirmative boolean (i.e. markers)
                if (g_key_file_get_boolean(config, group, options[i].name, NULL)) {
                    g_ptr_array_add(args, g_strdup_printf("--%s", options[i].name));
                }
                // check for negating boolean (i.e. no-markers)
                if (options[i].negation_option) {
                    gchar * negation_name = g_strdup_printf("no-%s", options[i].name);
                    if (g_key_file_has_key(config, group, negation_name, NULL)) {
                        if (g_key_file_get_boolean(config, group, negation_name, NULL)) {
                            g_ptr_array_add(args, g_strdup_printf("--%s", negation_name));
                        }
                    }
                    g_free(negation_name);
                }
                break;
            case k_integer:
                // special case for keys with arg_type optional_argument
                // if integer value is 0 or 1, take integer value
                // otherwise interpret as boolean and output bare option if true
                integer_value = g_key_file_get_integer(config, group,
                        options[i].name, NULL);
                if (options[i].arg_type == optional_argument &&
                        integer_value != 0 && integer_value != 1) {
                    GError *error = NULL;
                    gboolean b = g_key_file_get_boolean(config, group,
                            options[i].name, &error);
                    if (error == NULL) {
                        if (b == TRUE) {
                            g_ptr_array_add(args, g_strdup_printf("--%s",
                                        options[i].name));
                        } else if (options[i].negation_option){
                            gchar * negation_name = g_strdup_printf("no-%s",
                                    options[i].name);
                            if (g_key_file_has_key(config, group, negation_name,
                                        NULL)) {
                                if (g_key_file_get_boolean(config, group,
                                            negation_name, NULL)) {
                                    g_ptr_array_add(args,
                                            g_strdup_printf("--%s",
                                                negation_name));
                                }
                            }
                            g_free(negation_name);
                        }
                        break;
                    }
                }
                g_ptr_array_add(args, g_strdup_printf("--%s=%d",
                            options[i].name, integer_value));
                break;
            case k_double:
                double_value = g_key_file_get_double(config, group,
                        options[i].name, NULL);
                g_ptr_array_add(args, g_strdup_printf("--%s=%f",
                            options[i].name, double_value));
                break;
            case k_string_list:
                // handle boolean values for keys with optional arguments
                if (options[i].arg_type == optional_argument) {
                    GError *error = NULL;
                    gboolean b = g_key_file_get_boolean(config, group,
                            options[i].name, &error);
                    if (error == NULL) {
                        if (b == TRUE) {
                            g_ptr_array_add(args, g_strdup_printf("--%s",
                                        options[i].name));
                        } else if (options[i].negation_option){
                            gchar * negation_name = g_strdup_printf("no-%s",
                                    options[i].name);
                            if (g_key_file_has_key(config, group, negation_name,
                                        NULL)) {
                                if (g_key_file_get_boolean(config, group,
                                            negation_name, NULL)) {
                                    g_ptr_array_add(args,
                                            g_strdup_printf("--%s",
                                                negation_name));
                                }
                            }
                            g_free(negation_name);
                        }
                        break;
                    }
                }
                string_list_values = g_key_file_get_string_list(config, group,
                        options[i].name, &count, NULL);
                arg = g_string_new(NULL);
                g_string_append_printf(arg, "--%s=", options[i].name);
                int m = 0;
                for (; m < count-1; m++) {
                    g_string_append_printf(arg, "%s,",
                            g_strstrip(string_list_values[m]));
                }
                g_string_append_printf(arg, "%s",
                        g_strstrip(string_list_values[m]));
                g_strfreev(string_list_values);
                g_ptr_array_add(args, arg->str);
                g_string_free(arg, FALSE);
                break;
            case k_integer_list:
                integer_list_values = g_key_file_get_integer_list(config, group,
                        options[i].name, &count, NULL);
                arg = g_string_new(NULL);
                g_string_append_printf(arg, "--%s=", options[i].name);
                int n = 0;
                for (; n < count-1; n++) {
                    g_string_append_printf(arg, "%d,", integer_list_values[n]);
                }
                g_string_append_printf(arg, "%d", integer_list_values[n]);
                g_free(integer_list_values);
                g_ptr_array_add(args, arg->str);
                g_string_free(arg, FALSE);
                break;
            case k_double_list:
                double_list_values = g_key_file_get_double_list(config, group,
                        options[i].name, &count, NULL);
                arg = g_string_new(NULL);
                g_string_append_printf(arg, "--%s=", options[i].name);
                int o = 0;
                for (; o < count-1; o++) {
                    g_string_append_printf(arg, "%.1f,", double_list_values[o]);
                }
                g_string_append_printf(arg, "%.1f", double_list_values[o]);
                g_free(double_list_values);
                g_ptr_array_add(args, arg->str);
                g_string_free(arg, FALSE);
                break;
        }
        i++;
    }

    /*
     * Input and Output files are handled using keys specific to hbr
     */
    // input file arg (depends on input_basedir, iso_filename)
    g_ptr_array_add(args, g_strdup("-i"));
    gchar *temp = g_key_file_get_string(config, group, "input_basedir", NULL);
    GString *infile = g_string_new(temp);
    g_free(temp);
    if (infile->str[infile->len-1] != G_DIR_SEPARATOR){
        g_string_append(infile, G_DIR_SEPARATOR_S);
    }
    temp = g_key_file_get_string(config, group, "iso_filename", NULL);
    g_string_append_printf(infile, "%s", temp);
    g_free(temp);
    if (quoted) {
        g_ptr_array_add(args, g_shell_quote(infile->str));
        g_string_free(infile, TRUE); // g_shell_quote made a copy
    } else {
        g_ptr_array_add(args, infile->str);
        g_string_free(infile, FALSE); // keep string data, toss GString
    }

    /* output file arg (depends on type, name, year, season, episode,
     * specific_name, add_year, extra)
     */
    g_ptr_array_add(args, g_strdup("-o"));
    gchar *filename = build_filename(config, group);
    if (quoted) {
        g_ptr_array_add(args, g_shell_quote(filename));
        g_free(filename); // g_shell_quote made a new copy
    } else {
        g_ptr_array_add(args, filename);
        // keep string data, to be freed with ptr_array
    }
    // Null terminate the pointer array so we can use it without a count
    g_ptr_array_add(args, g_strdup(NULL));

    return args;
}


/**
 * @brief Generate a filename for an OUTFILE group
 *
 * @param config    keyfile that contains group
 * @param group     group name to generate a filename for
 *
 * @return filename
 */
gchar * build_filename(GKeyFile *config, gchar *group)
{
    gchar* output_basedir = g_key_file_get_string(config, group, "output_basedir", NULL);
    gchar* name = g_key_file_get_string(config, group, "name", NULL);
    gchar* type = g_key_file_get_string(config, group, "type", NULL);
    gchar* year = g_key_file_get_string(config, group, "year", NULL);
    gint season = g_key_file_get_integer(config, group, "season", NULL);
    gint episode = g_key_file_get_integer(config, group, "episode", NULL);
    gchar* specific_name = g_key_file_get_string(config, group, "specific_name", NULL);
    gchar* format = g_key_file_get_string(config, group, "format", NULL);
    gchar *extra_type = g_key_file_get_string(config, group, "extra", NULL);
    gboolean add_year = g_key_file_get_boolean(config, group, "add_year", NULL);

    GString* filename = g_string_new(NULL);

    if (output_basedir) {
        g_string_append(filename, output_basedir);
        if (filename->str[filename->len-1] != G_DIR_SEPARATOR){
            g_string_append(filename, G_DIR_SEPARATOR_S);
        }
    }
    g_free(output_basedir);

    if (strcmp(type, "movie") == 0) {
        if (year && add_year) {
            // year in the output directory
            append_year(config, group, filename);
            g_string_append(filename, G_DIR_SEPARATOR_S);
        }
        if (extra_type) {
            gchar *extra_name = NULL;
            if (strcmp(extra_type, "behindthescenes") == 0) {
                extra_name = g_strdup("Behind The Scenes");
            }
            if (strcmp(extra_type, "deleted") == 0) {
                extra_name = g_strdup("Deleted Scenes");
            }
            if (strcmp(extra_type, "featurette") == 0) {
                extra_name = g_strdup("Featurettes");
            }
            if (strcmp(extra_type, "interview") == 0) {
                extra_name = g_strdup("Interviews");
            }
            if (strcmp(extra_type, "scene") == 0) {
                extra_name = g_strdup("Scenes");
            }
            if (strcmp(extra_type, "short") == 0) {
                extra_name = g_strdup("Shorts");
            }
            if (strcmp(extra_type, "trailer") == 0) {
                extra_name = g_strdup("Trailers");
            }
            if (strcmp(extra_type, "other") == 0) {
                extra_name = g_strdup("Others");
            }
            g_free(extra_type);
            if (extra_name) {
                g_string_append(filename, extra_name);
                g_string_append(filename, G_DIR_SEPARATOR_S);
            }

            g_free(extra_name);
        } else {
            g_string_append(filename, name);
        }
        if (year && !extra_type) {
            g_string_append_printf(filename, " (%s)", year);
        }
    } else if (strcmp(type, "series") == 0) {
        g_string_append(filename, name);
        gboolean has_season = g_key_file_has_key(config, group, "season", NULL);
        gboolean has_episode = g_key_file_has_key(config, group, "episode", NULL);
        if (has_season || has_episode) {
            g_string_append(filename, " - ");
            if (has_season) {
                g_string_append_printf(filename, "s%02d", season);
            }
            if (has_episode) {
                g_string_append_printf(filename, "e%03d", episode);
            }
        }
    }
    if (specific_name && !extra_type) {
        g_string_append_printf(filename, " - %s", specific_name);
    } else if (specific_name && extra_type) {
        g_string_append_printf(filename, "%s", specific_name);
    }

    if (format != NULL && strcmp(format, "av_mkv") == 0 ) {
        g_string_append(filename, ".mkv");
    }
    else if (format != NULL && strcmp(format, "av_mp4") == 0 ) {
        g_string_append(filename, ".mp4");
    } else {
        // some default so files aren't left extension-less
        // TODO we should be verifying format is valid before we get here
        g_string_append(filename, ".mkv");
    }

    // TODO do some checks on this filename
    // check filename length doesn't exceed MAXPATHLEN (sys/param.h)
    // grab basename, check for leading -, leading/trailing spaces

    g_free(name);
    g_free(type);
    g_free(year);
    g_free(specific_name);
    g_free(format);
    gchar *strp = filename->str;
    g_string_free(filename, FALSE);
    return strp;
}


/**
 * @brief Adds a parenthesized year onto the final directory in a path
 *
 * @param config keyfile to pull year from
 * @param group group name
 * @param path path to be modified
 *
 * @return pointer to a new path string
 */
void append_year(GKeyFile *config, gchar *group, GString *path)
{
    gchar *dirname = g_path_get_dirname(path->str);
    gchar *year = g_key_file_get_string(config, group, "year", NULL);
    gchar *with_year = g_strjoin(NULL, dirname, " (", year, ")", NULL);
    
    // append a " (year)" onto the final directory
    gchar *path_with_year = g_build_path(dirname, with_year, NULL);
    g_string_assign(path, path_with_year);

    g_free(path_with_year);
    g_free(with_year);
    g_free(year);
    g_free(dirname);
}

/**
 * @brief Find the appropriate set of options to work with
 *
 * @param arg_version Version passed as a command line option to hbr
 */
void determine_handbrake_version(gchar *arg_version)
{
    // check HandBrakeCLI is available

    gchar *version = NULL;
    char *hb_version_argv[] = { "HandBrakeCLI", "--version", NULL };
    char *hb_update_argv[] = { "HandBrakeCLI", "--update", NULL };
    gchar *output = NULL;
    gint exit_status;
    // use version specified from -H or --hbversion
    if (arg_version) {
        version = arg_version;
    } else if (g_spawn_sync (NULL, hb_version_argv, NULL,
                G_SPAWN_SEARCH_PATH|G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &exit_status, NULL)
            && exit_status == 0) {
        gchar **split_output = g_strsplit_set(output, " \n", 3);
        g_free(output);
        version = g_strdup(split_output[1]);
        g_strfreev(split_output);
    } else {
        g_free(output);
        output = NULL;
        // try HandBrakeCLI --update and pick out version
        // note: update errors and spits out a version on *STDERR*
        if (g_spawn_sync (NULL, hb_update_argv, NULL,
                    G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL,
                    NULL, NULL, NULL, &output, &exit_status, NULL)) {
            gchar **split_output = g_strsplit_set(output, " \n", -1);
            g_free(output);
            int i = 0;
            // find "HandBrake" and take next string as version number
            while (split_output[i] != NULL) {
                if (strcmp(split_output[i], "HandBrake") == 0) {
                    version = g_strdup(split_output[i+1]);
                    break;
                }
                i++;
            }
            g_strfreev(split_output);
        }
    }

    if (version == NULL) {
        hbr_error("HandBrake output for version detection was not as expected. "
                "Exiting.", NULL, NULL, NULL, NULL);
        exit(1);
    }

    gint major, minor, patch;
    //split version number
    gchar **split_version = g_strsplit(version, ".", 3);
    major = atoi(split_version[0]);
    minor = atoi(split_version[1]);
    patch = atoi(split_version[2]);
    g_strfreev(split_version);

    /*
     * Baseline version if detection fails
     */
    version_options = option_v0_9_9;
    version_requires = require_v0_9_9;
    version_conflicts = conflict_v0_9_9;
    /*
     * NOTE: This logic is over-simplified because versions where changes
     * occurred are tightly packed. Think hard about this when adding new
     * versions.
     */
    if (major > 1 || (major == 1 && minor > 2) ||
            (major == 1 && minor == 2 && patch > 2)) {
        hbr_warn("Found newer HandBrake release (%s) than supported. "
                "Running with newest options available",
                NULL, NULL, NULL, NULL, version);
        version_options = option_v1_2_0;
        version_options_size = sizeof(option_v1_2_0);
        version_requires = require_v1_2_0;
        version_requires_size = sizeof(require_v1_2_0);
        version_conflicts = conflict_v1_2_0;
        version_conflicts_size = sizeof(conflict_v1_2_0);
    } else if (major == 1 && minor >= 2) {
        version_options = option_v1_2_0;
        version_options_size = sizeof(option_v1_2_0);
        version_requires = require_v1_2_0;
        version_requires_size = sizeof(require_v1_2_0);
        version_conflicts = conflict_v1_2_0;
        version_conflicts_size = sizeof(conflict_v1_2_0);
    } else if (major == 1 && minor > 0) {
        version_options = option_v1_1_0;
        version_options_size = sizeof(option_v1_1_0);
        version_requires = require_v1_1_0;
        version_requires_size = sizeof(require_v1_1_0);
        version_conflicts = conflict_v1_1_0;
        version_conflicts_size = sizeof(conflict_v1_1_0);
    } else if (major == 1 && minor >= 0) {
        version_options = option_v1_0_0;
        version_options_size = sizeof(option_v1_0_0);
        version_requires = require_v1_0_0;
        version_requires_size = sizeof(require_v1_0_0);
        version_conflicts = conflict_v1_0_0;
        version_conflicts_size = sizeof(conflict_v1_0_0);
    } else if (major == 0 && minor == 10 && patch >= 3) {
        version_options = option_v0_10_3;
        version_options_size = sizeof(option_v0_10_3);
        version_requires = require_v0_10_3;
        version_requires_size = sizeof(require_v0_10_3);
        version_conflicts = conflict_v0_10_3;
        version_conflicts_size = sizeof(conflict_v0_10_3);
    } else if (major == 0 && minor == 10) {
        version_options = option_v0_10_0;
        version_options_size = sizeof(option_v0_10_0);
        version_requires = require_v0_10_0;
        version_requires_size = sizeof(require_v0_10_0);
        version_conflicts = conflict_v0_10_0;
        version_conflicts_size = sizeof(conflict_v0_10_0);
    } else if (major == 0 && minor == 9 && patch >= 9) {
        version_options = option_v0_9_9;
        version_options_size = sizeof(option_v0_9_9);
        version_requires = require_v0_9_9;
        version_requires_size = sizeof(require_v0_9_9);
        version_conflicts = conflict_v0_9_9;
        version_conflicts_size = sizeof(conflict_v0_9_9);
    } else {
        hbr_warn("Could not match a supported HandBrake version. "
                "Trying oldest options available (0.9.9)",
                NULL, NULL, NULL, NULL);
    }
    g_free(version);

    // Merge handbrake and hbr specific tables
    options = g_malloc(version_options_size + sizeof(hbr_options));
    memcpy(options, version_options, version_options_size);
    gsize version_options_count =  version_options_size/sizeof(option_t);
    // overwrite final null option
    option_t *last_option = &(options[version_options_count-1]);
    memcpy(last_option, hbr_options, sizeof(hbr_options));

    requires = g_malloc(version_requires_size + sizeof(hbr_requires));
    memcpy(requires, version_requires, version_requires_size);
    gsize version_requires_count =  version_requires_size/sizeof(require_t);
    require_t *last_require = &(requires[version_requires_count-1]);
    memcpy(last_require, hbr_requires, sizeof(hbr_requires));

    conflicts = g_malloc(version_conflicts_size + sizeof(hbr_conflicts));
    memcpy(conflicts, version_conflicts, version_conflicts_size);
    gsize version_conflicts_count =  version_conflicts_size/sizeof(conflict_t);
    conflict_t *last_conflict = &(conflicts[version_conflicts_count-1]);
    memcpy(last_conflict, hbr_conflicts, sizeof(hbr_conflicts));
}

/**
 * @brief Generate hash tables to for look up of options, requires,
 *        and conflicts.
 */
void arg_hash_generate(void)
{
    // options
    options_index = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; options[i].name != NULL; i++) {
        g_hash_table_insert(options_index, options[i].name, GINT_TO_POINTER(i));
    }
    /*
     * We cannot specify the value_destroy_func as g_slist_free() for our hash
     * table because g_hash_table_insert() calls it every time a value is
     * replaced. Instead we have to manually free each GSList before destroying
     * the table.
     */
    // requires
    requires_index = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; requires[i].name != NULL; i++) {
        g_hash_table_insert(requires_index, requires[i].name,
                g_slist_prepend(g_hash_table_lookup(requires_index,
                        requires[i].name), GINT_TO_POINTER(i)));
    }

    // conflicts
    conflicts_index = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; conflicts[i].name != NULL; i++) {
        g_hash_table_insert(conflicts_index, conflicts[i].name,
                g_slist_prepend(g_hash_table_lookup(conflicts_index,
                        conflicts[i].name), GINT_TO_POINTER(i)));
    }
    return;
}

/**
 * @brief Free hash tables and lists created by arg_hash_generate()
 */
void arg_hash_cleanup(void)
{
    g_hash_table_destroy(options_index);
    g_hash_table_foreach(requires_index, free_slist_in_hash, NULL);
    g_hash_table_destroy(requires_index);
    g_hash_table_foreach(conflicts_index, free_slist_in_hash, NULL);
    g_hash_table_destroy(conflicts_index);
    g_free(options);
    g_free(requires);
    g_free(conflicts);
}

/**
 * @brief GSList freeing function to be passed to g_hash_table_foreach()
 *        This frees the lists allocated for requires_index and conflicts_index.
 *        Do not call directly.
 */
void free_slist_in_hash(gpointer key, gpointer slist, gpointer user_data)
{
    g_slist_free(slist);
}
