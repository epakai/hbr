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

#include "keyfile.h"
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gprintf.h>


/**
 * @brief Parses the key value file.
 *
 * @param infile path for key value file.
 *
 * @return GKeyfile pointer. NULL on failure. Must be freed by caller.
 */
GKeyFile * parse_key_file(char *infile)
{
    GError *error = NULL;
    GKeyFile *keyfile = g_key_file_new();
    if (!g_key_file_load_from_file (keyfile, infile,
                G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            fprintf(stderr, "Error loading file (%s): %s\n", infile, error->message);
        } else if (!g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_PARSE)) {
            fprintf(stderr, "Error parsing file (%s): %s\n", infile, error->message);
        } else if (!g_error_matches (error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_UNKNOWN_ENCODING)) {
            fprintf(stderr, "File has unknown encoding (%s): %s\n", infile, error->message);
        }
        g_error_free(error);
        g_key_file_free(keyfile);
        return NULL;
    }
    g_key_file_set_list_separator(keyfile, ',');
    return keyfile;
}

/**
 * @brief Generate a config file including comments for all keys
 *
 * @param c
 *
 * @return
 */
GKeyFile * generate_key_file(struct config c)
{
    GKeyFile *k = g_key_file_new();
    g_key_file_set_list_separator(k, ',');
    const gchar *g = "CONFIG";
    g_key_file_set_comment(k, NULL, NULL,
            " hbr (handbrake runner) config file\n"
            " Options mostly follow the naming from HandBrakeCLI --help\n"
            " See comments above each key for type and acceptable values\n"
            " [] indicates type, <> for potential values, {} for defaults", NULL);

    if (c.set.format) {
        g_key_file_set_string(k, g, FORMAT, c.key.format);
    } else {
        g_key_file_set_string(k, g, FORMAT, "");
    }
    g_key_file_set_comment(k, g, FORMAT,
            " Output container format\n"
            " [string] <{mkv},m4v>", NULL);

    if (c.set.markers) {
        g_key_file_set_boolean(k, g, MARKERS, c.key.markers);
    } else {
        g_key_file_set_string(k, g, MARKERS, "");
    }
    g_key_file_set_comment(k, g, MARKERS,
            " Add chapter markers\n"
            " [boolean] <true, {false}>", NULL);

    if (c.set.picture_anamorphic) {
        g_key_file_set_string(k, g, PICTURE_ANAMORPHIC, c.key.picture_anamorphic);
    } else {
        g_key_file_set_string(k, g, PICTURE_ANAMORPHIC, "");
    }
    g_key_file_set_comment(k, g, PICTURE_ANAMORPHIC,
            " How to store pixel aspect ratio\n"
            " [string] <{off},strict,loose>", NULL);

    if (c.set.picture_autocrop) {
        g_key_file_set_boolean(k, g, PICTURE_AUTOCROP, c.key.picture_autocrop);
    } else {
        g_key_file_set_string(k, g, PICTURE_AUTOCROP, "");
    }
    g_key_file_set_comment(k, g, PICTURE_AUTOCROP,
            " Determine crop values automatically\n"
            " [boolean] <true, {false}>", NULL);

    if (c.set.picture_loose_crop) {
        g_key_file_set_integer(k, g, PICTURE_LOOSE_CROP, c.key.picture_loose_crop);
    } else {
        g_key_file_set_string(k, g, PICTURE_LOOSE_CROP, "");
    }
    g_key_file_set_comment(k, g, PICTURE_LOOSE_CROP,
            " Max extra pixels to be cropped when rounding pixel dimensions\n"
            " [int] <...,{15},...>", NULL);

    if (c.set.picture_crop_top) {
        g_key_file_set_integer(k, g, PICTURE_CROP_TOP, c.key.picture_crop_top);
    } else {
        g_key_file_set_string(k, g, PICTURE_CROP_TOP, "");
    }
    g_key_file_set_comment(k, g, PICTURE_CROP_TOP,
            " Crop values for each edge\n"
            " [int] <{0},...>", NULL);
    if (c.set.picture_crop_bottom) {
        g_key_file_set_integer(k, g, PICTURE_CROP_BOTTOM, c.key.picture_crop_bottom);
    } else {
        g_key_file_set_string(k, g, PICTURE_CROP_BOTTOM, "");
    }
    if (c.set.picture_crop_left) {
        g_key_file_set_integer(k, g, PICTURE_CROP_LEFT, c.key.picture_crop_left);
    } else {
        g_key_file_set_string(k, g, PICTURE_CROP_LEFT, "");
    }
    if (c.set.picture_crop_right) {
        g_key_file_set_integer(k, g, PICTURE_CROP_RIGHT, c.key.picture_crop_right);
    } else {
        g_key_file_set_string(k, g, PICTURE_CROP_RIGHT, "");
    }

    if (c.set.filter_deinterlace) {
        g_key_file_set_string(k, g, FILTER_DEINTERLACE, c.key.filter_deinterlace);
    } else {
        g_key_file_set_string(k, g, FILTER_DEINTERLACE, "");
    }
    g_key_file_set_comment(k, g, FILTER_DEINTERLACE,
            " Deinterlace all frames\n"
            " [string] <{none},default,fast,slow,slower,bob>", NULL);

    if (c.set.filter_decomb) {
        g_key_file_set_string(k, g, FILTER_DECOMB, c.key.filter_decomb);
    } else {
        g_key_file_set_string(k, g, FILTER_DECOMB, "");
    }
    g_key_file_set_comment(k, g, FILTER_DECOMB,
            " Deinterlace frames when combing is detected\n"
            " [string] <{none},default,fast,bob>", NULL);

    if (c.set.filter_denoise) {
        g_key_file_set_string(k, g, FILTER_DENOISE, c.key.filter_denoise);
    } else {
        g_key_file_set_string(k, g, FILTER_DENOISE, "");
    }
    g_key_file_set_comment(k, g, FILTER_DENOISE,
            " Denoise video with hqdn3d filter\n"
            " [string] <{none},default,ultralight,light,medium,strong>", NULL);

    if (c.set.filter_grayscale) {
        g_key_file_set_boolean(k, g, FILTER_GRAYSCALE, c.key.filter_grayscale);
    } else {
        g_key_file_set_string(k, g, FILTER_GRAYSCALE, "");
    }
    g_key_file_set_comment(k, g, FILTER_GRAYSCALE,
            " Grayscale encoding\n"
            " [boolean] <true, {false}>", NULL);

    int count = 0;
    while (c.key.filter_rotate[count] != NULL) {
        count++;
    }
    if (c.set.filter_rotate) {
        g_key_file_set_string_list(k, g, FILTER_ROTATE, (const gchar * const *)
                c.key.filter_rotate, count);
    } else {
        g_key_file_set_string(k, g, FILTER_ROTATE, "");
    }
    g_key_file_set_comment(k, g, FILTER_ROTATE,
            " Rotate image. Combine operations by listing them comma-separated\n"
            " [string_list] <{none},default,vertical_flip,horizontal_flip,rotate_clockwise>", NULL);

    if (c.set.audio_encoder) {
        g_key_file_set_string(k, g, AUDIO_ENCODER, c.key.audio_encoder);
    } else {
        g_key_file_set_string(k, g, AUDIO_ENCODER, "");
    }
    g_key_file_set_comment(k, g, AUDIO_ENCODER,
            " Audio encoder, default depends on format(mp4=av_aac, mkv=mp3). "
            "fdk codecs are not compiled in by default\n"
            " [string] <{av_aac},{mp3},fdk_aac,fdk_haac,copy:aac,ac3,copy:ac3,"
            "copy:dts,copy:dtshd,copy:mp3,vorbis,flac16,flac24,copy>", NULL);

    if (c.set.audio_bitrate) {
        g_key_file_set_integer(k, g, AUDIO_BITRATE, c.key.audio_bitrate);
    } else {
        g_key_file_set_string(k, g, AUDIO_BITRATE, "");
    }
    g_key_file_set_comment(k, g, AUDIO_BITRATE,
            " Audio bitrate, Defaults and available bitrates depend on "
            "audio_encoder with lossless encoder having none\n"
            " [int] <32,40,48,56,64,80,96,112,128,160,192,224,256,320,384,448,"
            "512,576,640,768,960,1152,1344,1536,2304,3072,4608,6144>", NULL);

    if (c.set.audio_quality) {
        g_key_file_set_double(k, g, AUDIO_QUALITY, c.key.audio_quality);
    } else {
        g_key_file_set_string(k, g, AUDIO_QUALITY, "");
    }
    g_key_file_set_comment(k, g, AUDIO_QUALITY,
            " Audio quality, only available for some codecs, range varies by codec\n"
            " [double]", NULL);

    if (c.set.video_encoder) {
        g_key_file_set_string(k, g, VIDEO_ENCODER, c.key.video_encoder);
    } else {
        g_key_file_set_string(k, g, VIDEO_ENCODER, "");
    }
    g_key_file_set_comment(k, g, VIDEO_ENCODER,
            " Video encoder\n"
            " [string] <{x264},x265,mpeg4,mpeg2,VP8,theora>", NULL);

    if (c.set.video_framerate) {
        g_key_file_set_string(k, g, VIDEO_FRAMERATE, c.key.video_framerate);
    } else {
        g_key_file_set_string(k, g, VIDEO_FRAMERATE, "");
    }
    g_key_file_set_comment(k, g, VIDEO_FRAMERATE,
            " Specify encoded video framerate\n"
            " [string] <{source},5,10,12,15,23.976,24,25,29.97,30,50,59.94,60>", NULL);

    if (c.set.video_framerate_control) {
        g_key_file_set_string(k, g, VIDEO_FRAMERATE_CONTROL,
                c.key.video_framerate_control);
    } else {
        g_key_file_set_string(k, g, VIDEO_FRAMERATE_CONTROL, "");
    }
    g_key_file_set_comment(k, g, VIDEO_FRAMERATE_CONTROL,
            " Control how framerate limit is applied\n"
            " [string] <constant, {variable}, peak>", NULL);

    if (c.set.video_quality) {
        g_key_file_set_integer(k, g, VIDEO_QUALITY, c.key.video_quality);
    } else {
        g_key_file_set_string(k, g, VIDEO_QUALITY, "");
    }
    g_key_file_set_comment(k, g, VIDEO_QUALITY,
            " Video quality, defaults and range depend on video_encoder\n"
            " [int]", NULL);

    if (c.set.video_bitrate) {
        g_key_file_set_integer(k, g, VIDEO_BITRATE, c.key.video_bitrate);
    } else {
        g_key_file_set_string(k, g, VIDEO_BITRATE, "");
    }
    g_key_file_set_comment(k, g, VIDEO_BITRATE,
            " Video bitrate, defaults and range depend on video_encoder\n"
            " [int]", NULL);

    if (c.set.video_two_pass) {
        g_key_file_set_boolean(k, g, VIDEO_TWO_PASS, c.key.video_two_pass);
    } else {
        g_key_file_set_string(k, g, VIDEO_TWO_PASS, "");
    }
    g_key_file_set_comment(k, g, VIDEO_TWO_PASS,
            " Two-pass mode, only applies when video_bitrate is used\n"
            " [boolean] <true, {false}>", NULL);

    if (c.set.video_turbo) {
        g_key_file_set_boolean(k, g, VIDEO_TURBO, c.key.video_turbo);
    } else {
        g_key_file_set_string(k, g, VIDEO_TURBO, "");
    }
    g_key_file_set_comment(k, g, VIDEO_TURBO,
            " Turbo first pass, only applies when two-pass mode is used\n"
            " [boolean] <true, {false}>", NULL);
    return k;
}

