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
#include "handbrake/options-0.9.9.h"
#include "handbrake/options-0.10.0.h"
#include "handbrake/options-0.10.3.h"
#include "handbrake/options-1.0.0.h"
#include "handbrake/options-1.1.0.h"
#include "handbrake/options-1.2.0.h"
#include "handbrake/options-1.3.0.h"

#include "util.h"
#include "options.h"
#include <stdlib.h>
#include <string.h>

extern option_data_t option_data;

/*
 * hbr specific options tables
 * These aren't valid options to pass to HandBrakeCLI.
 * They are used by hbr to generate file names, specify file locations,
 * or control hbr features.
 */

/**
 * @brief hbr specific keys
 *        { name, arg_type, key_type, negation, valid_function,
 *          valid_values_count, valid_values }
 */
static option_t hbr_options[] =
{
    { "type", hbr_only, k_string, FALSE, valid_type, 2,
        (const gchar*[]){"series", "movie"}},
    // TODO document add_year (it should add a (year) to the directory name if type is movie)
    { "add_year", hbr_only, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "input_basedir", hbr_only, k_string, FALSE, valid_readable_path, 0, NULL},
    { "output_basedir", hbr_only, k_string, FALSE, valid_writable_path, 0, NULL},
    { "iso_filename", hbr_only, k_string, FALSE, valid_filename_component, 0, NULL},
    { "name", hbr_only, k_string, FALSE, valid_filename_component, 0, NULL},
    { "year", hbr_only, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "season", hbr_only, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "episode", hbr_only, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "specific_name", hbr_only, k_string, FALSE, valid_filename_component, 0, NULL},
    { "preview", hbr_only, k_boolean, FALSE, valid_boolean, 0, NULL },
    // TODO document extra (it causes subdirectories to be created for extras)
    { "extra", hbr_only, k_string, FALSE, valid_string_set, 8,
        (const gchar*[]){"behindthescenes", "deleted", "featurette",
            "interview", "scene", "short", "trailer", "other"}},
    { "debug", hbr_only, k_boolean, FALSE, valid_boolean, 0, NULL},
    { NULL, 0, 0, 0, NULL, 0, NULL}
};

/**
 * @brief hbr keys that require other keys
 *        { key, required key, specific require value }
 */
static require_t hbr_requires[] =
{
    { "extra", "type", "movie"},
    { "year", "type", "movie"},
    { "season", "type", "series"},
    { "episode", "type", "series"},
    { NULL, NULL, NULL}
};

/**
 * @brief hbr keys that conflict
 *        { key, key value, conflicting key, conflicting value }
 */
static conflict_t hbr_conflicts[] =
{
    { "add_year", "true", "type", "series" },
    { "type", "series", "add_year", "true" },
    { NULL, NULL, NULL, NULL}
};

/**
 * @brief Find the appropriate set of options to work with
 *
 * @param arg_version Version passed as a command line option to hbr
 */
