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
#include "build_args.h"
#include "handbrake/options-0.9.9.h"
#include "handbrake/options-0.10.0.h"
#include "handbrake/options-0.10.3.h"
#include "handbrake/options-1.0.0.h"
#include "handbrake/options-1.1.0.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * @brief 
 *
 * @param outfile
 * @param config
 * @param quoted
 *
 * @return 
 */
GPtrArray * build_args(GKeyFile *config, gchar *group, gboolean quoted)
{
    GPtrArray *args = g_ptr_array_new_full(32, g_free);

    int i = 0;
    while (options[i].name != NULL) {
        // Skip keys not in file
        if (!g_key_file_has_key(config, group, options[i].name, NULL)){
            i++;
            continue;
        }
        /*
         * TODO add a check for empty or blank keys. this is probably where you
         * need to check arg_type, but I think some arg_type values taken from
         * test.c may be incorrect.
         */

        /*
         * Check for valid input. Failure here means we failed to validate
         * in keyfile.c:validate_key_file()
         */
        assert(options[i].valid_input(&options[i], config, group));
        
        gchar *string_value;
        gint integer_value;
        gdouble double_value;
        gchar **string_list_values;
        gboolean *boolean_list_values;
        gint *integer_list_values;
        gdouble *double_list_values;
        gsize count;
        GString *arg;
        switch (options[i].key_type) {
            case k_string:
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
                if(g_key_file_get_boolean(config, group, options[i].name, NULL)) {
                    g_ptr_array_add(args, g_strdup_printf("--%s", options[i].name));
                }
                // check for negating boolean (i.e. no-markers)
                if (options[i].negation_option) {
                    gchar * negation_name = g_strdup_printf("no-%s", options[i].name);
                    if (g_key_file_has_key(config, group, negation_name, NULL)) {
                        if(g_key_file_get_boolean(config, group, negation_name, NULL)) {
                            g_ptr_array_add(args, g_strdup_printf("--%s", negation_name));
                        }   
                    }
                }
                break;
            case k_integer:
                integer_value = g_key_file_get_integer(config, group,
                        options[i].name, NULL);
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
                string_list_values = g_key_file_get_string_list(config, group,
                        options[i].name, &count, NULL);
                arg = g_string_new(NULL);
                g_string_append_printf(arg, "--%s=", options[i].name);
                int j = 0;
                for (; j < count-1; j++) {
                    g_string_append_printf(arg, "%s,", string_list_values[j]);
                }
                g_string_append_printf(arg, "%s", string_list_values[j]);
                g_strfreev(string_list_values);
                g_ptr_array_add(args, arg->str);
                g_string_free(arg, FALSE);
                break;
            case k_integer_list:
                integer_list_values = g_key_file_get_integer_list(config, group,
                        options[i].name, &count, NULL);
                arg = g_string_new(NULL);
                g_string_append_printf(arg, "--%s=", options[i].name);
                int k = 0;
                for (; k < count-1; k++) {
                    g_string_append_printf(arg, "%d,", integer_list_values[k]);
                }
                g_string_append_printf(arg, "%d", integer_list_values[k]);
                g_free(integer_list_values);
                g_ptr_array_add(args, arg->str);
                g_string_free(arg, FALSE);
                break;
            case k_double_list:
                double_list_values = g_key_file_get_double_list(config, group,
                        options[i].name, &count, NULL);
                arg = g_string_new(NULL);
                g_string_append_printf(arg, "--%s=", options[i].name);
                int l = 0;
                for (; l < count-1; l++) {
                    g_string_append_printf(arg, "%.1f,", double_list_values[l]);
                }
                g_string_append_printf(arg, "%.1f", double_list_values[l]);
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

    // output file arg (depends on type, name, year, season, episode_number, specific_name)
    g_ptr_array_add(args, g_strdup("-o"));
    GString *filename = build_filename(config, group, TRUE);
    if (quoted) {
        g_ptr_array_add(args, g_shell_quote(filename->str));
        g_string_free(filename, TRUE); // g_shell_quote made a new copy
    } else {
        g_ptr_array_add(args, filename->str);
        g_string_free(filename, FALSE); // keep string data, toss GString
    }
    // Couldn't find documents saying GPtrArray is null terminated so we'll do that
    g_ptr_array_add(args, g_strdup(NULL));
   
    return args;
}

GString * build_filename(GKeyFile *config, gchar *group, gboolean full_path)
{
    GString* filename = g_string_new(NULL);

    gchar* output_basedir = g_key_file_get_string(config, group, "output_basedir", NULL);
    if (full_path && output_basedir) {
        g_string_append(filename, output_basedir);
        if (filename->str[filename->len-1] != G_DIR_SEPARATOR){
            g_string_append(filename, G_DIR_SEPARATOR_S);
        }
    }
    g_free(output_basedir);

    gchar* name = g_key_file_get_string(config, group, "name", NULL);
    gchar* type = g_key_file_get_string(config, group, "type", NULL);
    gchar* year = g_key_file_get_string(config, group, "year", NULL);
    gint season = g_key_file_get_integer(config, group, "season", NULL);
    gint episode_number = g_key_file_get_integer(config, group, "episode_number", NULL);
    gchar* specific_name = g_key_file_get_string(config, group, "specific_name", NULL);
    gchar* format = g_key_file_get_string(config, group, "format", NULL);

    g_string_append_printf(filename, "%s", name);
    if (strcmp(type, "movie") == 0) {
        if (year) {
            g_string_append_printf(filename, " (%s)", year);
        }
    } else if ( strcmp(type, "series") == 0) {
        if (season || episode_number) {
            g_string_append(filename, " - ");
            if (season) {
                g_string_append_printf(filename, "s%02d", season);
            }
            if (episode_number) {
                g_string_append_printf(filename, "e%03d", episode_number);
            }
        }
    }
    if (specific_name) {
        g_string_append_printf(filename, " - %s", specific_name);
    }
    
    if (strcmp(format, "av_mkv") == 0 ) {
        g_string_append(filename, ".mkv");
    }
    else if (strcmp(format, "av_mp4") == 0 ) {
        g_string_append(filename, ".mp4");
    } else {
        // some default so files aren't left extension-less
        // we should be verifying format is valid before we get here
        g_string_append(filename, ".mkv");
    }

    g_free(name);
    g_free(type);
    g_free(year);
    g_free(specific_name);
    g_free(format);
    return filename;
}


/**
 * @brief Determine if s matches any strings in list
 *
 * @param s string to be tested
 * @param list string array to test against
 * @param len number of strings in list
 *
 * @return TRUE when s matches a string in list
 */
static gboolean strcmp_list(gchar *s, gchar **list, gsize len) {
    for (gsize i = 0; i< len; i++) {
        if (strcmp(s, list[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief Find the appropriate set of options to work with
 *
 * @return pointer to matching option array
 */
void determine_handbrake_version(gchar *arg_version)
{
    // check HandBrakeCLI is available

    gchar *version;
    // try version specified from -H/--hbversion
    if (arg_version) {
        version = arg_version;
    } else {
        // else try HandBrakeCLI --version
        // TODO
        // else try HandBrakeCLI --update and pick out version
        // TODO
    }

    gint major, minor, patch;
    //split version number
    //TODO


    /*
     * Baseline version if detection fails
     */
    options = option_v0_9_9;
    depends = depend_v0_9_9;
    conflicts = conflict_v0_9_9;
    /*
     * NOTE: This logic is over-simplified because versions where changes
     * occured are tightly packed. Think hard about this when adding new
     * versions.
     */
    if (major > 1 || (major == 1 && minor > 1) ||
            (major == 1 && minor == 1 && patch > 1)) {
        fprintf(stderr, "Found newer HandBrake release (%s) than supported.\n"
                "Running with newest options available.\n", version);
        options = option_v1_1_0;
        depends = depend_v1_1_0;
        conflicts = conflict_v1_1_0;
    } else if (major == 1 && minor > 0) {
        options = option_v1_1_0;
        depends = depend_v1_1_0;
        conflicts = conflict_v1_1_0;
    } else if (major == 1 && minor >= 0) {
        options = option_v1_0_0;
        depends = depend_v1_0_0;
        conflicts = conflict_v1_0_0;
    } else if (major == 0 && minor == 10 && patch >= 3) {
        options = option_v0_10_3;
        depends = depend_v0_10_3;
        conflicts = conflict_v0_10_3;
    } else if (major == 0 && minor == 10) {
        options = option_v0_10_0;
        depends = depend_v0_10_0;
        conflicts = conflict_v0_10_0;
    } else if (major == 0 && minor == 9 && patch >= 9) {
        options = option_v0_9_9;
        depends = depend_v0_9_9;
        conflicts = conflict_v0_9_9;
    } else {
        fprintf(stderr, "Could not match a supported HandBrake version.\n"
                "Trying oldest release available (0.9.9).\n");
    }

    // generate hash tables
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
    depends_index = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; depends[i].name != NULL; i++) {
        g_hash_table_insert(depends_index, depends[i].name,
                g_slist_prepend(g_hash_table_lookup(depends_index,
                        depends[i].name), GINT_TO_POINTER(i)));
    }
    conflicts_index = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; conflicts[i].name != NULL; i++) {
        g_hash_table_insert(conflicts_index, conflicts[i].name,
                g_slist_prepend(g_hash_table_lookup(conflicts_index,
                        conflicts[i].name), GINT_TO_POINTER(i)));
    }
    return;
}

/**
 * @brief GSList freeing function to be passed to g_hash_table_foreach()
 *        This frees the lists allocated for depends_index and conflicts_index.
 *        Do not call directly.
 */
void free_slist_in_hash(gpointer key, gpointer slist, gpointer user_data)
{
    g_slist_free(slist);
}

//TODO add asserts that valid_values and valid_values_count are NULL/0
//TODO add asserts throughout all vail_ functions because there's lots
// of ways to muck those files up
//TODO all-subtiles/first-subtitle all-audio/first-audio conflict
// but we don't have any way to handle this with valid_boolean
// may need to give them their own valid_function
gboolean valid_boolean(option_t *option, GKeyFile *config, gchar *group)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    return TRUE;
}


gboolean valid_integer_set(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_integer_list(option_t *option, GKeyFile *config, gchar *group)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    return TRUE;
}

gboolean valid_integer_list_set(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_positive_integer(option_t *option, GKeyFile *config, gchar *group)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    return TRUE;
}


gboolean valid_positive_double_list(option_t *option, GKeyFile *config, gchar *group)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    return TRUE;
}

gboolean valid_string(option_t *option, GKeyFile *config, gchar *group)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    return TRUE;
}

gboolean valid_string_set(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_string_list_set(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_string_list(option_t *option, GKeyFile *config, gchar *group)
{
    assert(option->valid_values_count == 0 && option->valid_values == NULL);
    return TRUE;
}


gboolean valid_optimize(option_t *option, GKeyFile *config, gchar *group)
{
    // TODO this and other valid functions may be unnecessary
    // Once conflict and depend checking is implemented we can treat
    // this like any other boolean option
    return TRUE;
}

gboolean valid_filename_exists(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_filename_exists_list(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_filename_dne(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_startstop_at(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_previews(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_audio(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_audio_quality(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_audio_bitrate(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_video_quality(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_video_bitrate(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_crop(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_pixel_aspect(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_decomb(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_denoise(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_deblock(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_deinterlace(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_detelecine(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_iso639(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_iso639_list(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}


gboolean valid_native_dub(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_subtitle(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_gain(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_drc(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_mixdown(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_chapters(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_encopts(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_encoder_preset(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_encoder_tune(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_encoder_profile(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_encoder_level(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_nlmeans(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_nlmeans_tune(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_dither(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_subtitle_forced(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_codeset(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_rotate(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_qsv_decoding(option_t *option, GKeyFile *config, gchar *group)
{
    /*
     * this function validates enable and disable
     * it should verify conflicting options aren't on the same level
     */
    return TRUE;
}

gboolean valid_comb_detect(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_pad(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_unsharp(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_filespec(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}

gboolean valid_preset_name(option_t *option, GKeyFile *config, gchar *group)
{
    return TRUE;
}
