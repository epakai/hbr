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

/**
 * @brief 
 *
 * @param outfile
 * @param config
 * @param quoted
 *
 * @return 
 */
GPtrArray * build_args(struct outfile outfile, struct config config, gboolean quoted)
{
    GPtrArray *args = g_ptr_array_new_full(30, g_free);
    // base args (format, markers)
    if (config.set.format) {
        if (strcmp(config.key.format, "mkv") == 0) {
            g_ptr_array_add(args, g_strdup("-f"));
            g_ptr_array_add(args, g_strdup("av_mkv"));
        }
        else if (strcmp(config.key.format, "m4v") == 0) {
            g_ptr_array_add(args, g_strdup("-f"));
            g_ptr_array_add(args, g_strdup("av_m4v"));
        }
    }
    if (config.set.markers) {
        if (config.key.markers == TRUE) {
            g_ptr_array_add(args, g_strdup("-m"));
        }
    }

    // picture args (crop can come from outfile section)
    if (config.set.picture_anamorphic) {
        if (strcmp(config.key.picture_anamorphic, "strict") == 0) {
            g_ptr_array_add(args, g_strdup("--strict-anamorphic"));
        }
        else if (strcmp(config.key.picture_anamorphic, "loose") == 0) {
            g_ptr_array_add(args, g_strdup("--loose-anamorphic"));
        }
    }
    if (config.set.picture_loose_crop) {
        g_ptr_array_add(args, g_strdup("--loose-crop"));
        if (config.key.picture_loose_crop > 0 && config.key.picture_loose_crop < 4000) {
            g_ptr_array_add(args, g_strdup_printf("%d", config.key.picture_loose_crop));
        }
    }
    // TODO handle crops individually (if only some crops are set, zero the others
    if (outfile.set.crop_top && outfile.set.crop_bottom &&
            outfile.set.crop_left && outfile.set.crop_right) {
        // try outfile crop first
        g_ptr_array_add(args, g_strdup("--crop"));
        g_ptr_array_add(args, g_strdup_printf("%d:%d:%d:%d", outfile.key.crop_top,
                outfile.key.crop_bottom, outfile.key.crop_left,
                outfile.key.crop_right));
    } else if (config.set.picture_crop_top && config.set.picture_crop_bottom &&
            config.set.picture_crop_left && config.set.picture_crop_right) {
        // then try config crop
        g_ptr_array_add(args, g_strdup("--crop"));
        g_ptr_array_add(args, g_strdup_printf("%d:%d:%d:%d",
                    config.key.picture_crop_top, config.key.picture_crop_bottom,
                    config.key.picture_crop_left, config.key.picture_crop_right));
    } else if (config.set.picture_autocrop && config.key.picture_autocrop) {
        g_ptr_array_add(args, g_strdup("--crop"));
    }

    // filter args
    if (config.set.filter_deinterlace) {
        if ( strcmp(config.key.filter_deinterlace, "none") == 0) {
            // no arg
        } else {
            gchar * list[] = {"fast", "slow", "slower", "bob"};
            int list_count = (sizeof (list) / sizeof (gchar *));
            if (strcmp(config.key.filter_deinterlace, "default") == 0) {
                g_ptr_array_add(args, g_strdup("-d"));
            } else if (strcmp_list(config.key.filter_deinterlace, list, list_count)){
                g_ptr_array_add(args, g_strdup("-d"));
                g_ptr_array_add(args, g_strdup_printf("%s",
                            config.key.filter_deinterlace));
            }
        }
    } else if (config.set.filter_decomb) {
        if (strcmp(config.key.filter_decomb, "default") == 0) {
            g_ptr_array_add(args, g_strdup("-5"));
        } else if (strcmp(config.key.filter_decomb, "fast") == 0) {
            g_ptr_array_add(args, g_strdup("-5"));
            g_ptr_array_add(args, g_strdup("fast"));
        } else if (strcmp(config.key.filter_decomb, "bob") == 0) {
            g_ptr_array_add(args, g_strdup("-5"));
            g_ptr_array_add(args, g_strdup("bob"));
        }
    }
    if (config.set.filter_denoise) {
        gchar * list[] = {"ultralight", "light", "medium", "strong"};
        int list_count = (sizeof (list) / sizeof (const char *));
        if (strcmp(config.key.filter_denoise, "default") == 0) {
            g_ptr_array_add(args, g_strdup("-8"));
        } else if (strcmp_list(config.key.filter_denoise, list, list_count)) {
            g_ptr_array_add(args, g_strdup("-8"));
            g_ptr_array_add(args, g_strdup_printf("%s", config.key.filter_denoise));
        }
    }
    if (config.set.filter_grayscale) {
        if (config.key.filter_grayscale) {
            g_ptr_array_add(args, g_strdup("-g"));
        }
    }
    if (config.set.filter_rotate) {
        gint rotate_mode = 0;
        for (int i = 0; i < config.key.filter_rotate_count; i++) {
            if (config.key.filter_rotate[i] == NULL) {
                continue;
            }
            if (strcmp(config.key.filter_rotate[i], "none") == 0) {
                rotate_mode = -1;
                break;
            }
            if (strcmp(config.key.filter_rotate[i], "default") == 0) {
                rotate_mode = 3;
                break;
            }
            if (strcmp(config.key.filter_rotate[i], "vertical_flip") == 0) {
                rotate_mode = rotate_mode + 1;
            }
            if (strcmp(config.key.filter_rotate[i], "horizontal_flip") == 0) {
                rotate_mode = rotate_mode + 2;
            }
            if (strcmp(config.key.filter_rotate[i], "rotate_clockwise") == 0) {
                rotate_mode = rotate_mode + 4;
            }
        }
        if (rotate_mode == 0) {
            g_ptr_array_add(args, g_strdup("--rotate"));
        } else if (rotate_mode != -1) {
            g_ptr_array_add(args, g_strdup("--rotate"));
            g_ptr_array_add(args, g_strdup_printf("%d", rotate_mode));
        }
    }

    // audio args
    if (config.set.audio_encoder) {
        gchar * list[] = {"av_aac", "copy:aac", "ac3", "copy:ac3", "copy:dts",
            "copy_dtshd", "mp3", "copy:mp3", "vorbis", "flac16", "flac24", "copy"};
        int list_count = (sizeof (list) / sizeof (const char *));
        if (strcmp_list(config.key.audio_encoder, list, list_count)){
            g_ptr_array_add(args, g_strdup("-E"));
            g_ptr_array_add(args, g_strdup_printf("%s", config.key.audio_encoder));
        }
    }
    if (config.set.audio_bitrate) {
        g_ptr_array_add(args, g_strdup("-B"));
        g_ptr_array_add(args, g_strdup_printf("%d", config.key.audio_bitrate));
    }
    if (config.set.audio_quality) {
        g_ptr_array_add(args, g_strdup("-Q"));
        g_ptr_array_add(args, g_strdup_printf("%d", config.key.audio_quality));
    }

    // video args
    if (config.set.video_encoder) {
        gchar * list[] = {"x264", "x265", "mpeg4", "mpeg2", "VP8", "theora"};
        int list_count = (sizeof (list) / sizeof (const char *));
        if (strcmp_list(config.key.video_encoder, list, list_count)) {
            g_ptr_array_add(args, g_strdup_printf("-e"));
            g_ptr_array_add(args, g_strdup_printf("%s", config.key.video_encoder));
        }
    }
    if (config.set.video_framerate) {
        gchar * list[] = {"5", "10", "12", "15", "23.976", "24", "25",
            "29.97", "30", "50", "59.94", "60"};
        int list_count = (sizeof (list) / sizeof (const char *));
        if (strcmp_list(config.key.video_framerate, list, list_count)) {
            g_ptr_array_add(args, g_strdup("-r"));
            g_ptr_array_add(args, g_strdup_printf("%s", config.key.video_framerate));
        }
    }
    if (config.set.video_framerate_control) {
        if (strcmp(config.key.video_framerate_control, "constant") == 0) {
            g_ptr_array_add(args, g_strdup("--cfr"));
        } else if (strcmp(config.key.video_framerate_control, "variable") == 0) {
            g_ptr_array_add(args, g_strdup("--vfr"));
        } else if (strcmp(config.key.video_framerate_control, "peak") == 0) {
            g_ptr_array_add(args, g_strdup("--pfr"));
        }
    }
    if (config.set.video_bitrate) {
        g_ptr_array_add(args, g_strdup("-b"));
        g_ptr_array_add(args, g_strdup_printf("%d", config.key.video_bitrate));
        if (config.set.video_two_pass) {
            if (config.key.video_two_pass) {
                g_ptr_array_add(args, g_strdup("-2"));
                if (config.set.video_turbo) {
                    if (config.key.video_turbo) {
                        g_ptr_array_add(args, g_strdup("-T"));
                    }
                }
            }
        }
    }
    if (config.set.video_quality) {
        g_ptr_array_add(args, g_strdup("-q"));
        g_ptr_array_add(args, g_strdup_printf("%d", config.key.video_quality));
    }

    // title and chapters arg
    if (outfile.set.dvdtitle) {
        g_ptr_array_add(args, g_strdup("-t"));
        g_ptr_array_add(args, g_strdup_printf("%d", outfile.key.dvdtitle));
    }
    if (outfile.set.chapters_start && outfile.set.chapters_end) {
        if (outfile.key.chapters_start == outfile.key.chapters_end) {
            g_ptr_array_add(args, g_strdup("-c"));
            g_ptr_array_add(args, g_strdup_printf("%d", outfile.key.chapters_start));
        } else {
            g_ptr_array_add(args, g_strdup("-c"));
            g_ptr_array_add(args, g_strdup_printf("%d-%d",
                    outfile.key.chapters_start, outfile.key.chapters_end));
        }
    }

    // audio tracks arg
    if (outfile.set.audio) {
        g_ptr_array_add(args, g_strdup("-a"));
        int i=0;
        GString *temp = g_string_new(NULL);
        for (; i < outfile.key.audio_count-1; i++) {
            g_string_append_printf(temp, "%d,", outfile.key.audio[i]);
        }
        g_string_append_printf(temp, "%d", outfile.key.audio[i]);
        g_ptr_array_add(args, temp->str);
        g_string_free(temp, FALSE); // keep string data, toss GString
    }

    // subtitle tracks arg
    if (outfile.set.subtitle) {
        g_ptr_array_add(args, "-s");
        int i=0;
        GString *temp = g_string_new(NULL);
        for (; i < outfile.key.subtitle_count-1; i++) {
            g_string_append_printf(temp, "%d,", outfile.key.subtitle[i]);
        }
        g_string_append_printf(temp, "%d", outfile.key.subtitle[i]);
        g_ptr_array_add(args, temp->str);
        g_string_free(temp, FALSE); // keep string data, toss GString
    }
    
    /*
     * Input and Output files are handled using keys specific to hbr
     */
    // input file arg (depends on input_basedir, iso_filename)
    g_ptr_array_add(args, g_strdup("-i"));
    GString *infile = g_string_new(NULL);
    g_string_append_printf(infile, "%s", config.key.input_basedir);
    if (infile->str[infile->len-1] != G_DIR_SEPARATOR){
        g_string_append(infile, G_DIR_SEPARATOR_S);
    }
    g_string_append_printf(infile, "%s", outfile.key.iso_filename);
    if (quoted) {
        g_ptr_array_add(args, g_shell_quote(infile->str));
    } else {
        g_ptr_array_add(args, infile->str);
    }
    g_string_free(infile, FALSE); // keep string data, toss GString

    // output file arg (depends on type, name, year, season, episode_number, specific_name)
    g_ptr_array_add(args, g_strdup("-o"));
    GString *filename = build_filename(outfile, config, TRUE);
    if (quoted) {
        g_ptr_array_add(args, g_shell_quote(filename->str));
    } else {
        g_ptr_array_add(args, filename->str);
    }
    g_string_free(filename, FALSE); // keep string data, toss GString
    // Couldn't find documents saying GPtrArray is null terminated so we'll do that
    g_ptr_array_add(args, g_strdup(NULL));
    return args;
}

