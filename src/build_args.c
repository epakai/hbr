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

#include <stdio.h>    // for NULL
#include <stdlib.h>   // for exit
#include <string.h>   // for strcmp, memcpy
#include <assert.h>   // for assert

#include "util.h"
#include "build_args.h"
#include "options.h"
#include "validate.h"

extern option_data_t option_data;

/**
 * @brief Produce options to be passed to HandBrakeCLI
 *
 * @param config Keyfile to build options from
 * @param group Name of group in keyfile that contains key and values
 * @param quoted Determines if filenames should be quoted (for debug mode)
 *
 * @return GPtrArray pointer with one option per element
 */
GPtrArray * build_args(GKeyFile *config, const gchar *group, gboolean quoted)
{
    option_t *options = option_data.options;
    GPtrArray *args = g_ptr_array_new_full(32, g_free);

    gint i = 0;
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

        switch (options[i].key_type) {
            case k_string:
                build_arg_string(config, group, args, i, FALSE);
                break;
            case k_boolean:
                build_arg_boolean(config, group, args, i);
                break;
            case k_integer:
                build_arg_integer(config, group, args, i);
                break;
            case k_double:
                build_arg_double(config, group, args, i);
                break;
            case k_string_list:
                build_arg_string_list(config, group, args, i, FALSE);
                break;
            case k_integer_list:
                build_arg_integer_list(config, group, args, i);
                break;
            case k_double_list:
                build_arg_double_list(config, group, args, i);
                break;
            /*
             * Paths are handled differently than other strings because they
             * need to be quoted.
             */
            case k_path:
                build_arg_string(config, group, args, i, TRUE);
                break;
            case k_path_list:
                build_arg_string_list(config, group, args, i, TRUE);
                break;
            default:
                hbr_error("Invalid key type. This should not be reached " \
                        "(key_types is a fixed enum).", NULL, NULL, NULL, NULL);
                assert(FALSE);
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
 * @brief Builds arguments where the key type is a string
 *
 * @param config       KeyFile to pull values from
 * @param group        group to pull values from
 * @param args         argument array to append argument to
 * @param i            index of the option being built
 * @param param_quoted when true strings are single quoted
 */
void build_arg_string(GKeyFile *config, const gchar *group,
        GPtrArray *args, gint i, gboolean param_quoted) {
    option_t *options = option_data.options;
    gchar *string_value;
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
            return;
        } else {
            g_error_free(error);
        }
    }
    // regular string option
    string_value = g_key_file_get_string(config, group,
            options[i].name, NULL);
    if (string_value != NULL) {
        if (param_quoted) {
            g_ptr_array_add(args, g_strdup_printf("--%s=\"%s\"",
                        options[i].name, string_value));
        } else {
            g_ptr_array_add(args, g_strdup_printf("--%s=%s",
                        options[i].name, string_value));
        }
    }
    g_free(string_value);
    return;
}

/**
 * @brief Builds arguments where the key type is a boolean
 *
 * @param config KeyFile to pull values from
 * @param group  group to pull values from
 * @param args   argument array to append argument to
 * @param i      index of the option being built
 */
void build_arg_boolean(GKeyFile *config, const gchar *group,
        GPtrArray *args, gint i) {
    option_t *options = option_data.options;
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
}

/**
 * @brief Builds arguments where the key type is a integer
 *
 * @param config KeyFile to pull values from
 * @param group  group to pull values from
 * @param args   argument array to append argument to
 * @param i      index of the option being built
 */
void build_arg_integer(GKeyFile *config, const gchar *group,
        GPtrArray *args, gint i) {
    option_t *options = option_data.options;
    // special case for keys with arg_type optional_argument
    // if integer value is 0 or 1, take integer value
    // otherwise interpret as boolean and output bare option if true
    gint integer_value = g_key_file_get_integer(config, group,
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
            return;
        }
    }
    g_ptr_array_add(args, g_strdup_printf("--%s=%d",
                options[i].name, integer_value));
}

/**
 * @brief Builds arguments where the key type is a double
 *
 * @param config KeyFile to pull values from
 * @param group  group to pull values from
 * @param args   argument array to append argument to
 * @param i      index of the option being built
 */
void build_arg_double(GKeyFile *config, const gchar *group, GPtrArray *args,
        gint i) {
    option_t *options = option_data.options;
    gdouble double_value = g_key_file_get_double(config, group,
            options[i].name, NULL);
    g_ptr_array_add(args, g_strdup_printf("--%s=%f",
                options[i].name, double_value));
}

/**
 * @brief Builds arguments where the key type is a list of strings
 *
 * @param config       KeyFile to pull values from
 * @param group        group to pull values from
 * @param args         argument array to append argument to
 * @param i            index of the option being built
 * @param param_quoted when true strings are single quoted
 */
