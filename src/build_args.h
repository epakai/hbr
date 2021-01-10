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

/// option type
typedef struct option_s option_t;
/// custom types
typedef struct custom_key_s custom_key_t;
typedef struct custom_s custom_t;
/// conflict type
typedef struct conflict_s conflict_t;
/// require type
typedef struct require_s require_t;

/**
 * @brief Possible key types (based on GKeyFile types)
 */
typedef enum key_types {k_string, k_boolean, k_integer, k_double, k_string_list,
    k_integer_list, k_double_list, k_path, k_path_list} key_type;

/**
 * @brief Data about options that get passed to HandBrakeCLI.
 *        name should be unique in the set of options.
 */
struct option_s {
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
    gboolean (* valid_option)(option_t *option, const gchar *group, GKeyFile *config,
            const gchar *config_path);
    /**
     * @brief number of valid values kept in the following array
     */
    gint valid_values_count;
    /**
     * @brief array of valid values, actual type depends on key_type
     */
    void *valid_values;
};

struct custom_key_s {
    key_type key_type;
    const gchar *key_name;
};

struct custom_s {
    const gchar *name;
    const void *key;
};

/**
 * @brief Data about which options conflict.
 *        name may repeat for options with multiple conflicts.
 */
struct conflict_s {
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
};

/**
 * @brief Data about which options require others.
 *        name may repeat for options with multiple requires.
 */
struct require_s {
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
};


/*
 * Pointers to be set by determine_handbrake_version()
 */
/// temporary pointer for options before they are combined with hbr options
extern option_t *version_options;
/// temporary pointer for custom filters before they are combined with hbr options
extern custom_t *version_customs;
/// temporary pointer for requires before they are combined with hbr requires
extern require_t *version_requires;
/// temporary pointer for conflicts before they are combined with hbr conflicts
extern conflict_t *version_conflicts;
/// options array size. used to join version_options and hbr_options with memcpy
extern size_t version_options_size;
/// custom filters array size. used to join version_options and hbr_options with memcpy
extern size_t version_customs_size;
/// requires array size. used to join version_requires and hbr_requires with memcpy
extern size_t version_requires_size;
/// conflicts array size. used to join version_conflicts and hbr_conflicts with memcpy
extern size_t version_conflicts_size;

/*
 * Pointers for dynamically allocated array that combines
 * options/hbr_options, requires/hbr_requires, and conflicts/hbr_conflicts
 */
/// pointer to final array of all options
extern option_t *options;
/// pointer to final array of all custom filters
extern custom_t *customs;
/// pointer to final array of all requires
extern require_t *requires;
/// pointer to final array of all conflicts
extern conflict_t *conflicts;

/*
 * Hash tables for looking up index given an option name.
 * Indexes are stored as a int inside a pointer and must be
 * accessed using the GPOINTER_TO_INT() macro.
 */
/// options look-up index generated by arg_hash_generate()
extern GHashTable *options_index;
extern GHashTable *customs_index;
/*
 * These are similar, but store an GSList of values because options may require
 * or conflict with multiple other keys/values.
 * These values are also ints stored inside pointers.
 */
/// requires look-up index generated by arg_hash_generate()
extern GHashTable *requires_index;
/// conflicts look-up index generated by arg_hash_generate()
extern GHashTable *conflicts_index;

GPtrArray *build_args(GKeyFile *config, const gchar *group, gboolean quoted);
void build_arg_string(GKeyFile *config, const gchar *group, GPtrArray *args, gint i, gboolean param_quoted);
void build_arg_boolean(GKeyFile *config, const gchar *group, GPtrArray *args, gint i);
void build_arg_integer(GKeyFile *config, const gchar *group, GPtrArray *args, gint i);
void build_arg_double(GKeyFile *config, const gchar *group, GPtrArray *args, gint i);
void build_arg_string_list(GKeyFile *config, const gchar *group, GPtrArray *args, gint i, gboolean param_quoted);
void build_arg_integer_list(GKeyFile *config, const gchar *group, GPtrArray *args, gint i);
void build_arg_double_list(GKeyFile *config, const gchar *group, GPtrArray *args, gint i);
gchar *build_filename(GKeyFile *config, const gchar *group);
void append_year(GKeyFile *config, const gchar *group, GString *path);

void determine_handbrake_version(gchar *arg_version);
void arg_hash_generate(void);
void arg_hash_cleanup(void);
void free_slist_in_hash( __attribute__((unused)) gpointer key,
        gpointer slist,
        __attribute__((unused)) gpointer user_data);

#endif