/**
 * @brief
 *
 * @param keyfile
 *
 * @return
 */
struct config get_local_config(GKeyFile* keyfile)
{
    const gchar group[] = "CONFIG";
    //verify CONFIG group exists in file
    if (!g_key_file_has_group(keyfile, group))
    {
        return empty_config();
    }
    struct config c = get_global_config(keyfile);

    c.key.input_basedir = get_key_value_string(keyfile, group,
            INPUT_BASEDIR, &c.set.input_basedir);
    c.key.output_basedir = get_key_value_string(keyfile, group,
            OUTPUT_BASEDIR, &c.set.output_basedir);
    return c;
}


/**
 * @brief
 *
 * @param keyfile
 *
 * @return
 */
struct config get_global_config(GKeyFile* keyfile)
{
    const gchar group[] = "CONFIG";
    struct config c = empty_config();

    //verify CONFIG group exists in file
    if (!g_key_file_has_group(keyfile, group))
    {
        return c;
    }
    c.key.format = get_key_value_string(keyfile, group,
            FORMAT, &c.set.format);
    c.key.markers = get_key_value_boolean(keyfile, group,
            MARKERS, &c.set.markers);

    c.key.picture_anamorphic = get_key_value_string(keyfile, group,
            PICTURE_ANAMORPHIC, &c.set.picture_anamorphic);
    c.key.picture_autocrop = get_key_value_boolean(keyfile, group,
            PICTURE_AUTOCROP, &c.set.picture_autocrop);
    // TODO loose crop defaults to 15 if set, but no value provided
    c.key.picture_loose_crop = get_key_value_integer(keyfile, group,
            PICTURE_LOOSE_CROP, &c.set.picture_loose_crop);
    c.key.picture_crop_top = get_key_value_integer(keyfile, group,
            PICTURE_CROP_TOP, &c.set.picture_crop_top);
    c.key.picture_crop_bottom = get_key_value_integer(keyfile, group,
            PICTURE_CROP_BOTTOM, &c.set.picture_crop_bottom);
    c.key.picture_crop_left = get_key_value_integer(keyfile, group,
            PICTURE_CROP_LEFT, &c.set.picture_crop_left);
    c.key.picture_crop_right = get_key_value_integer(keyfile, group,
            PICTURE_CROP_RIGHT, &c.set.picture_crop_right);

