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
#include <stdio.h>
#include <string.h>

GString * build_args(struct outfile outfile, struct config config)
{
    GString* args = g_string_new(NULL);
    // base args (format, markers)
    if (config.set.format) {
        if(strcmp(config.key.format, "mkv") == 0) {
            g_string_append(args, " -f av_mkv");
        }
        else if(strcmp(config.key.format, "m4v") == 0) {
            g_string_append(args, " -f av_m4v");
        }
    }
    if (config.set.markers) {
        if(config.key.markers == TRUE) {
            g_string_append(args, " -m");
        }
    }
    
    // picture args (crop can come from outfile section)
    if (config.set.picture_anamorphic) {
        if(strcmp(config.key.picture_anamorphic, "strict") == 0) {
            g_string_append(args, " --strict-anamorphic");
        }
        else if(strcmp(config.key.picture_anamorphic, "loose") == 0) {
            g_string_append(args, " --loose-anamorphic");
        }
    }
    if (config.set.picture_loose_crop) {
        g_string_append(args, " --loose-crop");
        if (config.key.picture_loose_crop > 0 && config.key.picture_loose_crop < 4000) {
            g_string_append_printf(args, " %d", config.key.picture_loose_crop);
        }
    }
    // TODO handle crops individually (if only some crops are set, zero the others
    if (outfile.set.crop_top && outfile.set.crop_bottom &&
            outfile.set.crop_left && outfile.set.crop_right) {
        // try outfile crop first
        g_string_append_printf(args, " --crop %d:%d:%d:%d", outfile.key.crop_top,
                outfile.key.crop_bottom, outfile.key.crop_left,
                outfile.key.crop_right);
    } else if (config.set.picture_crop_top && config.set.picture_crop_bottom &&
            config.set.picture_crop_left && config.set.picture_crop_right) {
        // then try config crop
        g_string_append_printf(args, " --crop %d:%d:%d:%d", config.key.picture_crop_top,
                config.key.picture_crop_bottom, config.key.picture_crop_left,
                config.key.picture_crop_right);
    } else if (config.set.picture_autocrop && config.key.picture_autocrop) {
        g_string_append(args, " --crop");
    }

    // filter args
    if (config.set.filter_deinterlace) {
        if ( strcmp(config.key.filter_deinterlace, "none") == 0) {
            // no arg
        } else {
            gchar * list[] = {"fast", "slow", "slower", "bob"};
            int list_count = (sizeof (list) / sizeof (gchar *));
            if (strcmp(config.key.filter_deinterlace, "default") == 0) {
                g_string_append(args, " -d");
            } else if (strcmp_list(config.key.filter_deinterlace, list, list_count)){
                g_string_append_printf(args, " -d %s",config.key.filter_deinterlace);
            }
        }
    } else if (config.set.filter_decomb) {
        if (strcmp(config.key.filter_decomb, "default") == 0) {
            g_string_append(args, " -5");
        } else if (strcmp(config.key.filter_decomb, "fast") == 0) {
            g_string_append(args, " -5 fast");
        } else if (strcmp(config.key.filter_decomb, "bob") == 0) {
            g_string_append(args, " -5 bob");
        }
    }
    if (config.set.filter_denoise) {
        gchar * list[] = {"ultralight", "light", "medium", "strong"};
        int list_count = (sizeof (list) / sizeof (const char *));
        if (strcmp(config.key.filter_denoise, "default") == 0) {
            g_string_append(args, " -8");
        } else if (strcmp_list(config.key.filter_denoise, list, list_count)) {
            g_string_append_printf(args, " -8 %s", config.key.filter_denoise);
        }
    }
    if (config.set.filter_grayscale) {
        if (config.key.filter_grayscale) {
            g_string_append(args, " -g");
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
            g_string_append(args, " --rotate");
        } else if (rotate_mode != -1) {
            g_string_append_printf(args, " --rotate %d", rotate_mode);
        }
    }

    // audio args
    if (config.set.audio_encoder) {
        gchar * list[] = {"av_aac", "copy:aac", "ac3", "copy:ac3", "copy:dts",
            "copy_dtshd", "mp3", "copy:mp3", "vorbis", "flac16", "flac24", "copy"};
        int list_count = (sizeof (list) / sizeof (const char *));
        if (strcmp_list(config.key.audio_encoder, list, list_count)){
            g_string_append_printf(args, " -E %s", config.key.audio_encoder);
        }
    }
    if (config.set.audio_bitrate) {
        g_string_append_printf(args, " -B %d", config.key.audio_bitrate);
    }
    if (config.set.audio_quality) {
        g_string_append_printf(args, " -Q %d", config.key.audio_quality);
    }

    // video args
    if (config.set.video_encoder) {
        gchar * list[] = {"x264", "x265", "mpeg4", "mpeg2", "VP8", "theora"};
        int list_count = (sizeof (list) / sizeof (const char *));
        if (strcmp_list(config.key.video_encoder, list, list_count)) {
            g_string_append_printf(args, " -e %s", config.key.video_encoder);
        }
    }
    if (config.set.video_framerate) {
        gchar * list[] = {"5", "10", "12", "15", "23.976", "24", "25",
            "29.97", "30", "50", "59.94", "60"};
        int list_count = (sizeof (list) / sizeof (const char *));
        if (strcmp_list(config.key.video_framerate, list, list_count)) {
            g_string_append_printf(args, " -r %s", config.key.video_framerate);
        }
    }
    if (config.set.video_framerate_control) {
        if (strcmp(config.key.video_framerate_control, "constant") == 0) {
            g_string_append(args, " --cfr");
        } else if (strcmp(config.key.video_framerate_control, "variable") == 0) {
            g_string_append(args, " --vfr");
        } else if (strcmp(config.key.video_framerate_control, "peak") == 0) {
            g_string_append(args, " --pfr");
        }
    }
    if (config.set.video_bitrate) {
        g_string_append_printf(args, " -b %d", config.key.video_bitrate);
        if (config.set.video_two_pass) {
            if (config.key.video_two_pass) {
                g_string_append(args, " -2");
                if (config.set.video_turbo) {
                    if (config.key.video_turbo) {
                        g_string_append(args, " -T");
                    }
                }
            }
        }
    }
    if (config.set.video_quality) {
        g_string_append_printf(args, " -q %d", config.key.video_quality);
    }

    // title and chapters arg
    if (outfile.set.dvdtitle) {
        g_string_append_printf(args, " -t %d", outfile.key.dvdtitle);
    }
    if (outfile.set.chapters_start && outfile.set.chapters_end) {
        if (outfile.key.chapters_start == outfile.key.chapters_end) {
            g_string_append_printf(args, " -c %d", outfile.key.chapters_start);
        } else {
            g_string_append_printf(args, " -c %d-%d",
                    outfile.key.chapters_start, outfile.key.chapters_end);
        }
    }

    // audio tracks arg
    if (outfile.set.audio) {
        g_string_append(args, " -a ");
        int i=0;
        for (; i < outfile.key.audio_count-1; i++) {
            g_string_append_printf(args, "%d,", outfile.key.audio[i]);
        }
        g_string_append_printf(args, "%d", outfile.key.audio[i]);
    }

    // subtitle tracks arg
    if (outfile.set.subtitle) {
        g_string_append(args, " -s ");
        int i=0;
        for (; i < outfile.key.subtitle_count-1; i++) {
            g_string_append_printf(args, "%d,", outfile.key.subtitle[i]);
        }
        g_string_append_printf(args, "%d", outfile.key.subtitle[i]);
    }

    // input file arg (depends on input_basedir, iso_filename)
    g_string_append_printf(args, " -i \"%s", config.key.input_basedir);
    if (args->str[args->len-1] != G_DIR_SEPARATOR){
        g_string_append(args, G_DIR_SEPARATOR_S);
    }
    g_string_append_printf(args, "%s\"", outfile.key.iso_filename);
    
    // output file arg (depends on type, name, year, season, episode_number, specific_name)
    g_string_append(args, " -o \"");
    GString *filename = build_filename(outfile, config, TRUE);
    g_string_append(args, filename->str);
    g_string_free(filename, TRUE);
    g_string_append(args, "\"");
    return args;
}

