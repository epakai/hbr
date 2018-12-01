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

struct option_s {
    // long option name, prefixed with -- on command line
    gchar *name;
    // argument type as given to getopt in HandBrake
    enum {no_argument, optional_argument, required_argument, hbr_only} arg_type;
    // how hbr tries to interpret values in the keyfile
    enum {k_string, k_boolean, k_integer, k_double, k_string_list,
        k_integer_list, k_double_list} key_type;
    // TRUE if the option has a corresponding --no option
    // i.e. --markers and --no-markers
    gboolean negation_option;
    // function to validate argument
    gboolean (* valid_input)(option_t *option, GKeyFile *config, gchar *group);
    // array of valid values, actual type depends on key_type
    int valid_values_count;
    void *valid_values;
};

struct conflict_s {
    // name of option being considered
    gchar *name;
    // option that name conflicts with
    gchar *conflict_name;
    // optional, specific value of conflict_name that we conflict with
    gchar *conflict_value;
};

struct depend_s {
    // name of option being considered
    gchar *name;
    // option that name depends on
    gchar *depend_name;
    // optional, specific value of conflict_name that we depend on
    gchar *depend_value;
};

/*
 * Pointers to be set by determine_handbrake_version()
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

/*
 * Input validation for hbr specific options
 */
gboolean valid_readable_path(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_writable_path(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_filename_string(option_t *option, GKeyFile *config, gchar *group);

// general input validation routines
gboolean valid_boolean(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_integer(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_integer_set(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_integer_list(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_integer_list_set(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_positive_integer(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_positive_double_list(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_string(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_string_set(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_string_list_set(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_string_list(option_t *option, GKeyFile *config, gchar *group);

// specific input validation routines
gboolean valid_optimize(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_filename_exists(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_filename_exists_list(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_filename_dne(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_startstop_at(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_previews(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_audio(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_audio_quality(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_audio_bitrate(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_video_quality(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_video_bitrate(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_crop(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_pixel_aspect(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_decomb(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_denoise(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_deblock(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_deinterlace(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_detelecine(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_iso639(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_iso639_list(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_native_dub(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_subtitle(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_gain(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_drc(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_mixdown(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_chapters(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_encopts(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_encoder_preset(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_encoder_tune(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_encoder_profile(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_encoder_level(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_nlmeans(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_nlmeans_tune(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_dither(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_subtitle_forced(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_codeset(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_rotate(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_qsv_decoding(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_comb_detect(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_pad(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_unsharp(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_filespec(option_t *option, GKeyFile *config, gchar *group);
gboolean valid_preset_name(option_t *option, GKeyFile *config, gchar *group);

/*
 * hbr specific options tables
 * These aren't valid options to pass to HandBrakeCLI.
 * They are used to generate file names or specify file locations
 */

static option_t hbr_options[] =
{
    { "type", hbr_only, k_string, FALSE, valid_string_set, 2, (gchar*[]){"series", "movie"}},
    { "input_basedir", hbr_only, k_string, FALSE, valid_readable_path, 0, NULL},
    { "output_basedir", hbr_only, k_string, FALSE, valid_writable_path, 0, NULL},
    { "iso_filename", hbr_only, k_string, FALSE, valid_filename_string, 0, NULL},
    { "name", hbr_only, k_string, FALSE, valid_filename_string, 0, NULL},
    { "year", hbr_only, k_integer, FALSE, valid_integer, 0, NULL},
    { "season", hbr_only, k_integer, FALSE, valid_integer, 0, NULL},
    { "episode", hbr_only, k_integer, FALSE, valid_integer, 0, NULL},
    { "specific_name", hbr_only, k_string, FALSE, valid_filename_string, 0, NULL},
    { NULL, 0, 0, 0, 0, 0}
};

static depend_t hbr_depends[] =
{
    {"year", "type", "movie"},
    {"season", "type", "series"},
    {"episode", "type", "series"},
    { NULL, 0, 0}
};

static conflict_t hbr_conflicts[] =
{
    { NULL, 0, 0}
};

#endif