    c.key.filter_decomb = get_key_value_string(keyfile, group,
            FILTER_DECOMB, &c.set.filter_decomb);
    c.key.filter_deinterlace = get_key_value_string(keyfile, group,
            FILTER_DEINTERLACE, &c.set.filter_deinterlace);
    c.key.filter_denoise = get_key_value_string(keyfile, group,
            FILTER_DENOISE, &c.set.filter_denoise);
    c.key.filter_grayscale = get_key_value_boolean(keyfile, group,
            FILTER_GRAYSCALE, &c.set.filter_grayscale);
    c.key.filter_rotate = get_key_value_string_list(keyfile, group,
            FILTER_ROTATE, &c.key.filter_rotate_count, &c.set.filter_rotate);

    c.key.audio_encoder = get_key_value_string(keyfile, group,
            AUDIO_ENCODER, &c.set.audio_encoder);
    c.key.audio_quality = get_key_value_integer(keyfile, group,
            AUDIO_QUALITY, &c.set.audio_quality );
    c.key.audio_bitrate = get_key_value_integer(keyfile, group,
            AUDIO_BITRATE, &c.set.audio_bitrate );

    c.key.video_encoder = get_key_value_string(keyfile, group,
            VIDEO_ENCODER, &c.set.video_encoder);
    c.key.video_quality = get_key_value_integer(keyfile, group,
            VIDEO_QUALITY, &c.set.video_quality );
    c.key.video_bitrate = get_key_value_integer(keyfile, group,
            VIDEO_BITRATE, &c.set.video_bitrate );
    c.key.video_framerate = get_key_value_string(keyfile, group,
            VIDEO_FRAMERATE, &c.set.video_framerate);
    c.key.video_framerate_control = get_key_value_string(keyfile, group,
            VIDEO_FRAMERATE_CONTROL, &c.set.video_framerate_control);
    c.key.video_turbo = get_key_value_boolean(keyfile, group,
            VIDEO_TURBO, &c.set.video_turbo);
    c.key.video_two_pass = get_key_value_boolean(keyfile, group,
            VIDEO_TWO_PASS, &c.set.video_two_pass);
    return c;
}


/**
 * @brief Combine two potentially incomplete configs
 *
 * @param pref config with preferred values
 * @param alt config with alternative values
 *
 * @return merged config
 */
struct config merge_configs(struct config pref, struct config alt)
{
    struct config merged = empty_config();

