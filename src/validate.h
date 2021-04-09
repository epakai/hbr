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

#ifndef _validate_h
#define _validate_h

#include <glib.h>
#include <gio/gio.h>

#include "options.h"

gboolean pre_validate_key_file(const gchar *infile);
gboolean post_validate_input_file(GKeyFile *input_keyfile, const gchar *infile,
        GKeyFile *config_keyfile);
gboolean post_validate_config_file(GKeyFile *keyfile, const gchar *infile);
gboolean post_validate_common(GKeyFile *keyfile, const gchar *infile,
        GKeyFile *config_keyfile);
gboolean has_required_keys(GKeyFile *input_keyfile, const gchar *infile,
        GKeyFile *config_keyfile);
gboolean has_requires(GKeyFile *input_keyfile, const gchar *infile,
        GKeyFile *config_keyfile);
gboolean unknown_keys_exist(GKeyFile *keyfile, const gchar *infile);
gboolean has_duplicate_groups(const gchar *infile);
gboolean has_duplicate_keys(const gchar *infile);
gboolean check_custom_format (GKeyFile *config, const gchar *group,
        option_t *option, const gchar *config_path);
void type_config_warnings(gchar *type, gboolean has_season,
        gboolean has_episode, gboolean has_year, GKeyFile *config,
        const gchar *config_path);
void type_outfile_warnings(gchar *type, gboolean has_season,
        gboolean has_episode, gboolean has_year, const gchar *group,
        GKeyFile *config, const gchar *config_path);
/*
 * Input validation for hbr specific options
 */
gboolean valid_type(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_readable_path(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_writable_path(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_filename_component(option_t *option, const gchar *group,
        GKeyFile *config,  const gchar *config_path);

// general input validation routines
gboolean valid_boolean(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_integer(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_integer_set(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_integer_list(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_integer_list_set(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_positive_integer(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_double_list(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_positive_double_list(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_string(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_string_set(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_string_list_set(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_string_list(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);

// specific input validation routines
gboolean valid_filename_exists(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_filename_exists_list(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_filename_dne(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_startstop_at(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_previews(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_audio(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_audio_encoder(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_audio_quality(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_audio_bitrate(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_audio_compression(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_video_quality(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_video_bitrate(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_video_framerate(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_chroma(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_crop(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_pixel_aspect(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_combined_decomb_deblock_deinterlace_comb_detect(option_t *option,
    const gchar *group, GKeyFile *config, const gchar *config_path);
gboolean valid_decomb(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_denoise(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_deblock(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_deinterlace(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_detelecine(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_iso_639(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_iso_639_list(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_native_dub(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_subtitle(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_gain(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_drc(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_mixdown(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_chapters(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_encopts(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_encoder_preset(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_encoder_tune(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_encoder_profile(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_encoder_level(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_nlmeans(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_nlmeans_tune(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_dither(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_subtitle_forced(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_subtitle_burned(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_subtitle_default(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_codeset(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_rotate(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_qsv_decoding(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_comb_detect(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_pad(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_unsharp(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_filespec(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);
gboolean valid_preset_name(option_t *option, const gchar *group, GKeyFile *config,
         const gchar *config_path);

#endif
