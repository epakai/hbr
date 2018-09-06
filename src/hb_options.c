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

#include <string.h>
#include <stdio.h>
#include "hb_options.h"

/**
 * @brief Build HandBrakeCLI general arguments
 *
 * @param doc XML document to read values from
 *
 * @return String of command line arguments
 */
gchar* hb_options_string(struct config global_config, GKeyFile *keyfile)
{
    // parse local file config
    struct config local_config = get_local_config(keyfile);

    // allocate and initialize options string
    gchar* opt_str = g_strdup("\0");
    gchar* options[19];

    options[0]  = hb_format(global_config, local_config);
    options[1]  = hb_markers(global_config, local_config);

    options[2]  = hb_picture_anamorphic(global_config, local_config);
    options[3]  = hb_picture_autocrop(global_config, local_config);
    options[4]  = hb_picture_loose_crop(global_config, local_config);

    // TODO maybe combine decomb/deinterlace since deinterelace overrides decomb
    options[5]  = hb_filter_decomb(global_config, local_config);
    options[6]  = hb_filter_deinterlace(global_config, local_config);
    options[7]  = hb_filter_denoise(global_config, local_config);
    options[8]  = hb_filter_grayscale(global_config, local_config);
    options[9]  = hb_filter_rotate(global_config, local_config);

    options[10] = hb_audio_encoder(global_config, local_config);
    // TODO maybe combine quality/bitrate since they're mutually exclusive
    options[11] = hb_audio_quality(global_config, local_config);
    options[12] = hb_audio_bitrate(global_config, local_config);

    options[13] = hb_video_encoder(global_config, local_config);
    // TODO maybe combine quality/bitrate since they're mutually exclusive
    options[14] = hb_video_quality(global_config, local_config);
    options[15] = hb_video_bitrate(global_config, local_config);
    options[16] = hb_video_framerate(global_config, local_config);
    options[17] = hb_video_framerate_control(global_config, local_config);
    options[18] = hb_video_turbo(global_config, local_config);
    int i;
    for ( i = 0; i < 19; i++ ) {
        if ( options[i] == NULL ) {
            fprintf(stderr, "Error while building handbrake options.\n");
            return NULL;
        }
        opt_str = strcat(opt_str, options[i]);
        g_free(options[i]);
    }

    opt_str = strncat(opt_str, " ", 1);
    return opt_str;
}
/**
 * @brief return handbrake option for file format (-f)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space, defaults to mkv on failure
 */
gchar* hb_format(struct config global_config, struct config local_config)
{
    gchar* temp;
    if (local_config.set.format == TRUE) {
        temp = local_config.key.format;
    } else if (global_config.set.format == TRUE) {
        temp = global_config.key.format;
    } else {
        fprintf(stdout, "Output container format not set. Defaulting to mkv.\n");
        temp = g_strdup("mkv");
    }

    if (strcmp(temp, "mkv") == 0) {
        g_free(temp);
        return g_strdup(" -f av_mkv");
    } else if (strcmp(temp, "mp4") == 0) {
        g_free(temp);
        return g_strdup(" -f av_mp4");
    } else {
        fprintf(stderr, "Invalid output container format \"%s\". Defaulting to mkv.\n", temp);
        g_free(temp);
        return g_strdup(" -f av_mkv");
    }
}

//TODO update params and return
/**
 * @brief return handbrake option for markers (-m)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space
 */
gchar* hb_markers(struct config global_config, struct config local_config)
{
    gboolean temp;
    if (local_config.set.markers == TRUE) {
        temp = local_config.key.markers;
    } else if (global_config.set.markers == TRUE) {
        temp = global_config.key.markers;
    } else {
        fprintf(stdout, "Chapter markers not set. Defaulting to no markers.\n");
        temp = FALSE;
    }

    if (temp) {
        return g_strdup(" -m");
    } else {
        return g_strdup("");
    }
}

//TODO update params and return
/**
 * @brief return handbrake option for anamorphic (--strict-anamorphic or --loose-anamorphic)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space
 */