GString * build_filename(struct outfile outfile, struct config config, gboolean full_path)
{
    GString* filename = g_string_new(NULL);
    if (full_path && config.set.output_basedir) {
        g_string_append(filename, config.key.output_basedir);
        if (filename->str[filename->len-1] != G_DIR_SEPARATOR){
            g_string_append(filename, G_DIR_SEPARATOR_S);
        }
    }
    g_string_append_printf(filename, "%s", outfile.key.name);
    if (strcmp(outfile.key.type, "movie") == 0) {
        if (outfile.set.year) {
            g_string_append_printf(filename, " (%s)", outfile.key.year);
        }
    } else if ( strcmp(outfile.key.type, "series") == 0) {
        if (outfile.set.season || outfile.set.episode_number) {
            g_string_append(filename, " - ");
            if (outfile.set.season) {
                g_string_append_printf(filename, "s%02d", outfile.key.season);
            }
            if (outfile.set.episode_number) {
                g_string_append_printf(filename, "e%03d", outfile.key.episode_number);
            }
        }
    }
    if (outfile.set.specific_name) {
        g_string_append_printf(filename, " - %s", outfile.key.specific_name);
    }
    if (config.set.format) {
        g_string_append_printf(filename, ".%s", config.key.format);
    } else {
        g_string_append(filename, ".mkv");
    }
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
option_t * determine_handbrake_version(gchar *arg_version)
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
     * NOTE: This logic is over-simplified because versions where changes
     * occured are tightly packed. Think hard about this when adding new
     * versions.
     */

    if (major > 1 || (major == 1 && minor > 1) ||
            (major == 1 && minor == 1 && patch > 1)) {
        fprintf(stderr, "Found newer HandBrake release (%s) than supported. "
                "Running with newest options available.", version);
        return v1_1_0;
    }
    if (major == 1 && minor > 0) {
        return v1_1_0;
    }
    if (major == 1 && minor >= 0) {
        return v1_0_0;
    }
    if (major == 0 && minor == 10 && patch >= 3) {
        return v0_10_3;
    }
    if (major == 0 && minor == 10) {
        return v0_10_0;
    }
    if (major == 0 && minor == 9 && patch >= 9) {
        return v0_9_9;
    }
    fprintf(stderr, "Could not match a supported HandBrake version. "
            " Trying oldest release available (0.9.9).");
    return v0_9_9;
}

