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

#include "../build_args.h"
#include "../validate.h"

static option_t option_v0_9_9[] =
{
    { "verbose", optional_argument, k_integer, FALSE, valid_integer_set, 2, (gint[]){0, 1}},
    { "no-dvdnav", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "format", required_argument, k_string, FALSE, valid_string_set, 2,
        (gchar*[]){"av_mp4", "av_mkv"}},
    { "large-file", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL },
    { "optimize", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "ipod-atom", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "title", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "min-duration", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "scan", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "main-feature", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "chapters", required_argument, k_string, FALSE, valid_chapters, 0, NULL},
    { "angle", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "markers", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "audio", required_argument, k_string, FALSE, valid_audio, 0, NULL},
    { "mixdown", required_argument, k_string_list, FALSE, valid_string_list_set, 11,
        (gchar*[]){"mono", "left_only", "right_only", "stereo", "dpl1", "dpl2",
        "5point1", "6point1", "7point1", "5_2_lfe", "none"}},
    { "normalize-mix", required_argument, k_integer_list, FALSE, valid_integer_list_set, 2,
        (gint[]){0, 1}},
    { "drc", required_argument, k_double, FALSE, valid_drc, 0, NULL},
    { "gain", required_argument, k_double_list, FALSE, valid_gain, 0, NULL},
    { "adither", required_argument, k_string_list, FALSE, valid_dither, 6,
        (gchar*[]){"auto", "none", "rectangular", "triangular", "triangular_hp", "lipshitz_ns"}},
    { "subtitle", required_argument, k_string, FALSE, valid_subtitle, 0, NULL},
    { "subtitle-forced", optional_argument, k_string_list, FALSE, valid_subtitle_forced, 0, NULL},
    { "subtitle-burned", optional_argument, k_string, FALSE, valid_subtitle_burned, 0, NULL},
    { "subtitle-default", optional_argument, k_string, FALSE, valid_subtitle_default, 0, NULL},
    { "srt-file", required_argument, k_string_list, FALSE, valid_filename_exists_list, 0, NULL},
    { "srt-codeset", required_argument, k_string_list, FALSE, valid_codeset, 0, NULL},
    { "srt-offset", required_argument, k_integer_list, FALSE, valid_integer_list, 0, NULL},
    { "srt-lang", required_argument, k_string_list, FALSE, valid_iso_639, 0, NULL},
    { "srt-default", optional_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "native-language", required_argument, k_string, FALSE, valid_iso_639, 0, NULL},
    { "native-dub", no_argument, k_boolean, FALSE, valid_native_dub, 0, NULL},
    { "encoder", required_argument, k_string, FALSE, valid_string_set, 4,
        (gchar*[]){"x264", "ffmpeg4", "ffmpeg2", "theora"}},
    { "aencoder", required_argument, k_string_list, FALSE, valid_audio_encoder, 12,
        (gchar*[]){"av_aac", "copy:aac", "ac3", "copy:ac3", "copy:dts",
            "copy:dtshd", "mp3", "copy:mp3", "vorbis", "flac16", "flac24", "copy"}},
    { "two-pass", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "deinterlace", optional_argument, k_string, FALSE, valid_deinterlace, 0, NULL},
    { "deblock", optional_argument, k_string, FALSE, valid_deblock, 0, NULL},
    { "denoise", optional_argument, k_string, FALSE, valid_denoise, 0, NULL},
    { "detelecine", optional_argument, k_string, FALSE, valid_detelecine, 0, NULL},
    { "decomb", optional_argument, k_string, FALSE, valid_decomb, 0, NULL},
    { "grayscale", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "rotate", optional_argument, k_integer, FALSE, valid_integer_set, 7,
        (gint[]){1, 2, 3, 4, 5, 6, 7}},
    { "strict-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "loose-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "custom-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "display-width", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "keep-display-aspect", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "pixel-aspect", required_argument, k_string, FALSE, valid_pixel_aspect, 0, NULL},
    { "modulus", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "itu-par", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "width", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "height", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "crop", required_argument, k_string, FALSE, valid_crop, 0, NULL},
    { "loose-crop", optional_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "vb", required_argument, k_integer, FALSE, valid_video_bitrate, 0, NULL},
    { "quality", required_argument, k_double, FALSE, valid_video_quality, 0, NULL},
    { "ab", required_argument, k_integer_list, FALSE, valid_audio_bitrate, 0, NULL},
    { "aq", required_argument, k_double_list, FALSE, valid_audio_quality, 0, NULL},
    { "ac", required_argument, k_double_list, FALSE, valid_audio_compression, 0, NULL},
    { "rate", required_argument, k_string, FALSE, valid_string_set, 12,
        (gchar*[]){"5", "10", "12", "15", "23.976", "24", "25", "29.97", "30",
            "50", "59.94", "60"}},
    { "arate", required_argument, k_string_list, FALSE, valid_string_list_set, 10,
        (gchar*[]){"auto", "8", "11.025", "12", "16", "22.05", "24", "32", "44.1", "48"}},
    { "encopts", required_argument, k_string, FALSE, valid_encopts, 0, NULL},
    { "turbo", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "maxHeight", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "maxWidth", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "preset", required_argument, k_string, FALSE, valid_string_list, 11,
        (gchar*[]){"Universal", "iPod", "iPhone & iPod touch", "iPad",
            "AppleTV", "AppleTV 2", "AppleTV 3", "Android", "Android Tablet",
            "Normal", "High Profile"}},
    { "aname", required_argument, k_string_list, FALSE, valid_string_list, 0, NULL},
    { "color-matrix", required_argument, k_string, FALSE, valid_string_set, 4,
        (gchar*[]){"709", "pal", "ntsc", "601"}},
    { "previews", required_argument, k_string, FALSE, valid_previews, 0, NULL},
    { "start-at-preview", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "start-at", required_argument, k_string, FALSE, valid_startstop_at, 0, NULL},
    { "stop-at", required_argument, k_string, FALSE, valid_startstop_at, 0, NULL},
    { "vfr", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "cfr", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "pfr", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "audio-copy-mask", required_argument, k_string_list, FALSE, valid_string_list_set, 5,
        (gchar*[]){"copy:aac", "copy:ac3", "copy:dts", "copy:dtshd", "copy:mp3"}},
    { "audio-fallback", required_argument, k_string, FALSE, valid_string_set, 6,
        (gchar*[]){"av_aac", "ac3", "mp3", "vorbis", "flac16", "flac24"}},
    { NULL, 0, 0, 0, 0, 0}
};

static require_t require_v0_9_9[] =
{
    { "optimize", "format", "av_mp4"},
    { "ipod-atom", "format", "av_mp4"},
    { "scan", "title", NULL},
    { "srt-codeset", "srt-file", NULL},
    { "srt-offset", "srt-file", NULL},
    { "srt-lang", "srt-file", NULL},
    { "srt-default", "srt-file", NULL},
    { "native-dub", "native-language", NULL},
    { "two-pass", "vb", NULL},
    { "turbo", "two-pass", NULL},
    { "keep-display-aspect", "custom-anamorphic", NULL},
    { "pixel-aspect", "custom-anamorphic", NULL},
    { NULL, 0, 0}
};

static conflict_t conflict_v0_9_9[] =
{
    { "main-feature", "scan", NULL},
    { "audio", "aname", NULL},
    { "gain", "audio", "copy"},
    { "deinterlace", "decomb", NULL},
    { "decomb", "deinterlace", NULL},
    { "strict-anamorphic", "auto-anamorphic", NULL},
    { "strict-anamorphic", "loose-anamorphic", NULL},
    { "strict-anamorphic", "custom-anamorphic", NULL},
    { "auto-anamorphic", "strict-anamorphic", NULL},
    { "auto-anamorphic", "loose-anamorphic", NULL},
    { "auto-anamorphic", "custom-anamorphic", NULL},
    { "loose-anamorphic", "strict-anamorphic", NULL},
    { "loose-anamorphic", "auto-anamorphic", NULL},
    { "loose-anamorphic", "custom-anamorphic", NULL},
    { "custom-anamorphic", "strict-anamorphic", NULL},
    { "custom-anamorphic", "auto-anamorphic", NULL},
    { "custom-anamorphic", "loose-anamorphic", NULL},
    { "modulus", "strict-anamorphic", NULL},
    { "vb", "quality", NULL},
    { "quality", "vb", NULL},
    { "ab", "aq", NULL},
    { "aq", "ab", NULL},
    { "aname", "audio", NULL},
    { "start-at-preview", "start-at", NULL},
    { "start-at", "start-at-preview", NULL},
    { "vfr", "cfr", NULL},
    { "vfr", "pfr", NULL},
    { "cfr", "vfr", NULL},
    { "cfr", "pfr", NULL},
    { "pfr", "vfr", NULL},
    { "pfr", "cfr", NULL},
    { NULL, 0, 0}
};