    merged.key.format = merge_string_key(pref.key.format, pref.set.format,
            alt.key.format, alt.set.format, &merged.set.format);
    merged.key.markers = merge_boolean_key(pref.key.markers, pref.set.markers,
            alt.key.markers, alt.set.markers, &merged.set.markers);
    merged.key.input_basedir = merge_string_key(pref.key.input_basedir,
            pref.set.input_basedir, alt.key.input_basedir, alt.set.input_basedir,
            &merged.set.input_basedir);
    merged.key.output_basedir = merge_string_key(pref.key.output_basedir,
            pref.set.output_basedir, alt.key.output_basedir, alt.set.output_basedir,
            &merged.set.output_basedir);

    merged.key.picture_anamorphic = merge_string_key(pref.key.picture_anamorphic,
            pref.set.picture_anamorphic, alt.key.picture_anamorphic, alt.set.picture_anamorphic,
            &merged.set.picture_anamorphic);
    merged.key.picture_autocrop = merge_boolean_key(pref.key.picture_autocrop,
            pref.set.picture_autocrop, alt.key.picture_autocrop,
            alt.set.picture_autocrop, &merged.set.picture_autocrop);
    merged.key.picture_loose_crop = merge_integer_key(pref.key.picture_loose_crop,
            pref.set.picture_loose_crop, alt.key.picture_loose_crop,
            alt.set.picture_loose_crop, &merged.set.picture_loose_crop);
    merged.key.picture_crop_top = merge_integer_key(pref.key.picture_crop_top,
            pref.set.picture_crop_top, alt.key.picture_crop_top,
            alt.set.picture_crop_top, &merged.set.picture_crop_top);
    merged.key.picture_crop_bottom = merge_integer_key(pref.key.picture_crop_bottom,
            pref.set.picture_crop_bottom, alt.key.picture_crop_bottom,
            alt.set.picture_crop_bottom, &merged.set.picture_crop_bottom);
    merged.key.picture_crop_left = merge_integer_key(pref.key.picture_crop_left,
            pref.set.picture_crop_left, alt.key.picture_crop_left,
            alt.set.picture_crop_left, &merged.set.picture_crop_left);
    merged.key.picture_crop_right = merge_integer_key(pref.key.picture_crop_right,
            pref.set.picture_crop_right, alt.key.picture_crop_right,
            alt.set.picture_crop_right, &merged.set.picture_crop_right);

    merged.key.filter_decomb = merge_string_key(pref.key.filter_decomb,
            pref.set.filter_decomb, alt.key.filter_decomb, alt.set.filter_decomb,
            &merged.set.filter_decomb);
    merged.key.filter_deinterlace = merge_string_key(pref.key.filter_deinterlace,
            pref.set.filter_deinterlace, alt.key.filter_deinterlace,
            alt.set.filter_deinterlace, &merged.set.filter_deinterlace);
    merged.key.filter_denoise = merge_string_key(pref.key.filter_denoise,
            pref.set.filter_denoise, alt.key.filter_denoise,
            alt.set.filter_denoise, &merged.set.filter_denoise);
    merged.key.filter_grayscale = merge_boolean_key(pref.key.filter_grayscale,
            pref.set.filter_grayscale, alt.key.filter_grayscale,
            alt.set.filter_grayscale, &merged.set.filter_grayscale);
    merged.key.filter_rotate = merge_string_list_key(
            pref.key.filter_rotate, pref.set.filter_rotate, pref.key.filter_rotate_count,
            alt.key.filter_rotate, alt.set.filter_rotate, alt.key.filter_rotate_count,
            &merged.set.filter_rotate, &merged.key.filter_rotate_count);

    merged.key.audio_encoder = merge_string_key(pref.key.audio_encoder,
            pref.set.audio_encoder, alt.key.audio_encoder, alt.set.audio_encoder,
            &merged.set.audio_encoder);
    merged.key.audio_quality = merge_integer_key(pref.key.audio_quality,
            pref.set.audio_quality, alt.key.audio_quality,
            alt.set.audio_quality, &merged.set.audio_quality);
    merged.key.audio_bitrate = merge_integer_key(pref.key.audio_bitrate,
            pref.set.audio_bitrate, alt.key.audio_bitrate,
            alt.set.audio_bitrate, &merged.set.audio_bitrate);

    merged.key.video_encoder = merge_string_key(pref.key.video_encoder,
            pref.set.video_encoder, alt.key.video_encoder, alt.set.video_encoder,
            &merged.set.video_encoder);
    merged.key.video_quality = merge_integer_key(pref.key.video_quality,
            pref.set.video_quality, alt.key.video_quality,
            alt.set.video_quality, &merged.set.video_quality);
    merged.key.video_bitrate = merge_integer_key(pref.key.video_bitrate,
            pref.set.video_bitrate, alt.key.video_bitrate,
            alt.set.video_bitrate, &merged.set.video_bitrate);
    merged.key.video_framerate = merge_string_key(pref.key.video_framerate,
            pref.set.video_framerate, alt.key.video_framerate,
            alt.set.video_framerate, &merged.set.video_framerate);
    merged.key.video_framerate_control = merge_string_key(
            pref.key.video_framerate_control, pref.set.video_framerate_control,
            alt.key.video_framerate_control, alt.set.video_framerate_control,
            &merged.set.video_framerate_control);
    merged.key.video_turbo = merge_boolean_key(pref.key.video_turbo, pref.set.video_turbo,
            alt.key.video_turbo, alt.set.video_turbo, &merged.set.video_turbo);
    merged.key.video_two_pass = merge_boolean_key(pref.key.video_two_pass, pref.set.video_two_pass,
            alt.key.video_two_pass, alt.set.video_two_pass, &merged.set.video_two_pass);
    return merged;
}