void determine_handbrake_version(gchar *arg_version)
{
    // check HandBrakeCLI is available

    gchar *version = NULL;
    char *hb_version_argv[] = { (char *)"HandBrakeCLI", (char *)"--version",
        NULL };
    char *hb_update_argv[] = { (char *)"HandBrakeCLI", (char *)"--update",
        NULL };
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
     * temporary pointers before these are combined with hbr specific data
     */
    option_t *version_options = option_v0_9_9;
    custom_t *version_customs = custom_v0_9_9;
    require_t *version_requires = require_v0_9_9;
    conflict_t *version_conflicts = conflict_v0_9_9;
    size_t version_options_size = 0;
    //size_t version_customs_size; // not used since hbr options have no customs
    size_t version_requires_size = 0;
    size_t version_conflicts_size = 0;

    /*
     * NOTE: This logic is over-simplified because versions where changes
     * occurred are tightly packed. Think hard about this when adding new
     * versions.
     */
    if (major > 1 || (major == 1 && minor > 3) ||
            (major == 1 && minor == 3 && patch > 2)) {
        hbr_warn("Found newer HandBrake release (%s) than supported. "
                "Running with newest options available",
                NULL, NULL, NULL, NULL, version);
        version_options = option_v1_3_0;
        version_options_size = sizeof(option_v1_3_0);
        version_customs = custom_v1_3_0;
        //version_customs_size = sizeof(custom_v1_3_0);
        version_requires = require_v1_3_0;
        version_requires_size = sizeof(require_v1_3_0);
        version_conflicts = conflict_v1_3_0;
        version_conflicts_size = sizeof(conflict_v1_3_0);
    } else if (major == 1 && minor >= 3) {
        version_options = option_v1_3_0;
        version_options_size = sizeof(option_v1_3_0);
        version_customs = custom_v1_3_0;
        //version_customs_size = sizeof(custom_v1_3_0);
        version_requires = require_v1_3_0;
        version_requires_size = sizeof(require_v1_3_0);
        version_conflicts = conflict_v1_3_0;
        version_conflicts_size = sizeof(conflict_v1_3_0);
    } else if (major == 1 && minor >= 2) {
        version_options = option_v1_2_0;
        version_options_size = sizeof(option_v1_2_0);
        version_customs = custom_v1_2_0;
        //version_customs_size = sizeof(custom_v1_2_0);
        version_requires = require_v1_2_0;
        version_requires_size = sizeof(require_v1_2_0);
        version_conflicts = conflict_v1_2_0;
        version_conflicts_size = sizeof(conflict_v1_2_0);
    } else if (major == 1 && minor > 0) {
        version_options = option_v1_1_0;
        version_options_size = sizeof(option_v1_1_0);
        version_customs = custom_v1_1_0;
        //version_customs_size = sizeof(custom_v1_1_0);
        version_requires = require_v1_1_0;
        version_requires_size = sizeof(require_v1_1_0);
        version_conflicts = conflict_v1_1_0;
        version_conflicts_size = sizeof(conflict_v1_1_0);
    } else if (major == 1 && minor >= 0) {
        version_options = option_v1_0_0;
        version_options_size = sizeof(option_v1_0_0);
        version_customs = custom_v1_0_0;
        //version_customs_size = sizeof(custom_v1_0_0);
        version_requires = require_v1_0_0;
        version_requires_size = sizeof(require_v1_0_0);
        version_conflicts = conflict_v1_0_0;
        version_conflicts_size = sizeof(conflict_v1_0_0);
    } else if (major == 0 && minor == 10 && patch >= 3) {
        version_options = option_v0_10_3;
        version_options_size = sizeof(option_v0_10_3);
        version_customs = custom_v0_10_3;
        //version_customs_size = sizeof(custom_v0_10_3);
        version_requires = require_v0_10_3;
        version_requires_size = sizeof(require_v0_10_3);
        version_conflicts = conflict_v0_10_3;
        version_conflicts_size = sizeof(conflict_v0_10_3);
    } else if (major == 0 && minor == 10) {
        version_options = option_v0_10_0;
        version_options_size = sizeof(option_v0_10_0);
        version_customs = custom_v0_10_0;
        //version_customs_size = sizeof(custom_v0_10_0);
        version_requires = require_v0_10_0;
        version_requires_size = sizeof(require_v0_10_0);
        version_conflicts = conflict_v0_10_0;
        version_conflicts_size = sizeof(conflict_v0_10_0);
    } else if (major == 0 && minor == 9 && patch >= 9) {
        version_options = option_v0_9_9;
        version_options_size = sizeof(option_v0_9_9);
        version_customs = custom_v0_9_9;
        //version_customs_size = sizeof(custom_v0_9_9);
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
    option_data.options = g_malloc(version_options_size + sizeof(hbr_options));
    memcpy(option_data.options, version_options, version_options_size);
    gsize version_options_count =  version_options_size/sizeof(option_t);
    // overwrite final null option
    option_t *last_option = &(option_data.options[version_options_count-1]);
    memcpy(last_option, hbr_options, sizeof(hbr_options));

    option_data.customs = version_customs;

    option_data.requires = g_malloc(version_requires_size + sizeof(hbr_requires));
    memcpy(option_data.requires, version_requires, version_requires_size);
    gsize version_requires_count =  version_requires_size/sizeof(require_t);
    require_t *last_require = &(option_data.requires[version_requires_count-1]);
    memcpy(last_require, hbr_requires, sizeof(hbr_requires));

    option_data.conflicts = g_malloc(version_conflicts_size + sizeof(hbr_conflicts));
    memcpy(option_data.conflicts, version_conflicts, version_conflicts_size);
    gsize version_conflicts_count =  version_conflicts_size/sizeof(conflict_t);
    conflict_t *last_conflict = &(option_data.conflicts[version_conflicts_count-1]);
    memcpy(last_conflict, hbr_conflicts, sizeof(hbr_conflicts));
}

/**
 * @brief Generate hash tables to for look up of options, requires,
 *        and conflicts.
 */
void arg_hash_generate(void)
{
    // options
    option_data.options_index = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; option_data.options[i].name != NULL; i++) {
        g_hash_table_insert(option_data.options_index,
                (void *)option_data.options[i].name,
                GINT_TO_POINTER(i));
    }
    option_data.customs_index = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; option_data.customs[i].name != NULL; i++) {
        g_hash_table_insert(option_data.customs_index,
                (void *)option_data.customs[i].name,
                GINT_TO_POINTER(i));
    }
    /*
     * We cannot specify the value_destroy_func as g_slist_free() for our hash
     * table because g_hash_table_insert() calls it every time a value is
     * replaced. Instead we have to manually free each GSList before destroying
     * the table.
     */
    // requires
    option_data.requires_index = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; option_data.requires[i].name != NULL; i++) {
        g_hash_table_insert(option_data.requires_index,
                (void *)option_data.requires[i].name,
                g_slist_prepend(g_hash_table_lookup(option_data.requires_index,
                        option_data.requires[i].name), GINT_TO_POINTER(i)));
    }

    // conflicts
    option_data.conflicts_index = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; option_data.conflicts[i].name != NULL; i++) {
        g_hash_table_insert(option_data.conflicts_index,
                (void *)option_data.conflicts[i].name,
                g_slist_prepend(g_hash_table_lookup(option_data.conflicts_index,
                        option_data.conflicts[i].name), GINT_TO_POINTER(i)));
    }
    return;
}

/**
 * @brief Free hash tables and lists created by arg_hash_generate()
 */
void arg_hash_cleanup(void)
{
    g_hash_table_destroy(option_data.options_index);
    g_hash_table_destroy(option_data.customs_index);
    g_hash_table_foreach(option_data.requires_index, free_slist_in_hash, NULL);
    g_hash_table_destroy(option_data.requires_index);
    g_hash_table_foreach(option_data.conflicts_index, free_slist_in_hash, NULL);
    g_hash_table_destroy(option_data.conflicts_index);
    g_free(option_data.options);
    // customs is all allocated on the stack
    // never reallocated to merge like the other sets
    g_free(option_data.requires);
    g_free(option_data.conflicts);
}

/**
 * @brief GSList freeing function to be passed to g_hash_table_foreach()
 *        This frees the lists allocated for requires_index and conflicts_index.
 *        Do not call directly.
 */
void free_slist_in_hash(
        __attribute__((unused)) gpointer key,
        gpointer slist,
        __attribute__((unused)) gpointer user_data)
{
    g_slist_free(slist);
}