gchar* hb_picture_anamorphic(struct config global_config, struct config local_config)
{
    gchar* temp;
    if (local_config.set.format == TRUE) {
        temp = local_config.key.format;
    } else if (global_config.set.format == TRUE) {
        temp = global_config.key.format;
    } else {
        g_free(temp);
        return g_strdup("");
    }

    if (strcmp(temp, "strict") == 0 ) {
        g_free(temp);
        return g_strdup(" --strict-anamorphic");
    } else if (strcmp(temp, "loose") == 0 ) {
        g_free(temp);
        return g_strdup(" --loose-anamorphic");
    } else if (strcmp(temp, "off") == 0 ) {
        g_free(temp);
        return g_strdup("");
    } else {
        fprintf(stderr, "Invalid picture_anamorphic setting: \"%s\". Defaulting to off.\n", temp);
        g_free(temp);
        return g_strdup("");
    }
}


/**
 * @brief TODO
 *
 * @param global_config
 * @param local_config
 *
 * @return
 */
gchar* hb_picture_autocrop(struct config global_config, struct config local_config)
{
    gboolean temp;
    if (local_config.set.picture_autocrop == TRUE) {
        temp = local_config.key.picture_autocrop;
    } else if (global_config.set.picture_autocrop == TRUE) {
        temp = global_config.key.picture_autocrop;
    } else {
        fprintf(stdout, "Autocrop not set. Defaulting to no autocrop.\n");
        temp = FALSE;
    }

    if (temp) {
        return g_strdup(" --crop");
    } else {
        return g_strdup("");
    }
}


/**
 * @brief TODO
 *
 * @param global_config
 * @param local_config
 *
 * @return
 */
gchar* hb_picture_loose_crop(struct config global_config, struct config local_config)
{
    gint temp;
    if (local_config.set.picture_loose_crop == TRUE) {
        temp = local_config.key.picture_loose_crop;
    } else if (global_config.set.picture_loose_crop == TRUE) {
        temp = global_config.key.picture_loose_crop;
    } else {
        fprintf(stdout, "Loose crop not set. Defaulting to no loose crop.\n");
        return g_strdup("");
    }

    gchar arg[20];
    g_snprintf(arg, 20, " --loose-crop %d", temp);
    return g_strdup(arg);
}

//TODO update params and return
/**
 * @brief return handbrake option for decombing (-5)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space
 */
gchar* hb_filter_decomb(struct config global_config, struct config local_config)
{
    gchar* temp;
    if (local_config.set.filter_decomb == TRUE) {
        temp = local_config.key.filter_decomb;
    } else if (global_config.set.filter_decomb == TRUE) {
        temp = global_config.key.filter_decomb;
    } else {
        fprintf(stdout, "Decomb not set. Defaulting to no decomb.\n");
        temp = g_strdup("none");
    }

    if (strcmp(temp, "fast") == 0){
        g_free(temp);
        return g_strdup(" -5 fast");
    } else if (strcmp(temp, "bob") == 0) {
        g_free(temp);
        return g_strdup(" -5 bob");
    } else if (strcmp(temp, "default") == 0 ) {
        g_free(temp);
        return g_strdup(" -5");
    } else if (strcmp(temp, "none") == 0) {
        g_free(temp);
        return g_strdup("");
    } else {
        fprintf(stderr, "Invalid filter_decomb setting: \"%s\". Defaulting to no decomb.\n", temp);
        g_free(temp);
        return g_strdup("");
    }
}

//TODO update params and return
/**
 * @brief return handbrake option for deinterlacing (-d)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space
 */
gchar* hb_filter_deinterlace(struct config global_config, struct config local_config)
{
    gchar* temp;
    if (local_config.set.filter_deinterlace == TRUE) {
        temp = local_config.key.filter_deinterlace;
    } else if (global_config.set.filter_deinterlace == TRUE) {
        temp = global_config.key.filter_deinterlace;
    } else {
        fprintf(stdout, "Decomb not set. Defaulting to no decomb.\n");
        temp = g_strdup("none");
    }

    if (strcmp(temp, "fast") == 0 || strcmp(temp, "slow") == 0 || strcmp(temp, "slower") == 0 ||
            strcmp(temp, "bob") == 0) {
        gchar* arg = g_strdup(" -d ");
        arg = strncat(arg, temp, strlen(temp));
        g_free(temp);
        return arg;
    } else if (strcmp(temp, "default") == 0 ) {
        g_free(temp);
        return g_strdup(" -d");
    } else if (strcmp(temp, "none") == 0) {
        g_free(temp);
        return g_strdup("");
    } else {
        fprintf(stderr, "Invalid filter_decomb setting: \"%s\". Defaulting to no decomb.\n", temp);
        // This condition shouldn't be reached unless the DTD is modified
        g_free(temp);
        return NULL;
    }
}