/**
 * @brief Construct a config with all members unset
 *        Variables are uninitialized. Must always check
 *        if a member is set before accessing it's contents.
 *
 * @return struct config
 */
struct config empty_config()
{
    struct config c;
    c.set.format                  = FALSE;
    c.set.markers                 = FALSE;
    c.set.input_basedir           = FALSE; // local config only
    c.set.output_basedir          = FALSE; // local config only

    c.set.picture_anamorphic      = FALSE;
    c.set.picture_autocrop        = FALSE;
    c.set.picture_loose_crop      = FALSE;
    c.set.picture_crop_top        = FALSE; // local config only
    c.set.picture_crop_bottom     = FALSE; // local config only
    c.set.picture_crop_left       = FALSE; // local config only
    c.set.picture_crop_right      = FALSE; // local config only

    c.set.filter_decomb           = FALSE;
    c.set.filter_deinterlace      = FALSE;
    c.set.filter_denoise          = FALSE;
    c.set.filter_grayscale        = FALSE;
    c.set.filter_rotate           = FALSE;

    c.set.audio_encoder           = FALSE;
    c.set.audio_quality           = FALSE;
    c.set.audio_bitrate           = FALSE;

    c.set.video_encoder           = FALSE;
    c.set.video_quality           = FALSE;
    c.set.video_bitrate           = FALSE;
    c.set.video_framerate         = FALSE;
    c.set.video_framerate_control = FALSE;
    c.set.video_turbo             = FALSE;
    c.set.video_two_pass          = FALSE;
    return c;
}

/**
 * @brief Construct a config with defaults set where
 *        applicable.
 *
 * @return struct config
 */
struct config default_config()
{
    // TODO verify all these defaults
    struct config c = empty_config();
    c.set.format                  = TRUE;
    c.key.format                  = g_strdup("mkv");
    c.set.markers                 = TRUE;
    c.key.markers                 = TRUE;

    c.set.picture_anamorphic      = TRUE;
    c.key.picture_anamorphic      = g_strdup("off");;
    c.set.picture_autocrop        = TRUE;
    c.key.picture_autocrop        = FALSE;

    c.set.filter_decomb           = TRUE;
    c.key.filter_decomb           = g_strdup("none");
    c.set.filter_deinterlace      = TRUE;
    c.key.filter_deinterlace      = g_strdup("none");
    c.set.filter_denoise          = TRUE;
    c.key.filter_denoise          = g_strdup("none");
    c.set.filter_grayscale        = TRUE;
    c.key.filter_grayscale        = FALSE;
    c.set.filter_rotate           = TRUE;
    gchar **one                   = g_malloc(sizeof(gchar *)*2);
    one[0]                        = g_strdup("none");
    one[1]                        = NULL;
    c.key.filter_rotate           = one;

    c.set.audio_encoder           = TRUE;
    c.key.audio_encoder           = g_strdup("mp3");
    c.set.audio_bitrate           = TRUE;
    c.key.audio_bitrate           = 160;

    c.set.video_encoder           = TRUE;
    c.key.video_encoder           = g_strdup("x264");
    c.set.video_quality           = TRUE;
    c.key.video_quality           = 20;
    c.set.video_framerate         = TRUE;
    c.key.video_framerate         = g_strdup("source");
    c.set.video_framerate_control = TRUE;
    c.key.video_framerate_control = g_strdup("variable");
    c.set.video_turbo             = TRUE;
    c.key.video_turbo             = FALSE;
    c.set.video_two_pass          = TRUE;
    c.key.video_two_pass          = FALSE;
    return c;
}

/**
 * @brief Frees all allocatable members of a struct config
 *
 * @param c
 */
void free_config(struct config c)
{
    // free all potentially allocated strings
    if (c.set.format)                  { g_free(c.key.format); }
    if (c.set.input_basedir)           { g_free(c.key.input_basedir); }
    if (c.set.output_basedir)          { g_free(c.key.output_basedir); }

    if (c.set.picture_anamorphic)      { g_free(c.key.picture_anamorphic); }

    if (c.set.filter_decomb)           { g_free(c.key.filter_decomb); }
    if (c.set.filter_deinterlace)      { g_free(c.key.filter_deinterlace); }
    if (c.set.filter_denoise)          { g_free(c.key.filter_denoise); }
    if (c.set.filter_rotate)           { g_strfreev(c.key.filter_rotate); }

    if (c.set.audio_encoder)           { g_free(c.key.audio_encoder); }

    if (c.set.video_encoder)           { g_free(c.key.video_encoder); }
    if (c.set.video_framerate)         { g_free(c.key.video_framerate); }
    if (c.set.video_framerate_control) { g_free(c.key.video_framerate_control); }
}

/**
 * @brief Counts number of outfile sections
 *
 * @return Number of outfile sections
 */
gint get_outfile_count(GKeyFile *keyfile)
{
    gsize count = 0;
    gsize len;
    gchar **groups = g_key_file_get_groups(keyfile, &len);
    for (gint i = 0; i < len; i++) {
        if (strncmp("OUTFILE", groups[i], 7) == 0) {
            count++;
        }
    }
    g_strfreev(groups);
    return count;
}

/**
 * @brief Get a list of outfile names in a string array
 *
 * @param keyfile
 *
 * @return
 */
