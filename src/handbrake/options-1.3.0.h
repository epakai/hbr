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

#include "../options.h"
#include "../validate.h"

static option_t option_v1_3_0[] =
{
    { "verbose", optional_argument, k_integer, FALSE, valid_integer_set, 2, (gint[]){0, 1}},
    { "no-dvdnav", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "qsv-baseline", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "qsv-async-depth", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "qsv-implementation", required_argument, k_string, FALSE, valid_string_set, 2,
        (const gchar*[]){"software", "hardware"}},
    { "disable-qsv-decoding", no_argument, k_boolean, FALSE, valid_qsv_decoding, 0, NULL},
    { "enable-qsv-decoding", no_argument, k_boolean, FALSE, valid_qsv_decoding, 0, NULL},
    { "format", required_argument, k_string, FALSE, valid_string_set, 2,
        (const gchar*[]){"av_mp4", "av_mkv"}},
    { "optimize", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "ipod-atom", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "use-opencl", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "title", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "min-duration", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "scan", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "main-feature", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "chapters", required_argument, k_string, FALSE, valid_chapters, 0, NULL},
    { "angle", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "markers", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "inline-parameter-sets", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "align-av", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "audio-lang-list", required_argument, k_string_list, FALSE, valid_iso_639_list, 0, NULL},
    { "all-audio", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "first-audio", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "audio", required_argument, k_string, FALSE, valid_audio, 0, NULL},
    { "mixdown", required_argument, k_string_list, FALSE, valid_string_list_set, 11,
        (const gchar*[]){"mono", "left_only", "right_only", "stereo", "dpl1", "dpl2",
        "5point1", "6point1", "7point1", "5_2_lfe", "none"}},
    { "normalize-mix", required_argument, k_integer_list, FALSE, valid_integer_list_set, 2,
        (gint[]){0, 1}},
    { "drc", required_argument, k_double, FALSE, valid_drc, 0, NULL},
    { "gain", required_argument, k_double_list, FALSE, valid_gain, 0, NULL},
    { "adither", required_argument, k_string_list, FALSE, valid_dither, 6,
        (const gchar*[]){"auto", "none", "rectangular", "triangular",
            "triangular_hp", "lipshitz_ns"}},
    { "subtitle-lang-list", required_argument, k_string_list, FALSE, valid_iso_639_list, 0, NULL},
    { "all-subtitles", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "first-subtitle", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "subtitle", required_argument, k_string, FALSE, valid_subtitle, 0, NULL},
    { "subtitle-forced", optional_argument, k_string_list, FALSE, valid_subtitle_forced, 0, NULL},
    { "subtitle-burned", optional_argument, k_string, FALSE, valid_subtitle_burned, 0, NULL},
    { "subtitle-default", optional_argument, k_string, FALSE, valid_subtitle_default, 0, NULL},
    { "subname", required_argument, k_string_list, FALSE, valid_string_list, 0, NULL},
    { "srt-file", required_argument, k_path_list, FALSE, valid_filename_exists_list, 0, NULL},
    { "srt-codeset", required_argument, k_string_list, FALSE, valid_codeset, 0, NULL},
    { "srt-offset", required_argument, k_integer_list, FALSE, valid_integer_list, 0, NULL},
    { "srt-lang", required_argument, k_string_list, FALSE, valid_iso_639, 0, NULL},
    { "srt-default", optional_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "srt-burn", optional_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "ssa-file", required_argument, k_string_list, FALSE, valid_filename_exists_list, 0, NULL},
    { "ssa-offset", required_argument, k_integer_list, FALSE, valid_integer_list, 0, NULL},
    { "ssa-lang", required_argument, k_string_list, FALSE, valid_iso_639, 0, NULL},
    { "ssa-default", optional_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "ssa-burn", optional_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "native-language", required_argument, k_string, FALSE, valid_iso_639, 0, NULL},
    { "native-dub", no_argument, k_boolean, FALSE, valid_native_dub, 0, NULL},
    { "encoder", required_argument, k_string, FALSE, valid_string_set, 14,
        (const gchar*[]){"x264", "x264_10bit", "qsv_h264", "x265", "x265_10bit",
            "x265_12bit", "x265_16bit", "qsv_h265", "qsv_h265_10bit", "mpeg4",
            "mpeg2", "VP8", "VP9", "theora"}},
    { "aencoder", required_argument, k_string_list, FALSE, valid_audio_encoder, 13,
        (const gchar*[]){"av_aac", "copy:aac", "ac3", "copy:ac3", "copy:dts",
            "copy:dtshd", "mp3", "copy:mp3", "vorbis", "flac16", "flac24",
            "opus", "copy"}},
    { "two-pass", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "deinterlace", optional_argument, k_string, TRUE, valid_deinterlace, 7,
        (const gchar*[]){"default", "skip-spatial", "bob", "qsv", "fast",
            "slow", "slower"}},
    { "deblock", optional_argument, k_string, TRUE, valid_deblock, 6,
        (const gchar*[]) {"ultralight", "light", "medium", "strong", "stronger",
            "verystrong"}},
    { "deblock-tune", required_argument, k_string_list, FALSE, valid_string_set, 3,
        (const gchar*[]) {"small", "medium", "large"}},
    { "denoise", optional_argument, k_string, FALSE, valid_denoise, 0, NULL},
    { "hqdn3d", optional_argument, k_string, TRUE, valid_denoise, 0, NULL},
    { "nlmeans", optional_argument, k_string, TRUE, valid_nlmeans, 0, NULL},
    { "nlmeans-tune", required_argument, k_string, FALSE, valid_string_set, 7,
        (const gchar*[]){"none", "film", "grain", "highmotion", "animation", "tape",
            "sprite"}},
    { "chroma-smooth", optional_argument, k_string, TRUE, valid_chroma, 6,
        (const gchar*[]){"ultralight", "light", "medium", "strong", "stronger",
            "verystrong"}},
    { "chrome-smooth-tune", required_argument, k_string, FALSE, valid_string_set, 6,
        (const gchar*[]) {"none", "tiny", "small", "medium", "wide", "verywide"}},
    { "unsharp", optional_argument, k_string, TRUE, valid_unsharp, 0, NULL},
    { "unsharp-tune", required_argument, k_string, FALSE, valid_string_set, 6,
        (const gchar*[]){"none", "ultrafine", "fine", "medium", "coarse", "verycoarse"}},
    { "lapsharp", optional_argument, k_string, TRUE, valid_string_set, 0, NULL},
    { "lapsharp-tune", required_argument, k_string, FALSE, valid_string_set, 5,
        (const gchar*[]){"none", "film", "grain", "animation", "sprite"}},
    { "detelecine", optional_argument, k_string, TRUE, valid_detelecine, 0, NULL},
    { "comb-detect", optional_argument, k_string, TRUE, valid_comb_detect, 4,
        (const gchar*[]){"permissive", "fast", "default", "off"}},
    { "decomb", optional_argument, k_string, TRUE, valid_decomb, 3,
        (const gchar*[]){"bob", "eedi2", "eedi2bob"}},
    { "grayscale", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "rotate", optional_argument, k_string, FALSE, valid_rotate, 0, NULL},
    { "non-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "auto-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "loose-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "custom-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "display-width", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "keep-display-aspect", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "pixel-aspect", required_argument, k_string, FALSE, valid_pixel_aspect, 0, NULL},
    { "modulus", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "itu-par", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "width", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "height", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "crop", required_argument, k_string, FALSE, valid_crop, 0, NULL},
    { "loose-crop", no_argument, k_integer, TRUE, valid_positive_integer, 0, NULL},
    { "pad", required_argument, k_string, TRUE, valid_pad, 0, NULL},
    { "encoder-preset", required_argument, k_string, FALSE, valid_encoder_preset, 0, NULL},
    { "encoder-tune", required_argument, k_string, FALSE, valid_encoder_tune, 0, NULL},
    { "encopts", required_argument, k_string, FALSE, valid_encopts, 0, NULL},
    { "encoder-profile", required_argument, k_string, FALSE, valid_encoder_profile, 0, NULL},
    { "encoder-level", required_argument, k_string, FALSE, valid_encoder_level, 0, NULL},
    { "vb", required_argument, k_integer, FALSE, valid_video_bitrate, 0, NULL},
    { "quality", required_argument, k_double, FALSE, valid_video_quality, 0, NULL},
    { "ab", required_argument, k_integer_list, FALSE, valid_audio_bitrate, 0, NULL},
    { "aq", required_argument, k_double_list, FALSE, valid_audio_quality, 0, NULL},
    { "ac", required_argument, k_double_list, FALSE, valid_audio_compression, 0, NULL},
    { "rate", required_argument, k_string, FALSE, valid_video_framerate, 19,
        (const gchar*[]){"5", "10", "12", "15", "20", "23.976", "24", "25", "29.97",
            "30", "48", "50", "59.94", "60", "72", "75", "90", "100", "120"}},
    { "arate", required_argument, k_string_list, FALSE, valid_string_list_set, 10,
        (const gchar*[]){"auto", "8", "11.025", "12", "16", "22.05", "24", "32", "44.1", "48"}},
    { "turbo", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "maxHeight", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "maxWidth", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "preset", required_argument, k_string, FALSE, valid_string_list, 12,
        (const gchar*[]){"Universal", "iPod", "iPhone & iPod touch", "iPad",
            "AppleTV", "AppleTV 2", "AppleTV 3", "Android", "Android Tablet",
            "Windows Phone 8", "Normal", "High Profile"}},
    { "preset-import-file", required_argument, k_string, FALSE, valid_filespec, 0, NULL},
    { "preset-import-gui", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "preset-export", required_argument, k_string, FALSE, valid_preset_name, 0, NULL},
    { "preset-export-file", required_argument, k_string, FALSE, valid_filename_dne, 0, NULL},
    { "preset-export-description", required_argument, k_string, FALSE, valid_string, 0, NULL},
    { "queue-import-file", required_argument, k_string, FALSE, valid_filename_component, 0, NULL},
    { "aname", required_argument, k_string_list, FALSE, valid_string_list, 0, NULL},
    { "color-matrix", required_argument, k_string, FALSE, valid_string_set, 4,
        (const gchar*[]){"709", "pal", "ntsc", "601"}},
    { "previews", required_argument, k_string, FALSE, valid_previews, 0, NULL},
    { "start-at-preview", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "start-at", required_argument, k_string, FALSE, valid_startstop_at, 0, NULL},
    { "stop-at", required_argument, k_string, FALSE, valid_startstop_at, 0, NULL},
    { "vfr", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "cfr", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "pfr", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "audio-copy-mask", required_argument, k_string_list, FALSE, valid_string_list_set, 8,
        (const gchar*[]){"copy:aac", "copy:ac3", "copy:eac3", "copy:truehd",
            "copy:dts", "copy:dtshd", "copy:mp3", "copy:flac"}},
    { "audio-fallback", required_argument, k_string, FALSE, valid_string_set, 7,
        (const gchar*[]){"av_aac", "ac3", "mp3", "vorbis", "flac16", "flac24", "opus"}},
    { "json", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { NULL, 0, 0, 0, NULL, 0, NULL}
};

static custom_t custom_v1_3_0[] =
{
    { "comb-detect", (const custom_key_t []) {
                                                 { k_integer, "mode"},
                                                 { k_integer, "spatial-metric"},
                                                 { k_integer, "motion-thresh"},
                                                 { k_integer, "spatial-thresh"},
                                                 { k_integer, "filter-mode"},
                                                 { k_integer, "block-thresh"},
                                                 { k_integer, "block-width"},
                                                 { k_integer, "block-height"},
                                                 { k_boolean, "disable"},
                                                 { k_boolean, NULL} }},
    { "deblock", (const custom_key_t []) {
                                             { k_string, "strength"},
                                             { k_integer, "thresh"},
                                             { k_integer, "blocksize"},
                                             { k_boolean, "disable"},
                                             { k_boolean, NULL} }},
    { "decomb",  (const custom_key_t []) {
                                             { k_integer, "mode"},
                                             { k_integer, "magnitude-thresh"},
                                             { k_integer, "variance-thresh"},
                                             { k_integer, "laplacian-thresh"},
                                             { k_integer, "dilation-thresh"},
                                             { k_integer, "erosion-thresh"},
                                             { k_integer, "noise-thresh"},
                                             { k_integer, "search-distance"},
                                             { k_integer, "postproc"},
                                             { k_integer, "parity"},
                                             { k_boolean, NULL} }},
    { "deinterlace", (const custom_key_t []) {
                                                 { k_integer, "mode"},
                                                 { k_integer, "parity"},
                                                 { k_boolean, NULL} }},

    { NULL, NULL }
};

static require_t require_v1_3_0[] =
{
    { "qsv-async-depth", "enable-qsv-decoding", NULL},
    { "qsv-baseline", "enable-qsv-decoding", NULL},
    { "qsv-implementation", "enable-qsv-decoding", NULL},
    { "disable-qsv-decoding", "enable-qsv-decoding", NULL},
    { "optimize", "format", "av_mp4"},
    { "ipod-atom", "format", "av_mp4"},
    { "scan", "title", NULL},
    { "srt-codeset", "srt-file", NULL},
    { "srt-offset", "srt-file", NULL},
    { "srt-lang", "srt-file", NULL},
    { "srt-default", "srt-file", NULL},
    { "srt-burn", "srt-file", NULL},
    { "ssa-offset", "ssa-file", NULL},
    { "ssa-lang", "ssa-file", NULL},
    { "ssa-default", "ssa-file", NULL},
    { "ssa-burn", "ssa-file", NULL},
    { "native-dub", "native-language", NULL},
    { "two-pass", "vb", NULL},
    { "turbo", "two-pass", NULL},
    { "nlmeans-tune", "nlmeans", NULL},
    { "unsharp-tune", "unsharp", NULL},
    { "lapsharp-tune", "lapsharp", NULL},
    { "keep-display-aspect", "custom-anamorphic", NULL},
    { "pixel-aspect", "custom-anamorphic", NULL},
    { "deblock-tune", "deblock", NULL},
    { "chrome-smooth-tune", "chrome-smooth", NULL},
    { NULL, NULL, NULL}
};

static conflict_t conflict_v1_3_0[] =
{
    { "enable-qsv-decoding", NULL, "disable-qsv-decoding", NULL},
    { "main-feature", NULL, "scan", NULL},
    { "all-audio", NULL, "audio", NULL},
    { "all-audio", NULL, "aname", NULL},
    { "first-audio", NULL, "audio", NULL},
    { "first-audio", NULL, "aname", NULL},
    { "first-audio", NULL, "all-audio", NULL},
    { "audio", NULL, "all-audio", NULL },
    { "audio", NULL, "first-audio", NULL},
    { "audio", NULL, "aname", NULL},
    { "gain", NULL, "audio", "copy"},
    { "subtitle-lang-list", NULL, "subtitle", NULL},
    { "all-subtitles", NULL, "subtitle", NULL},
    { "all-subtitles", NULL, "first-subtitle", NULL},
    { "first-subtitle", NULL, "subtitle", NULL},
    { "first-subtitle", NULL, "all-subtitles", NULL},
    { "subtitle", NULL, "all-subtitles", NULL},
    { "subtitle", NULL, "first-subtitles", NULL},
    { "deinterlace", NULL, "decomb", NULL},
    { "decomb", NULL, "deinterlace", NULL},
    { "non-anamorphic", NULL, "auto-anamorphic", NULL},
    { "non-anamorphic", NULL, "loose-anamorphic", NULL},
    { "non-anamorphic", NULL, "custom-anamorphic", NULL},
    { "auto-anamorphic", NULL, "non-anamorphic", NULL},
    { "auto-anamorphic", NULL, "loose-anamorphic", NULL},
    { "auto-anamorphic", NULL, "custom-anamorphic", NULL},
    { "loose-anamorphic", NULL, "non-anamorphic", NULL},
    { "loose-anamorphic", NULL, "auto-anamorphic", NULL},
    { "loose-anamorphic", NULL, "custom-anamorphic", NULL},
    { "custom-anamorphic", NULL, "non-anamorphic", NULL},
    { "custom-anamorphic", NULL, "auto-anamorphic", NULL},
    { "custom-anamorphic", NULL, "loose-anamorphic", NULL},
    { "vb", NULL, "quality", NULL},
    { "quality", NULL, "vb", NULL},
    { "ab", NULL, "aq", NULL},
    { "aq", NULL, "ab", NULL},
    { "aname", NULL, "audio", NULL},
    { "start-at-preview", NULL, "start-at", NULL},
    { "start-at", NULL, "start-at-preview", NULL},
    { "vfr", NULL, "cfr", NULL},
    { "vfr", NULL, "pfr", NULL},
    { "cfr", NULL, "vfr", NULL},
    { "cfr", NULL, "pfr", NULL},
    { "pfr", NULL, "vfr", NULL},
    { "pfr", NULL, "cfr", NULL},
    { NULL, NULL, NULL, NULL}
};
