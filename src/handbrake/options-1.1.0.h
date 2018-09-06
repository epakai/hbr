static option_t v1_1_0[] =
{
    { "verbose", optional_argument, k_integer, FALSE, valid_integer_set, 2, (gint[]){0,1}},
    { "no-dvdnav", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "qsv-baseline", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "qsv-async-depth", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "qsv-implementation", required_argument, k_string, FALSE, valid_string_set, 2, (gchar*[]){"software", "hardware"}},
    { "disable-qsv-decoding", no_argument, k_boolean, FALSE, valid_qsv_decoding, 0, NULL},
    { "enable-qsv-decoding", no_argument, k_boolean, FALSE, valid_qsv_decoding, 0, NULL},
    { "format", required_argument, k_string, FALSE, valid_string_set, 2, (gchar*[]){"av_mp4", "av_mkv"}},
    { "optimize", no_argument, k_boolean, TRUE, valid_optimize, 0, NULL},
    { "ipod-atom", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "use-opencl", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "title", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "min-duration", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "scan", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "main-feature", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "chapters", required_argument, k_string, FALSE, valid_chapters, 0, NULL},
    { "angle", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "markers", optional_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "inline-parameter-sets", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "align-av", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "audio-lang-list", required_argument, k_string_list, FALSE, valid_iso639_list, 0, NULL},
    { "all-audio", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "first-audio", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "audio", required_argument, k_string, FALSE, valid_audio, 0, NULL},
    { "mixdown", required_argument, k_string_list, FALSE, valid_mixdown, 0, NULL},
    { "normalize-mix", required_argument, k_integer_list, FALSE, valid_integer_list_set, 2, (gint[]){0, 1}},
    { "drc", required_argument, k_double, FALSE, valid_drc, 0, NULL},
    { "gain", required_argument, k_double, FALSE, valid_gain, 0, NULL},
    { "adither", required_argument, k_string, FALSE, valid_dither, 0, NULL},
    { "subtitle-lang-list", required_argument, k_string_list, FALSE, valid_iso639_list, 0, NULL},
    { "all-subtitles", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "first-subtitle", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "subtitle", required_argument, k_string, FALSE, valid_subtitle, 0, NULL},
    { "subtitle-forced", optional_argument, k_string, FALSE, valid_subtitle_forced, 0, NULL},
    { "subtitle-burned", optional_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "subtitle-default", optional_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "srt-file", required_argument, k_string_list, FALSE, valid_filename_exists_list, 0, NULL},
    { "srt-codeset", required_argument, k_string_list, FALSE, valid_codeset, 0, NULL},
    { "srt-offset", required_argument, k_integer_list, FALSE, valid_integer_list, 0, NULL},
    { "srt-lang", required_argument, k_string_list, FALSE, valid_iso639, 0, NULL},
    { "srt-default", optional_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "srt-burn", optional_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "native-language", required_argument, k_string, FALSE, valid_iso639, 0, NULL},
    { "native-dub", no_argument, k_boolean, FALSE, valid_native_dub, 0, NULL},
    { "encoder", required_argument, k_string, FALSE, valid_string_set, 14, (gchar*[]){"x264", "x264_10bit", "qsv_h264", "x265", "x265_10bit", "x265_12bit", "x265_16bit", "qsv_h265", "qsv_h265_10bit", "mpeg4", "mpeg2", "VP8", "VP9","theora"}},
    { "aencoder", required_argument, k_string_list, FALSE, valid_string_list_set, 12, (gchar*[]){"av_aac", "copy:aac", "ac3", "copy:ac3", "copy:dts", "copy:dtshd", "mp3", "copy:mp3", "vorbis", "flac16", "flac24", "copy"}},
    { "two-pass", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "deinterlace", optional_argument, k_string, TRUE, valid_deinterlace, 0, NULL},
    { "deblock", optional_argument, k_string, TRUE, valid_deblock, 0, NULL},
    { "denoise", optional_argument, k_string, FALSE, valid_denoise, 0, NULL},
    { "hqdn3d", optional_argument, k_string, TRUE, valid_denoise, 0, NULL},
    { "nlmeans", optional_argument, k_string, TRUE, valid_nlmeans, 0, NULL},
    { "nlmeans-tune", required_argument, k_string, FALSE, valid_string_set, 7, (gchar*[]){"none", "film", "grain", "highmotion", "animation", "tape", "sprite"}},
    { "unsharp", optional_argument, k_string, TRUE, valid_unsharp, 0, NULL},
    { "unsharp-tune", required_argument, k_string, FALSE, valid_string_set, 6, (gchar*[]){"none", "ultrafine", "fine", "medium", "coarse", "verycoarse"}},
    { "lapsharp", optional_argument, k_string, TRUE, valid_string_set, 0, NULL},
    { "lapsharp-tune", required_argument, k_string, FALSE, valid_string_set, 5, (gchar*[]){"none", "film", "grain", "animation", "sprite"}},
    { "detelecine", optional_argument, k_string, TRUE, valid_detelecine, 0, NULL},
    { "comb-detect", optional_argument, k_string, TRUE, valid_comb_detect, 0, NULL},
    { "decomb", optional_argument, k_string, TRUE, valid_decomb, 0, NULL},
    { "grayscale", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "rotate", optional_argument, k_string, FALSE, valid_rotate, 0, NULL},
    { "non-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "auto-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "loose-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "custom-anamorphic", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "display-width", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "keep-display-aspect", optional_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "pixel-aspect", required_argument, k_string, FALSE, valid_pixel_aspect, 0, NULL},
    { "modulus", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "itu-par", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "width", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "height", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "crop", required_argument, k_string, FALSE, valid_crop, 0, NULL},
    { "loose-crop", optional_argument, k_integer, TRUE, valid_positive_integer, 0, NULL},
    { "pad", required_argument, k_string, TRUE, valid_pad, 0, NULL},
    { "encoder-preset", required_argument, k_string, FALSE, valid_encoder_preset, 0, NULL},
    { "encoder-tune", required_argument, k_string, FALSE, valid_encoder_tune, 0, NULL},
    { "encopts", required_argument, k_string, FALSE, valid_encopts, 0, NULL},
    { "encoder-profile", required_argument, k_string, FALSE, valid_encoder_profile, 0, NULL},
    { "encoder-level", required_argument, k_string, FALSE, valid_encoder_level, 0, NULL},
    { "vb", required_argument, k_integer, FALSE, valid_video_bitrate, 0, NULL},
    { "quality", required_argument, k_integer, FALSE, valid_video_quality, 0, NULL},
    { "ab", required_argument, k_double_list, FALSE, valid_audio_bitrate, 0, NULL},
    { "aq", required_argument, k_double_list, FALSE, valid_audio_quality, 0, NULL},
    { "ac", required_argument, k_double_list, FALSE, valid_positive_double_list, 0, NULL},
    { "rate", required_argument, k_string, FALSE, valid_string_set, 12, (gchar*[]){"5", "10", "12", "15", "23.976", "24", "25", "29.97", "30", "50", "59.94", "60"}},
    { "arate", required_argument, k_string_list, FALSE, valid_string_list_set, 9, (gchar*[]){"8", "11.025", "12", "16", "22.05", "24", "32", "44.1", "48"}},
    { "turbo", no_argument, k_boolean, TRUE, valid_boolean, 0, NULL},
    { "maxHeight", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "maxWidth", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "preset", required_argument, k_string, FALSE, valid_string_list, 12, (gchar*[]){"Universal", "iPod", "iPhone & iPod touch", "iPad", "AppleTV", "AppleTV 2", "AppleTV 3", "Android", "Android Tablet", "Windows Phone 8", "Normal", "High Profile"}},
    { "preset-import-file", required_argument, k_string, FALSE, valid_filespec, 0, NULL},
    { "preset-import-gui", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "preset-export", required_argument, k_string, FALSE, valid_preset_name, 0, NULL},
    { "preset-export-file", required_argument, k_string, FALSE, valid_filename_dne, 0, NULL},
    { "preset-export-description", required_argument, k_string, FALSE, valid_string, 0, NULL},
    { "queue-import-file", required_argument, },
    { "aname", required_argument, k_string_list, FALSE, valid_string_list, 0, NULL},
    { "color-matrix", required_argument, k_string, FALSE, valid_string_set, 4, (gchar*[]){"709", "pal", "ntsc", "601"}},
    { "previews", required_argument, k_string, FALSE, valid_previews, 0, NULL},
    { "start-at-preview", required_argument, k_integer, FALSE, valid_positive_integer, 0, NULL},
    { "start-at", required_argument, k_string, FALSE, valid_startstop_at, 0, NULL},
    { "stop-at", required_argument, k_string, FALSE, valid_startstop_at, 0, NULL},
    { "vfr", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "cfr", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "pfr", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    { "audio-copy-mask", required_argument, k_string_list, FALSE, valid_string_list_set, 6, (gchar*[]){"all", "aac", "ac3", "dts", "dtshd", "mp3"}},
    { "audio-fallback", required_argument, k_string, FALSE, valid_string_set, 6, (gchar*[]){"av_aac", "ac3", "mp3", "vorbis", "flac16", "flac24"}},
    { "json", no_argument, k_boolean, FALSE, valid_boolean, 0, NULL},
    {0, 0, 0, 0, 0, 0}
};