gchar ** get_outfile_list(GKeyFile *keyfile, gsize *outfile_count)
{
    gsize count;
    gchar **groups = g_key_file_get_groups(keyfile, &count);
    gchar **filter_groups;
    *outfile_count = get_outfile_count(keyfile);
    // Copy outfile groups to new list
    filter_groups = g_malloc(sizeof(gchar*)*(*outfile_count+1));
    for (int i = 0, j = 0; i < count; i++) {
        if (strncmp("OUTFILE", groups[i], 7) == 0) {
            filter_groups[j] = g_strdup(groups[i]);
            j++;
        }
    }
    // NULL terminate string array so g_strfreev can free it later
    filter_groups[*outfile_count] = NULL;
    g_strfreev(groups);
    return filter_groups;
}

/**
 * @brief Find a group with matching episode number.
 *
 * @param episode_number Episode number to match.
 *
 * @return
 */
gchar * get_group_from_episode(GKeyFile *keyfile, int episode_number)
{
    gchar *group;
    gsize count = 0;
    gchar **groups = get_outfile_list(keyfile, &count);
    for (int i = 0; i < count; i++) {
        if ( episode_number == get_key_value_integer(keyfile, groups[i],
                    EPISODE_NUMBER, NULL)) {
            group = g_strdup(groups[i]);
            g_strfreev(groups);
            return group;
        }
    }
    g_strfreev(groups);
    return NULL;
}

/**
 * @brief Find first outfile with matching episode number.
 *
 * @param episode_number Episode number to match.
 *
 * @return struct outfile with episode data. Empty struct outfile if no match
 *         Caller must check episode_number is set to determine.
 */
struct outfile get_outfile_from_episode(GKeyFile *keyfile, int episode_number)
{
    gchar *group;
    gsize count = 0;
    gchar **groups = get_outfile_list(keyfile, &count);
    for (int i = 0; i < count; i++) {
        if ( episode_number == get_key_value_integer(keyfile, groups[i],
                    EPISODE_NUMBER, NULL)) {
            g_strfreev(groups);
            return get_outfile(keyfile, groups[i]);
        }
    }
    g_strfreev(groups);
    return empty_outfile();
}

/**
 * @brief Construct a outfile with all members unset
 *        Variables are uninitialized. Must always check
 *        if a member is set before accessing it's contents.
 *
 * @return struct outfile
 */
struct outfile empty_outfile()
{
    struct outfile o;
    o.set.type           = FALSE;
    o.set.iso_filename   = FALSE;
    o.set.dvdtitle       = FALSE;
    o.set.name           = FALSE;
    o.set.year           = FALSE;
    o.set.season         = FALSE;
    o.set.episode_number = FALSE;
    o.set.specific_name  = FALSE;
    o.set.crop_top       = FALSE;
    o.set.crop_bottom    = FALSE;
    o.set.crop_left      = FALSE;
    o.set.crop_right     = FALSE;
    o.set.chapters_start = FALSE;
    o.set.chapters_end   = FALSE;
    o.set.audio          = FALSE;
    o.set.subtitle       = FALSE;
    return o;
}

/**
 * @brief Fetch outfile data for a specific group and build an outfile struct
 *
 * @param keyfile
 * @param group
 *
 * @return
 */
struct outfile get_outfile(GKeyFile *keyfile, gchar *group)
{
    struct outfile o = empty_outfile();
    //verify CONFIG group exists in file
    if (!g_key_file_has_group(keyfile, group))
    {
        return empty_outfile();
    }
    o.key.type = get_key_value_string(keyfile, group,
            TYPE, &o.set.type);
    o.key.iso_filename = get_key_value_string(keyfile, group,
            ISO_FILENAME, &o.set.iso_filename);
    o.key.dvdtitle = get_key_value_integer(keyfile, group,
            DVDTITLE, &o.set.dvdtitle);
    o.key.name = get_key_value_string(keyfile, group,
            NAME, &o.set.name);
    o.key.year = get_key_value_string(keyfile, group,
            YEAR, &o.set.year);
    o.key.season = get_key_value_integer(keyfile, group,
            SEASON, &o.set.season);
    o.key.episode_number = get_key_value_integer(keyfile, group,
            EPISODE_NUMBER, &o.set.episode_number);
    o.key.specific_name = get_key_value_string(keyfile, group,
            SPECIFIC_NAME, &o.set.specific_name);
    o.key.crop_top = get_key_value_integer(keyfile, group,
            CROP_TOP, &o.set.crop_top);
    o.key.crop_bottom = get_key_value_integer(keyfile, group,
            CROP_BOTTOM, &o.set.crop_bottom);
    o.key.crop_left = get_key_value_integer(keyfile, group,
            CROP_LEFT, &o.set.crop_left);
    o.key.crop_right = get_key_value_integer(keyfile, group,
            CROP_RIGHT, &o.set.crop_right);
    o.key.chapters_start = get_key_value_integer(keyfile, group,
            CHAPTERS_START, &o.set.chapters_start);
    o.key.chapters_end = get_key_value_integer(keyfile, group,
            CHAPTERS_END, &o.set.chapters_end);
    o.key.audio = get_key_value_integer_list(keyfile, group,
            AUDIO, &o.key.audio_count, &o.set.audio);
    o.key.subtitle = get_key_value_integer_list(keyfile, group,
            SUBTITLE, &o.key.subtitle_count, &o.set.subtitle);
    return o;
}

/**
 * @brief Frees all allocatable members of a struct outfile
 *
 * @param o
 */
