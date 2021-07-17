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

#ifndef _options_h
#define _options_h

#include <glib.h>


/**
 * @brief Possible key types (based on GKeyFile types)
 */
typedef enum key_types {k_string, k_boolean, k_integer, k_double, k_string_list,
    k_integer_list, k_double_list, k_path, k_path_list} key_type;

/**
 * @brief Data about options that get passed to HandBrakeCLI.
 *        name should be unique in the set of options.
 */
typedef struct option_s {
    /**
     * @brief long option name, prefixed with -- on command line
     */
    const gchar *name;
    /**
     * @brief argument type as given to getopt in HandBrake
     */
    enum {no_argument, optional_argument, required_argument, hbr_only} arg_type;
    /**
     * @brief how hbr tries to interpret values in the keyfile
     */
    key_type key_type;
    /**
     * @brief TRUE if the option has a corresponding --no option
     *        i.e. --markers and --no-markers
     */
    gboolean negation_option;
    /**
     * @brief function to validate arguments
     *
     * @param option        The option type to be validated against
     * @param config        Keyfile to fetch values from
     * @param group         Group name inside keyfile to fetch values from
     * @param config_path   Path name to config
     * @param global_config Global config for validating some options
     */
    gboolean (* valid_option)(struct option_s *option, const gchar *group, GKeyFile *config,
            const gchar *config_path);
    /**
     * @brief number of valid values kept in the following array
     */
    gint valid_values_count;
    /**
     * @brief array of valid values, actual type depends on key_type
     */
    void *valid_values;
} option_t;

typedef struct {
    key_type key_type;
    const gchar *key_name;
} custom_key_t;

typedef struct {
    const gchar *name;
    const void *key;
} custom_t;

/**
 * @brief Data about which options conflict.
 *        name may repeat for options with multiple conflicts.
 */
typedef struct {
    /**
     * @brief name of option being considered
     */
    const gchar *name;
    /**
     * @brief optional, specific value of name that we are concerned with
     */
    const gchar *value;
    /**
     * @brief option that name conflicts with
     */
    const gchar *conflict_name;
    /**
     * @brief optional, specific value of conflict_name that we conflict with
     */
    const gchar *conflict_value;
} conflict_t;

/**
 * @brief Data about which options require others.
 *        name may repeat for options with multiple requires.
 */
typedef struct {
    /**
     * @brief name of option being considered
     */
    const gchar *name;
    /**
     * @brief option that name requires
     */
    const gchar *require_name;
    /**
     * @brief optional, specific value of require_name that we require
     */
    const gchar *require_value;
} require_t;

typedef struct {
    /*
     * Pointers for dynamically allocated array that combines
     * options/hbr_options, requires/hbr_requires, and conflicts/hbr_conflicts
     */
    option_t *options;
    custom_t *customs;
    require_t *requires;
    conflict_t *conflicts;

    /// Hash tables are built in arg_hash_generate()
    /*
     * Hash tables for looking up index given an option name.
     * Indexes are stored as a int inside a pointer and must be
     * accessed using the GPOINTER_TO_INT() macro.
     */
    GHashTable *options_index;
    GHashTable *customs_index;
    /*
     * These are similar, but store an GSList of values because options may require
     * or conflict with multiple other keys/values.
     * These values are also ints stored inside pointers.
     */
    GHashTable *requires_index;
    GHashTable *conflicts_index;
} option_data_t;

void determine_handbrake_version(gchar *arg_version);
void arg_hash_generate(void);
void arg_hash_cleanup(void);
void free_slist_in_hash( __attribute__((unused)) gpointer key,
        gpointer slist,
        __attribute__((unused)) gpointer user_data);

#endif