//TODO update params and return
/**
 * @brief return handbrake option for denoising (-8)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space
 */
gchar* hb_filter_denoise(struct config global_config, struct config local_config)
{
    gchar* arg = g_strdup(" -8 ");
    gchar* temp = xmlGetNoNsProp(options, "denoise");

    if (strcmp(temp, "ultralight") == 0 ||
            strcmp(temp, "light") == 0 ||
            strcmp(temp, "medium") == 0 ||
            strcmp(temp, "strong") == 0 ||
            strcmp(temp, "default") == 0 ) {
        arg = strncat(arg, temp, strlen(temp));
        g_free(temp);
        return arg;
    } else if (strcmp(temp, "none") == 0) {
        g_free(arg);
        g_free(temp);
        return g_strdup("");
    } else {
        // This condition shouldn't be reached unless the DTD is modified
        fprintf(stderr, "Invalid handbrake_options denoise attribute\n");
        g_free(arg);
        g_free(temp);
        return NULL;
    }
}

/**
 * @brief TODO
 *
 * @param global_config
 * @param local_config
 *
 * @return
 */
gchar* hb_filter_grayscale(struct config global_config, struct config local_config)
{
    //TODO
}

/**
 * @brief TODO
 *
 * @param global_config
 * @param local_config
 *
 * @return
 */
gchar* hb_filter_rotate(struct config global_config, struct config local_config)
{
    //TODO
}

//TODO update params and return
/**
 * @brief return handbrake option for audio encoder (-E)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space
 */
gchar* hb_audio_encoder(struct config global_config, struct config local_config)
{
    gchar* arg = g_strdup(" -E ");
    gchar* encoder = xmlGetNoNsProp(options, "audio_encoder");
    if (strcmp(encoder, "av_aac") == 0 ||
            strcmp(encoder, "fdk_aac") == 0 ||
            strcmp(encoder, "fdk_haac") == 0 ||
            strcmp(encoder, "copy:aac") == 0 ||
            strcmp(encoder, "ac3") == 0 ||
            strcmp(encoder, "copy:ac3") == 0 ||
            strcmp(encoder, "copy:dts") == 0 ||
            strcmp(encoder, "copy:dtshd") == 0 ||
            strcmp(encoder, "mp3") == 0 ||
            strcmp(encoder, "copy:mp3") == 0 ||
            strcmp(encoder, "vorbis") == 0 ||
            strcmp(encoder, "flac16") == 0 ||
            strcmp(encoder, "flac24") == 0 ||
            strcmp(encoder, "copy") == 0 ) {
        arg = strncat(arg, encoder, strlen(encoder));
        g_free(encoder);
        return arg;
    } else {
        // This condition shouldn't be reached unless the DTD is modified
        fprintf(stderr, "Invalid handbrake_options audio_encoder attribute\n");
        return NULL;
    }
}

//TODO update params and return
/**
 * @brief return handbrake option for audio quality (-Q)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space
 */
gchar* hb_audio_quality(struct config global_config, struct config local_config)
{
    gchar* arg = g_strdup(" -Q ");
    gchar* quality = xmlGetNoNsProp(options, "audio_quality");
    if (quality[0] == '\0') {
        g_free(arg);
        g_free(quality);
        // Empty string lets Handbrake pick the default
        return g_strdup("");
    }
    gchar* codec = hb_audio_encoder(options);
    if (strcmp(codec, " -E mp3") == 0
            || strcmp(codec, " -E vorbis") == 0) {
        float a_quality = strtof((char *) quality, NULL);
        char temp[5];
        if (codec[4] == 'm') { //mp3
            if (a_quality > 0.0 && a_quality < 10.0) {
                snprintf(temp, 5, "%f.1", a_quality);
            }
        } else if (codec[4] == 'v') { //vorbis
            if (a_quality > -2.0 && a_quality < 10.0) {
                snprintf(temp, 5, "%f.1", a_quality);
            }
        } else {
            goto invalid_bitrate;
        }
        arg = strncat(arg, temp, 5);
        g_free(quality);
        g_free(codec);
        return arg;
    } else {
invalid_bitrate:
        //TODO see if we can add line number support back in
        fprintf(stderr,
                "Invalid quality'%s' for %s codec in \"%s\" line number: ~\n",
                quality, xmlStrsub(codec, 4, 7), doc->URL);
        g_free(arg);
        g_free(quality);
        g_free(codec);
        return NULL;
    }
}