void free_outfile(struct outfile o)
{
    if (o.set.type)          { g_free(o.key.type); }
    if (o.set.iso_filename)  { g_free(o.key.iso_filename); }
    if (o.set.name)          { g_free(o.key.name); }
    if (o.set.year)          { g_free(o.key.year); }
    if (o.set.specific_name) { g_free(o.key.specific_name); }
    if (o.set.audio)         { g_free(o.key.audio); }
    if (o.set.subtitle)      { g_free(o.key.subtitle); }
}

gboolean validate_key_file(gchar *infile, GKeyFile *keyfile, gboolean global)
{
    /* PRE-PARSING CHECKS */
    // check for duplicate sections?? maybe impossible

    /* PARSE AND REPORT ANY ERRORS */

    /* POST_PARSING CHECKS */
    // check CONFIG exists
    // check for unknown sections
    // check for unknown keys
    // check required keys exist (independent)
    // check required keys exist (dependent)
    // check known keys values (independent)
    // check known keys values (dependent)

    /* NOT GLOBAL CONFIG */
    // check at least one OUTFILE exists
    // check required keys exist (independent)
    // check required keys exist (dependent)
    // check known keys values (independent)
    // check known keys values (dependent)
}

/**
 * @brief Get key value of type string
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return string value or NULL if not found
 */
gchar* get_key_value_string(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    GError *error = NULL;
    gchar *val = g_key_file_get_string (keyfile, group_name, key, &error);
    if (error == NULL) {
        if (strcmp(val, "") == 0) {
            // handle case of key in file, but no value provided
            g_free(val);
            val = NULL;
        } else if (set != NULL) {
            *set = TRUE;
        }
    } else {
        g_error_free(error);
    }
    return val;
}

/**
 * @brief Get key value of type boolean
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return boolean value
 */
gboolean get_key_value_boolean(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    GError *error = NULL;
    gboolean val = g_key_file_get_boolean(keyfile, group_name, key, &error);
    gchar *temp = g_key_file_get_string (keyfile, group_name, key, NULL);
    if (error == NULL) {
        if (strcmp(temp, "") == 0) {
            // handle case of key in file, but no value provided
        } else if (set != NULL) {
            *set = TRUE;
        }
    } else {
        g_error_free(error);
    }
    g_free(temp);
    return val;
}

/**
 * @brief Get key value of type integer
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return integer value
 */
gint get_key_value_integer(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    GError *error = NULL;
    gint val = g_key_file_get_integer(keyfile, group_name, key, &error);
    gchar *temp = g_key_file_get_string (keyfile, group_name, key, NULL);
    if (error == NULL) {
        if (strcmp(temp, "") == 0) {
            // handle case of key in file, but no value provided
        } else if (set != NULL) {
            *set = TRUE;
        }
    } else {
        g_error_free(error);
    }
    g_free(temp);
    return val;

}

/**
 * @brief Get key value of type double
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return double value
 */
gdouble get_key_value_double(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gboolean *set)
{
    GError *error = NULL;
    gdouble val = g_key_file_get_double(keyfile, group_name, key, &error);
    gchar *temp = g_key_file_get_string (keyfile, group_name, key, NULL);
    if (error == NULL) {
        if (strcmp(temp, "") == 0) {
            // handle case of key in file, but no value provided
        } else if (set != NULL) {
            *set = TRUE;
        }
    } else {
        g_error_free(error);
    }
    g_free(temp);
    return val;
}

/**
 * @brief Get key value of type string list
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return string list value or NULL if not found
 */
gchar** get_key_value_string_list(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    GError *error = NULL;
    gchar **val = g_key_file_get_string_list(keyfile, group_name, key, length, &error);
    gchar *temp = g_key_file_get_string (keyfile, group_name, key, NULL);
    if (error == NULL) {
        if (strcmp(temp, "") == 0) {
            // handle case of key in file, but no value provided
            g_strfreev(val);
            val = NULL;
        } else if (set != NULL) {
            *set = TRUE;
        }
    } else {
        g_error_free(error);
    }
    g_free(temp);
    return val;
}

/**
 * @brief Get key value of type gboolean list
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return gboolean list value or NULL if not found
 */
gboolean* get_key_value_boolean_list(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    GError *error = NULL;
    gboolean* val = g_key_file_get_boolean_list(keyfile, group_name, key, length, &error);
    gchar *temp = g_key_file_get_string (keyfile, group_name, key, NULL);
    if (error == NULL) {
        if (strcmp(temp, "") == 0) {
            // handle case of key in file, but no value provided
            g_free(val);
            val = NULL;
        } else if (set != NULL) {
            *set = TRUE;
        }
    } else {
        g_error_free(error);
    }
    g_free(temp);
    return val;
}

/**
 * @brief Get key value of type integer list
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return integer list value or NULL if not found
 */
gint* get_key_value_integer_list(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    GError *error = NULL;
    gint* val = g_key_file_get_integer_list(keyfile, group_name, key, length, &error);
    gchar *temp = g_key_file_get_string (keyfile, group_name, key, NULL);
    if (error == NULL) {
        if (strcmp(temp, "") == 0) {
            // handle case of key in file, but no value provided
            g_free(val);
            val = NULL;
        } else if (set != NULL) {
            *set = TRUE;
        }
    } else {
        g_error_free(error);
    }
    g_free(temp);
    return val;
}

/**
 * @brief Get key value of type double list
 *
 * @param keyfile Parsed key file
 * @param group_name Group to find key in
 * @param key Key name
 * @param set Optional, set true when a key value was found, pass NULL to ignore
 *
 * @return double list value or NULL if not found
 */