void build_arg_string_list(GKeyFile *config, const gchar *group, GPtrArray *args,
        gint i, gboolean param_quoted) {
    GString *arg;
    gsize count;
    option_t *options = option_data.options;
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
            return;
        }
    }
    gchar **string_list_values = g_key_file_get_string_list(config, group,
            options[i].name, &count, NULL);
    arg = g_string_new(NULL);
    g_string_append_printf(arg, "--%s=", options[i].name);
    for (gsize m = 0; m < count; m++) {
        if (param_quoted) {
            g_string_append_printf(arg, "\"%s\"",
                    g_strstrip(string_list_values[m]));
        } else {
            g_string_append_printf(arg, "%s",
                    g_strstrip(string_list_values[m]));
        }
        if (m < count-1) {
            g_string_append(arg, ",");
        }
    }
    g_strfreev(string_list_values);
    g_ptr_array_add(args, arg->str);
    g_string_free(arg, FALSE);
}

/**
 * @brief Builds arguments where the key type is a list of integers
 *
 * @param config KeyFile to pull values from
 * @param group  group to pull values from
 * @param args   argument array to append argument to
 * @param i      index of the option being built
 */
void build_arg_integer_list(GKeyFile *config, const gchar *group,
        GPtrArray *args, gint i) {
    GString *arg;
    gsize count;
    option_t *options = option_data.options;
    gint *integer_list_values = g_key_file_get_integer_list(config, group,
            options[i].name, &count, NULL);
    arg = g_string_new(NULL);
    g_string_append_printf(arg, "--%s=", options[i].name);
    for (gsize n = 0; n < count; n++) {
        g_string_append_printf(arg, "%d", integer_list_values[n]);
        if (n < count-1) {
            g_string_append(arg, ",");
        }
    }
    g_free(integer_list_values);
    g_ptr_array_add(args, arg->str);
    g_string_free(arg, FALSE);
}

/**
 * @brief Builds arguments where the key type is a list of doubles
 *
 * @param config KeyFile to pull values from
 * @param group  group to pull values from
 * @param args   argument array to append argument to
 * @param i      index of the option being built
 */
void build_arg_double_list(GKeyFile *config, const gchar *group,
        GPtrArray *args, gint i) {
    GString *arg;
    gsize count;
    option_t *options = option_data.options;
    gdouble *double_list_values = g_key_file_get_double_list(config, group,
            options[i].name, &count, NULL);
    arg = g_string_new(NULL);
    g_string_append_printf(arg, "--%s=", options[i].name);
    for (gsize o = 0; o < count; o++) {
        g_string_append_printf(arg, "%.1f", double_list_values[o]);
        if (o < count-1) {
            g_string_append(arg, ",");
        }
    }
    g_free(double_list_values);
    g_ptr_array_add(args, arg->str);
    g_string_free(arg, FALSE);
}

/**
 * @brief Generate a filename for an OUTFILE group
 *
 * @param config    keyfile that contains group
 * @param group     group name to generate a filename for
 *
 * @return filename
 */
gchar * build_filename(GKeyFile *config, const gchar *group)
{
    gchar* output_basedir = g_key_file_get_string(config, group, "output_basedir", NULL);
    gchar* name = g_key_file_get_string(config, group, "name", NULL);
    gchar* type = g_key_file_get_string(config, group, "type", NULL);
    gchar* year = g_key_file_get_string(config, group, "year", NULL);
    gint season = g_key_file_get_integer(config, group, "season", NULL);
    gint episode = g_key_file_get_integer(config, group, "episode", NULL);
    gchar* specific_name = g_key_file_get_string(config, group, "specific_name", NULL);
    gchar* format = g_key_file_get_string(config, group, "format", NULL);
    gchar* extra_type = g_key_file_get_string(config, group, "extra", NULL);
    gboolean add_year = g_key_file_get_boolean(config, group, "add_year", NULL);

    GString* filename = g_string_new(NULL);

    if (output_basedir) {
        g_string_append(filename, output_basedir);
        if (filename->str[filename->len-1] != G_DIR_SEPARATOR){
            g_string_append(filename, G_DIR_SEPARATOR_S);
        }
    }
    g_free(output_basedir);

    if (year && add_year) {
        // year in the output directory
        append_year(config, group, filename);
        g_string_append(filename, G_DIR_SEPARATOR_S);
    }

    if (strcmp(type, "movie") == 0) {
        // extras subdirectories
        if (extra_type) {
            struct extra {
                const char *extra_type;
                const char *extra_name;
            } extras[] = {
                {"behindthescenes", "Behind The Scenes"},
                {"deleted", "Deleted Scenes"},
                {"featurette", "Featurettes"},
                {"interview", "Interviews"},
                {"scene", "Scenes"},
                {"short", "Shorts"},
                {"trailer", "Trailers"},
                {"other", "Others"},
                {NULL, NULL}
            };
            int i = 0;
            while (extras[i].extra_type != NULL) {
                if (strcmp(extra_type, extras[i].extra_type) == 0) {
                    g_string_append(filename, extras[i].extra_name);
                    g_string_append(filename, G_DIR_SEPARATOR_S);
                }
                i++;
            }
            g_free(extra_type);
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
        if (has_season) {
            g_string_append_printf(filename, " - s%02d", season);
        }
        if (has_episode) {
            if (!has_season) {
                g_string_append(filename, " - ");
            }
            g_string_append_printf(filename, "e%03d", episode);
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
    return g_string_free(filename, FALSE);
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
void append_year(GKeyFile *config, const gchar *group, GString *path)
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