//TODO update params and return
/**
 * @brief return handbrake option for audio bitrate (-B)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space
 */
gchar* hb_audio_bitrate(struct config global_config, struct config local_config)
{
    gchar* arg = g_strdup(" -B ");
    gchar* bitrate = xmlGetNoNsProp(options, "audio_bitrate");
    if (bitrate[0] == '\0') {
        g_free(arg);
        g_free(bitrate);
        // Empty string lets Handbrake pick the default
        return g_strdup("");
    }
    int br = strtol((char *) bitrate, NULL, 10);
    gchar* codec = hb_audio_encoder(options);
    if ( ((strcmp(codec, " -E mp3") == 0)
                || (strcmp(codec, " -E copy:mp3") == 0))
            && valid_bit_rate(br, 32, 320)) {
        goto good_bitrate;
    }
    if ( ((strcmp(codec, " -E av_aac") == 0)
                || (strcmp(codec, " -E copy:aac") == 0))
            && valid_bit_rate(br, 192, 1536 )) {
        goto good_bitrate;
    }
    if ( (strcmp(codec, " -E vorbis") == 0)
            && valid_bit_rate(br, 192, 1344)) {
        goto good_bitrate;
    }
    if ( ((strcmp(codec, " -E fdk_aac") == 0)
                || (strcmp(codec, " -E copy:dtshd") == 0)
                || (strcmp(codec, " -E copy") == 0)
                || (strcmp(codec, " -E copy:dts") == 0))
            && valid_bit_rate(br, 160, 1344 )) {
        goto good_bitrate;
    }
    if ( ((strcmp(codec, " -E ac3") == 0)
                || ( strcmp(codec, " -E copy:ac3") == 0))
            && valid_bit_rate(br, 224, 640)) {
        goto good_bitrate;
    }
    if ( (strcmp(codec, " -E fdk_haac") == 0)
            && valid_bit_rate(br, 80, 256)) {
        goto good_bitrate;
    }
    //TODO see if we can add line number support back in
    fprintf(stderr,
            "Invalid bitrate '%s' for %s codec in \"%s\" line number: ~\n",
            bitrate, xmlStrsub(codec, 4, 11), doc->URL);
    g_free(bitrate);
    g_free(codec);
    g_free(arg);
    return NULL;
good_bitrate:
    arg = strncat(arg, bitrate, strlen(bitrate));
    g_free(bitrate);
    g_free(codec);
    return arg;
}

//TODO update params and return
/**
 * @brief return handbrake option for video encoder (-e)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space
 */
gchar* hb_video_encoder(struct config global_config, struct config local_config)
{
    gchar* arg = g_strdup(" -e ");
    gchar* temp = xmlGetNoNsProp(options, "video_encoder");
    if (strcmp(temp, "x264") == 0 ||
            strcmp(temp, "mpeg4") == 0 ||
            strcmp(temp, "mpeg2") == 0 ||
            strcmp(temp, "VP8") == 0 ||
            strcmp(temp, "theora") == 0 ) {
        arg = strncat(arg, temp, strlen(temp));
        g_free(temp);
        return arg;
    } else {
        // This condition shouldn't be reached unless the DTD is modified
        fprintf(stderr, "Invalid handbrake_options video_encoder attribute\n");
        return NULL;
    }
}

//TODO update params and return
/**
 * @brief return handbrake option for video quality (-q)
 *
 * @param global_config
 * @param local_config
 *
 * @return command line option string starting with a space
 */