gdouble* get_key_value_double_list(GKeyFile *keyfile, const gchar *group_name,
        const gchar *key, gsize *length, gboolean *set)
{
    GError *error = NULL;
    gdouble* val = g_key_file_get_double_list(keyfile, group_name, key, length, &error);
    gchar *temp = g_key_file_get_string (keyfile, group_name, key, NULL);
    if (error == NULL) {
        if (strcmp(temp, "") == 0) {
            // handle case of key in file, but no value provided
            g_free(val);
            val = NULL;
        } else if (set != NULL) {
            *set = TRUE;
        }
    } else {
        g_error_free(error);
    }
    g_free(temp);
    return val;
}

gchar * merge_string_key(gchar *pref_val, gboolean pref_set, gchar* alt_val,
        gboolean alt_set, gboolean *result_set)
{
    if (pref_set) {
        *result_set = TRUE;
        return g_strdup(pref_val);
    } else if (alt_set) {
        *result_set = TRUE;
        return g_strdup(alt_val);
    }
    return NULL;
}

gboolean merge_boolean_key(gboolean pref_val, gboolean pref_set, gboolean alt_val,
        gboolean alt_set, gboolean *result_set)
{
    if (pref_set) {
        *result_set = TRUE;
        return pref_val;
    } else if (alt_set) {
        *result_set = TRUE;
        return alt_val;
    }
    return FALSE; // this value is disregarded since result_set remains FALSE
}

gint merge_integer_key(gint pref_val, gboolean pref_set, gint alt_val,
        gboolean alt_set, gboolean *result_set)
{
    if (pref_set) {
        *result_set = TRUE;
        return pref_val;
    } else if (alt_set) {
        *result_set = TRUE;
        return alt_val;
    }
    return 0; // this value is disregarded since result_set remains FALSE
}

gdouble merge_double_key(gdouble pref_val, gboolean pref_set, gdouble alt_val,
        gboolean alt_set, gboolean *result_set)
{
    if (pref_set) {
        *result_set = TRUE;
        return pref_val;
    } else if (alt_set) {
        *result_set = TRUE;
        return alt_val;
    }
    return 0; // this value is disregarded since result_set remains FALSE
}

gchar ** merge_string_list_key(
        gchar **pref_val, gboolean pref_set, gsize pref_count,
        gchar **alt_val, gboolean alt_set, gsize alt_count,
        gboolean *result_set, gsize *result_count)
{
    gchar **temp = NULL;
    int i;
    if (pref_set) {
        *result_set = TRUE;
        // allocate extra pointer for NULL terminated array
        temp = g_malloc(sizeof(gchar*)*(pref_count+1));
        *result_count = pref_count;
        for (i = 0; i < pref_count; i++) {
            temp[i] = g_strdup(pref_val[i]);
        }
        // null terminate array
        temp[i] = NULL;
    } else if (alt_set) {
        *result_set = TRUE;
        // allocate extra pointer for NULL terminated array
        temp = g_malloc(sizeof(gchar*)*(alt_count+1));
        *result_count = alt_count;
        for (i = 0; i < alt_count; i++) {
            temp[i] = g_strdup(alt_val[i]);
        }
        // null terminate array
        temp[i] = NULL;
    }
    return temp;
}

gboolean * merge_boolean_list_key(
        gboolean *pref_val, gboolean pref_set, gsize pref_count,
        gboolean *alt_val, gboolean alt_set, gsize alt_count,
        gboolean *result_set, gsize *result_count)
{
    gboolean *temp = NULL;
    int i;
    if (pref_set) {
        *result_set = TRUE;
        temp = g_malloc(sizeof(gboolean)*pref_count);
        *result_count = pref_count;
        for (i = 0; i < pref_count; i++) {
            temp[i] = pref_val[i];
        }
    } else if (alt_set) {
        *result_set = TRUE;
        temp = g_malloc(sizeof(gboolean)*alt_count);
        *result_count = alt_count;
        for (i = 0; i < alt_count; i++) {
            temp[i] = alt_val[i];
        }
    }
    return temp;
}

gint * merge_integer_list_key(
        gint *pref_val, gboolean pref_set, gsize pref_count,
        gint *alt_val, gboolean alt_set, gsize alt_count,
        gboolean *result_set, gsize *result_count)
{
    gint *temp = NULL;
    int i;
    if (pref_set) {
        *result_set = TRUE;
        temp = g_malloc(sizeof(gint)*pref_count);
        *result_count = pref_count;
        for (i = 0; i < pref_count; i++) {
            temp[i] = pref_val[i];
        }
    } else if (alt_set) {
        *result_set = TRUE;
        temp = g_malloc(sizeof(gint)*alt_count);
        *result_count = alt_count;
        for (i = 0; i < alt_count; i++) {
            temp[i] = alt_val[i];
        }
    }
    return temp;
}

gdouble * merge_double_list_key(
        gdouble *pref_val, gboolean pref_set, gsize pref_count,
        gdouble *alt_val, gboolean alt_set, gsize alt_count,
        gboolean *result_set, gsize *result_count)
{
    gdouble *temp = NULL;
    int i;
    if (pref_set) {
        *result_set = TRUE;
        temp = g_malloc(sizeof(gdouble)*pref_count);
        *result_count = pref_count;
        for (i = 0; i < pref_count; i++) {
            temp[i] = pref_val[i];
        }
    } else if (alt_set) {
        *result_set = TRUE;
        temp = g_malloc(sizeof(gdouble)*alt_count);
        *result_count = alt_count;
        for (i = 0; i < alt_count; i++) {
            temp[i] = alt_val[i];
        }
    }
    return temp;
}
