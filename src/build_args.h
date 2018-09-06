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

// missing versions share the same options with a previous version
typedef struct option_s option_t;

struct option_s {
    // long option name, prefixed with -- on command line
    gchar *name;
    // argument type as given to getopt in HandBrake
    enum {no_argument, optional_argument, required_argument} arg_type;
    // how hbr tries to interpret values in the keyfile
    enum {k_string, k_boolean, k_integer, k_double, k_string_list,
        k_boolean_list, k_integer_list, k_double_list} key_type;
    // TRUE if the option has a corresponding --no option
    // i.e. --markers and --no-markers
    gboolean negation_option;
    // function to validate argument
    gboolean (* valid_input)(option_t *option, void *value_p);
    // array of valid values, actual type depends on key_type
    int valid_values_count;
    void *valid_values;
};

GPtrArray * build_args(struct outfile outfile, struct config config, gboolean quoted);
GString * build_filename(struct outfile outfile, struct config config,
        gboolean full_path);
static gboolean strcmp_list(gchar *s, gchar **list, gsize len);
option_t * determine_handbrake_version();

// general input validation routines
gboolean valid_boolean(option_t *option, void *value_p);

gboolean valid_integer_set(option_t *option, void *value_p);
gboolean valid_integer_list(option_t *option, void *value_p);
gboolean valid_integer_list_set(option_t *option, void *value_p);
gboolean valid_positive_integer(option_t *option, void *value_p);

gboolean valid_positive_double_list(option_t *option, void *value_p);

gboolean valid_string(option_t *option, void *value_p);
gboolean valid_string_set(option_t *option, void *value_p);
gboolean valid_string_list_set(option_t *option, void *value_p);
gboolean valid_string_list(option_t *option, void *value_p);

// specific input validation routines
gboolean valid_optimize(option_t *option, void *value_p);
gboolean valid_filename_exists(option_t *option, void *value_p);
gboolean valid_filename_exists_list(option_t *option, void *value_p);
gboolean valid_filename_dne(option_t *option, void *value_p);
gboolean valid_startstop_at(option_t *option, void *value_p);
gboolean valid_previews(option_t *option, void *value_p);
gboolean valid_audio(option_t *option, void *value_p);
gboolean valid_audio_quality(option_t *option, void *value_p);
gboolean valid_audio_bitrate(option_t *option, void *value_p);
gboolean valid_video_quality(option_t *option, void *value_p);
gboolean valid_video_bitrate(option_t *option, void *value_p);
gboolean valid_crop(option_t *option, void *value_p);
gboolean valid_pixel_aspect(option_t *option, void *value_p);
gboolean valid_decomb(option_t *option, void *value_p);
gboolean valid_denoise(option_t *option, void *value_p);
gboolean valid_deblock(option_t *option, void *value_p);
gboolean valid_deinterlace(option_t *option, void *value_p);
gboolean valid_detelecine(option_t *option, void *value_p);
gboolean valid_iso639(option_t *option, void *value_p);
gboolean valid_iso639_list(option_t *option, void *value_p);
gboolean valid_native_dub(option_t *option, void *value_p);
gboolean valid_subtitle(option_t *option, void *value_p);
gboolean valid_gain(option_t *option, void *value_p);
gboolean valid_drc(option_t *option, void *value_p);
gboolean valid_mixdown(option_t *option, void *value_p);
gboolean valid_chapters(option_t *option, void *value_p);
gboolean valid_encopts(option_t *option, void *value_p);
gboolean valid_encoder_preset(option_t *option, void *value_p);
gboolean valid_encoder_tune(option_t *option, void *value_p);
gboolean valid_encoder_profile(option_t *option, void *value_p);
gboolean valid_encoder_level(option_t *option, void *value_p);
gboolean valid_nlmeans(option_t *option, void *value_p);
gboolean valid_nlmeans_tune(option_t *option, void *value_p);
gboolean valid_dither(option_t *option, void *value_p);
gboolean valid_subtitle_forced(option_t *option, void *value_p);
gboolean valid_codeset(option_t *option, void *value_p);
gboolean valid_rotate(option_t *option, void *value_p);
gboolean valid_qsv_decoding(option_t *option, void *value_p);
gboolean valid_comb_detect(option_t *option, void *value_p);
gboolean valid_pad(option_t *option, void *value_p);
gboolean valid_unsharp(option_t *option, void *value_p);
gboolean valid_filespec(option_t *option, void *value_p);
gboolean valid_preset_name(option_t *option, void *value_p);

#endif
