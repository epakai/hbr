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
#include <gio/gio.h>

#include "keyfile.h"

typedef struct option_s option_t;
typedef struct conflict_s conflict_t;
typedef struct require_s require_t;
typedef struct depend_s depend_t;

enum key_types {k_string, k_boolean, k_integer, k_double, k_string_list,
    k_integer_list, k_double_list};
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
    enum key_types key_type;
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
    gboolean (* valid_option)(option_t *option,  gchar *group, GKeyFile *config,
            const gchar *config_path);
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
 * @brief Data about which options require others.
 *        name may repeat for options with multiple requires.
 */
struct require_s {
    /**
     * @brief name of option being considered
     */
    gchar *name;
    /**
     * @brief option that name requires
     */
    gchar *require_name;
    /**
     * @brief optional, specific value of require_name that we require
     */
    gchar *require_value;
};

/**
 * @brief
 */
struct depend_s {
    /**
     * @brief name of option being considered
     */
    gchar *name;
    /**
     * @brief option that name depends on
     */
    gchar *depends_name;
    /**
     * @brief value for depends_name that this depend applies to
     */
    gchar *depends_value;
    /**
     * @brief type for valid_values
     */
    enum key_types key_type;
    /**
     * @brief acceptable values for 'name' given depends_name=depends_value
     */
    void *valid_values;
};

/*
 * Pointers to be set by determine_handbrake_version()
 */
option_t *version_options;
require_t *version_requires;
conflict_t *version_conflicts;
conflict_t *version_depends;
size_t version_options_size;
size_t version_requires_size;
size_t version_conflicts_size;
size_t version_depends_size;

/*
 * Pointers for dynamically allocated array that combines
 * options/hbr_options, requires/hbr_requires, and conflicts/hbr_conflicts
 */
option_t *options;
require_t *requires;
conflict_t *conflicts;
depend_t *depends;

/*
 * Hash tables for looking up index given an option name.
 * Indexes are stored as a int inside a pointer and must be
 * accessed using the GPOINTER_TO_INT() macro.
 */
GHashTable *options_index;
/*
 * These are similar, but store an GSList of values because options may require
 * or conflict with multiple other keys/values.
 * These values are also ints stored inside pointers.
 */
GHashTable *requires_index;
GHashTable *conflicts_index;
GHashTable *depends_index;

GPtrArray *build_args(GKeyFile *config, gchar *group, gboolean quoted);
gchar *build_filename(GKeyFile *config, gchar *group);
void append_year(GKeyFile *config, gchar *group, GString *path);

void determine_handbrake_version(gchar *arg_version);
void arg_hash_generate(void);
void arg_hash_cleanup(void);
void free_slist_in_hash(gpointer key, gpointer slist, gpointer user_data);

#endif