GString * build_filename(struct outfile outfile, struct config config, gboolean full_path)
{
    GString* args = g_string_new(NULL);
    if (full_path && config.set.output_basedir) {
        g_string_append(args, config.key.output_basedir);
        if (args->str[args->len-1] != G_DIR_SEPARATOR){
            g_string_append(args, G_DIR_SEPARATOR_S);
        }
    }
    g_string_append_printf(args, "%s", outfile.key.name);
    if (strcmp(outfile.key.type, "movie") == 0) {
        if (outfile.set.year) {
            g_string_append_printf(args, " (%s)", outfile.key.year);
        }
    } else if ( strcmp(outfile.key.type, "series") == 0) {
        if (outfile.set.season || outfile.set.episode_number) {
            g_string_append(args, " - ");
            if (outfile.set.season) {
                g_string_append_printf(args, "s%02d", outfile.key.season);
            }
            if (outfile.set.episode_number) {
                g_string_append_printf(args, "e%03d", outfile.key.episode_number);
            }
        }
    }
    if (outfile.set.specific_name) {
        g_string_append_printf(args, " - %s", outfile.key.specific_name);
    }
    if (config.set.format) {
        g_string_append_printf(args, ".%s", config.key.format);
    } else {
        g_string_append(args, ".mkv");
    }
    return args;
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
gboolean strcmp_list(gchar *s, gchar **list, gsize len) {
    for (gsize i = 0; i< len; i++) {
        if (strcmp(s, list[i]) == 0)
            return TRUE;
    }
    return FALSE;
}

/**
 * @brief Ensure bitrate is one of the preset values
 *
 * @param bitrate bitrate for audio
 * @param minimum set range for valid bitrate (depending on codec)
 * @param maximum set range for valid bitrate (depending on codec)
 *
 * @return boolean status
 */
gboolean valid_bit_rate(int bitrate, int minimum, int maximum)
{
    if (bitrate < 0 || minimum < 0 || maximum < 0) {
        fprintf(stderr, "Negative value passed to valid_bit_rate()\n");
        return FALSE;
    }
    if (minimum > maximum) {
        fprintf(stderr, "Minimum exceeds maximum in valid_bit_rate()\n");
        return FALSE;
    }

    // list is slightly reordered to put common rates first
    int valid_bitrates[] = { 128, 160, 192, 224, 256, 320,
        384, 448, 512, 32, 40, 48, 56, 64, 80, 96, 112, 576, 640,
        768, 960, 1152, 1344, 1536, 2304, 3072, 4608, 6144 };
    int i;
    for (i=0; i<(sizeof(valid_bitrates)/sizeof(int)); i++) {
        if (bitrate == valid_bitrates[i]){
            if (bitrate >= minimum && bitrate <= maximum) {
                return TRUE; //valid
            }
        }
    }
    return FALSE;
}