//TODO add asserts that valid_values and valid_values_count are NULL/0
//TODO add asserts throughout all vail_ functions because there's lots
// of ways to muck those files up
//TODO all-subtiles/first-subtitle all-audio/first-audio conflict
// but we don't have any way to handle this with valid_boolean
// may need to give them their own valid_function
gboolean valid_boolean(option_t *option, void *value_p)
{
    return FALSE;
}


gboolean valid_integer_set(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_integer_list(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_integer_list_set(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_positive_integer(option_t *option, void *value_p)
{
    return FALSE;
}


gboolean valid_positive_double_list(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_string(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_string_set(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_string_list_set(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_string_list(option_t *option, void *value_p)
{
    return FALSE;
}


gboolean valid_optimize(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_filename_exists(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_filename_exists_list(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_filename_dne(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_startstop_at(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_previews(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_audio(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_audio_quality(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_audio_bitrate(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_video_quality(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_video_bitrate(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_crop(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_pixel_aspect(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_decomb(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_denoise(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_deblock(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_deinterlace(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_detelecine(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_iso639(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_iso639_list(option_t *option, void *value_p)
{
    return FALSE;
}


gboolean valid_native_dub(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_subtitle(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_gain(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_drc(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_mixdown(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_chapters(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_encopts(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_encoder_preset(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_encoder_tune(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_encoder_profile(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_encoder_level(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_nlmeans(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_nlmeans_tune(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_dither(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_subtitle_forced(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_codeset(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_rotate(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_qsv_decoding(option_t *option, void *value_p)
{
    /*
     * this function validates enable and disable
     * it should verify conflicting options aren't on the same level
     */
    return FALSE;
}

gboolean valid_comb_detect(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_pad(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_unsharp(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_filespec(option_t *option, void *value_p)
{
    return FALSE;
}

gboolean valid_preset_name(option_t *option, void *value_p)
{
    return FALSE;
}