gchar* hb_video_quality(struct config global_config, struct config local_config)
{
    gchar* quality_string;
    if ((quality_string = xmlGetNoNsProp(options, "video_quality"))[0] == '\0') {
        g_free(quality_string);
        // Empty string lets Handbrake pick the default
        return g_strdup("");
    }

    gchar* encoder = hb_video_encoder(options);
    gchar* temp = g_strdup(" -q ");

    // test for max of two digits
    if ( strlen(quality_string) > 2
            || !isdigit(quality_string[0])
            || !isdigit(quality_string[1])) {
        goto invalid_quality;
    }
    long quality = strtol((char *) quality_string, NULL, 10);
    if ( 0 <= quality && quality <=63 ) {
        if ( strncmp(encoder, " -e VP8", 7) == 0 ||
                strncmp(encoder, " -e theora", 10) == 0) {
            goto good_quality;
        }
    }
    if ( 0 <= quality && quality <=51 ) {
        if ( strncmp(encoder, " -e x264", 8) == 0) {
            goto good_quality;
        }
    }
    if ( 1 <= quality && quality <=31 ) {
        if ( strncmp(encoder, " -e mpeg4", 9) == 0 ||
                strncmp(encoder, " -e mpeg2", 9) == 0) {
            goto good_quality;
        }
    }
invalid_quality:
    //TODO see if we can add line number support back in
    fprintf(stderr, "Invalid video quality '%s' for %s encoder in \"%s\""
            "line number: ~\n",
            quality_string, encoder, doc->URL);
    g_free(quality_string);
    g_free(encoder);
    g_free(temp);
    return NULL;
good_quality:
    temp = strcat(temp, quality_string);
    g_free(quality_string);
    g_free(encoder);
    return temp;
}

/**
 * @brief TODO
 *
 * @param global_config
 * @param local_config
 *
 * @return
 */
gchar* hb_video_bitrate(struct config global_config, struct config local_config)
{
    //TODO
}

/**
 * @brief TODO
 *
 * @param global_config
 * @param local_config
 *
 * @return
 */
gchar* hb_video_framerate(struct config global_config, struct config local_config)
{
    //TODO
}

/**
 * @brief TODO
 *
 * @param global_config
 * @param local_config
 *
 * @return
 */
gchar* hb_video_framerate_control(struct config global_config, struct config local_config)
{
    //TODO
}

/**
 * @brief TODO
 *
 * @param global_config
 * @param local_config
 *
 * @return
 */
gchar* hb_video_turbo(struct config global_config, struct config local_config)
{
    //TODO
    // also checks for and sets two-pass option if turbo is set
}

/**
 * @brief TODO
 *
 * @param global_config
 * @param local_config
 *
 * @return
 */
gchar* hb_video(struct config global_config, struct config local_config)
{
    //TODO
}

/**
 * @brief return string for file format (mp4 or mkv)
 *
 * @param doc XML document to read values from
 *
 * @return either "mp4" or "mkv", caller must g_free()
 */
gchar* get_format(GKeyFile* keyfile)
{
    xmlNode *root_element = xmlDocGetRootElement(doc);
    // Find the handbrake_options element
    xmlNode *cur = root_element->children;
    while (cur && strcmp(cur->name, "handbrake_options")) {
        cur = cur->next;
    }

    return xmlGetNoNsProp(cur, "format");
}

/**
 * @brief return string of the base directory for input files
 *
 * @param doc XML document to read values from
 *
 * @return path for input files, caller must g_free()
 */
gchar* get_input_basedir(GKeyFile* keyfile)
{
    xmlNode *root_element = xmlDocGetRootElement(doc);
    // Find the handbrake_options element
    xmlNode *cur = root_element->children;
    while (cur && strcmp(cur->name, "handbrake_options")) {
        cur = cur->next;
    }

    return xmlGetNoNsProp(cur, "input_basedir");
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
        return false;
    }
    if (minimum > maximum) {
        fprintf(stderr, "Minimum exceeds maximum in valid_bit_rate()\n");
        return false;
    }

    // list is slightly reordered to put common rates first
    int valid_bitrates[] = { 128, 160, 192, 224, 256, 320,
        384, 448, 512, 32, 40, 48, 56, 64, 80, 96, 112, 576, 640,
        768, 960, 1152, 1344, 1536, 2304, 3072, 4608, 6144 };
    int i;
    for (i=0; i<(sizeof(valid_bitrates)/sizeof(int)); i++) {
        if (bitrate == valid_bitrates[i]){
            if (bitrate >= minimum && bitrate <= maximum) {
                return true; //valid
            }
        }
    }
    return false;
}
