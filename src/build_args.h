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

#ifndef _build_args_h
#define _build_args_h

#include <glib.h>

#include "keyfile.h"

typedef struct option_s option_t;
typedef struct conflict_s conflict_t;
typedef struct depend_s depend_t;

/**
 * @brief Data about options that get passed to HandBrakeCLI.
 *        name should be unique in the set of options.
 */
struct option_s {
    /**
     * @brief long option name, prefixed with -- on command line
     */
    gchar *name;
    /**
     * @brief argument type as given to getopt in HandBrake
     */
    enum {no_argument, optional_argument, required_argument, hbr_only} arg_type;
    /**
     * @brief how hbr tries to interpret values in the keyfile
     */
    enum {k_string, k_boolean, k_integer, k_double, k_string_list,
        k_integer_list, k_double_list} key_type;
    /**
     * @brief TRUE if the option has a corresponding --no option
     *        i.e. --markers and --no-markers
     */
    gboolean negation_option;
    /**
     * @brief function to validate arguments
     *
     * @param The option type to be validated against
     * @param config Keyfile to fetch values from
     * @param group Group name inside keyfile to fetch values from
     */
    gboolean (* valid_input)(option_t *option,  gchar *group, GKeyFile *config,
            gchar *config_path);
    /**
     * @brief number of valid values kept in the following array
     */
    int valid_values_count;
    /**
     * @brief array of valid values, actual type depends on key_type
     */
    void *valid_values;
};

/**
 * @brief Data about which options conflict.
 *        name may repeat for options with multiple conflicts.
 */
struct conflict_s {
    /**
     * @brief name of option being considered
     */
    gchar *name;
    /**
     * @brief option that name conflicts with
     */
    gchar *conflict_name;
    /**
     * @brief optional, specific value of conflict_name that we conflict with
     */
    gchar *conflict_value;
};

/**
 * @brief Data about which options depend on others.
 *        name may repeat for options with multiple depends.
 */
struct depend_s {
    /**
     * @brief name of option being considered
     */
    gchar *name;
    /**
     * @brief option that name depends on
     */
    gchar *depend_name;
    /**
     * @brief optional, specific value of depend_name that we depend on
     */
    gchar *depend_value;
};

/*
 * Pointers to be set by determine_handbrake_version()
 */
option_t *version_options;
depend_t *version_depends;
conflict_t *version_conflicts;
size_t version_options_size;
size_t version_depends_size;
size_t version_conflicts_size;

/*
 * Pointers for dynamically allocated array that combines
 * options/hbr_options, depends/hbr_depends, and conflicts/hbr_conflicts
 */
option_t *options;
depend_t *depends;
conflict_t *conflicts;

/*
 * Hash tables for looking up index given an option name.
 * Indexes are stored as a int inside a pointer and must be
 * accessed using the GPOINTER_TO_INT() macro.
 */
GHashTable *options_index;
/*
 * These are similar, but store an GSList of values because options may depend
 * on, or conflict with multiple other keys/values.
 * These values are also ints stored inside pointers.
 */
GHashTable *depends_index;
GHashTable *conflicts_index;

GPtrArray *build_args(GKeyFile *config, gchar *group, gboolean quoted);
GString *build_filename(GKeyFile *config, gchar *group, gboolean full_path);
void determine_handbrake_version(gchar *arg_version);
void arg_hash_generate();
void arg_hash_cleanup();
void free_slist_in_hash(gpointer key, gpointer slist, gpointer user_data);

#endif
