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

#include "hb_options.h"

/**
 * @brief Build HandBrakeCLI general arguments
 *
 * @param doc XML document to read values from
 *
 * @return String of command line arguments
 */
gchar* hb_options_string(GKeyFile* keyfile)
{
	// Find the handbrake_options element
	while (cur && strcmp(cur->name, BAD_CAST "handbrake_options")) {
		cur = cur->next;
	}

	// allocate and initialize options string
	gchar* opt_str = g_strdup("\0");
	gchar* options[11];

	options[0] = hb_format(cur);
	options[1] = hb_video_encoder(cur);
	options[2] = hb_video_quality(cur, doc);
	options[3] = hb_audio_encoder(cur);
	options[4] = hb_audio_quality(cur, doc);
	options[5] = hb_audio_bitrate(cur, doc);
	options[6] = hb_markers(cur);
	options[7] = hb_anamorphic(cur);
	options[8] = hb_deinterlace(cur);
	options[9] = hb_decomb(cur);
	options[10] = hb_denoise(cur);
	int i;
	for ( i = 0; i < 11; i++ ) {
		if ( options[i] == NULL ) {
			fprintf(stderr,
					"Error while building handbrake options for \"%s\".\n",
					doc->URL);

			return NULL;
		}
		opt_str = strcat(opt_str, BAD_CAST options[i]);
		g_free(options[i]);
	}

	opt_str = strncat(opt_str, BAD_CAST " ", 1);
	return opt_str;
}

/**
 * @brief return handbrake option for file format (-f)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space, NULL on failure
 */
gchar* hb_format(gchar* options)
{
	gchar* temp = xmlGetNoNsProp(options, BAD_CAST "format");
	// format is constrained by the XML DTD to be "mkv" or "mp4"
	if (strcmp(temp, BAD_CAST "mkv") == 0){
		g_free(temp);
		return g_strdup(" -f av_mkv");
	} else if (strcmp(temp, BAD_CAST "mp4") == 0){
		g_free(temp);
		return g_strdup(" -f av_mp4");
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options format attribute\n");
		return NULL;
	}
}

/**
 * @brief return handbrake option for video encoder (-e)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space
 */
gchar* hb_video_encoder(gchar* options)
{
	gchar* arg = g_strdup(" -e ");
	gchar* temp = xmlGetNoNsProp(options, BAD_CAST "video_encoder");
	if (strcmp(temp, BAD_CAST "x264") == 0 ||
			strcmp(temp, BAD_CAST "mpeg4") == 0 ||
			strcmp(temp, BAD_CAST "mpeg2") == 0 ||
			strcmp(temp, BAD_CAST "VP8") == 0 ||
			strcmp(temp, BAD_CAST "theora") == 0 ) {
		arg = strncat(arg, temp, strlen(temp));
		g_free(temp);
		return arg;
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options video_encoder attribute\n");
		return NULL;
	}
}

/**
 * @brief return handbrake option for video quality (-q)
 *
 * @param options handbrake_options pointer
 * @param doc xml document, for error indicating
 *
 * @return command line option string starting with a space
 */
gchar* hb_video_quality(gchar* options, GKeyFile* keyfile)
{
	gchar* quality_string;
	if ((quality_string = xmlGetNoNsProp(options, BAD_CAST "video_quality"))[0] == '\0') {
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
		if ( strncmp(encoder, BAD_CAST " -e VP8", 7) == 0 ||
				strncmp(encoder, BAD_CAST " -e theora", 10) == 0) {
			goto good_quality;
		}
	}
	if ( 0 <= quality && quality <=51 ) {
		if ( strncmp(encoder, BAD_CAST " -e x264", 8) == 0) {
			goto good_quality;
		}
	}
	if ( 1 <= quality && quality <=31 ) {
		if ( strncmp(encoder, BAD_CAST " -e mpeg4", 9) == 0 ||
				strncmp(encoder, BAD_CAST " -e mpeg2", 9) == 0) {
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
 * @brief return handbrake option for audio encoder (-E)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space
 */
gchar* hb_audio_encoder(gchar* options)
{
	gchar* arg = g_strdup(" -E ");
	gchar* encoder = xmlGetNoNsProp(options, BAD_CAST "audio_encoder");
	if (strcmp(encoder, BAD_CAST "av_aac") == 0 ||
			strcmp(encoder, BAD_CAST "fdk_aac") == 0 ||
			strcmp(encoder, BAD_CAST "fdk_haac") == 0 ||
			strcmp(encoder, BAD_CAST "copy:aac") == 0 ||
			strcmp(encoder, BAD_CAST "ac3") == 0 ||
			strcmp(encoder, BAD_CAST "copy:ac3") == 0 ||
			strcmp(encoder, BAD_CAST "copy:dts") == 0 ||
			strcmp(encoder, BAD_CAST "copy:dtshd") == 0 ||
			strcmp(encoder, BAD_CAST "mp3") == 0 ||
			strcmp(encoder, BAD_CAST "copy:mp3") == 0 ||
			strcmp(encoder, BAD_CAST "vorbis") == 0 ||
			strcmp(encoder, BAD_CAST "flac16") == 0 ||
			strcmp(encoder, BAD_CAST "flac24") == 0 ||
			strcmp(encoder, BAD_CAST "copy") == 0 ) {
		arg = strncat(arg, encoder, strlen(encoder));
		g_free(encoder);
		return arg;
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options audio_encoder attribute\n");
		return NULL;
	}
}

/**
 * @brief return handbrake option for audio quality (-Q)
 *
 * @param options handbrake_options pointer
 * @param doc xml document, for error indicating
 *
 * @return command line option string starting with a space
 */
gchar* hb_audio_quality(gchar* options, GKeyFile* keyfile)
{
	gchar* arg = g_strdup(" -Q ");
	gchar* quality = xmlGetNoNsProp(options, BAD_CAST "audio_quality");
	if (quality[0] == '\0') {
		g_free(arg);
		g_free(quality);
		// Empty string lets Handbrake pick the default
		return g_strdup("");
	}
	gchar* codec = hb_audio_encoder(options);
	if (strcmp(codec, BAD_CAST " -E mp3") == 0
			|| strcmp(codec, BAD_CAST " -E vorbis") == 0) {
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
		arg = strncat(arg, BAD_CAST temp, 5);
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

/**
 * @brief return handbrake option for audio bitrate (-B)
 *
 * @param options handbrake_options pointer
 * @param doc xml document, for error indicating
 *
 * @return command line option string starting with a space
 */
gchar* hb_audio_bitrate(gchar* options, GKeyFile* keyfile)
{
	gchar* arg = g_strdup(" -B ");
	gchar* bitrate = xmlGetNoNsProp(options, BAD_CAST "audio_bitrate");
	if (bitrate[0] == '\0') {
		g_free(arg);
		g_free(bitrate);
		// Empty string lets Handbrake pick the default
		return g_strdup("");
	}
	int br = strtol((char *) bitrate, NULL, 10);
	gchar* codec = hb_audio_encoder(options);
	if ( ((strcmp(codec, BAD_CAST " -E mp3") == 0)
			|| (strcmp(codec, BAD_CAST " -E copy:mp3") == 0))
			&& valid_bit_rate(br, 32, 320)) {
		goto good_bitrate;
	}
	if ( ((strcmp(codec, BAD_CAST " -E av_aac") == 0)
				|| (strcmp(codec, BAD_CAST " -E copy:aac") == 0))
			&& valid_bit_rate(br, 192, 1536 )) {
		goto good_bitrate;
	}
	if ( (strcmp(codec, BAD_CAST " -E vorbis") == 0)
			&& valid_bit_rate(br, 192, 1344)) {
		goto good_bitrate;
	}
	if ( ((strcmp(codec, BAD_CAST " -E fdk_aac") == 0)
				|| (strcmp(codec, BAD_CAST " -E copy:dtshd") == 0)
				|| (strcmp(codec, BAD_CAST " -E copy") == 0)
				|| (strcmp(codec, BAD_CAST " -E copy:dts") == 0))
			&& valid_bit_rate(br, 160, 1344 )) {
		goto good_bitrate;
	}
	if ( ((strcmp(codec, BAD_CAST " -E ac3") == 0)
				|| ( strcmp(codec, BAD_CAST " -E copy:ac3") == 0))
			&& valid_bit_rate(br, 224, 640)) {
		goto good_bitrate;
	}
	if ( (strcmp(codec, BAD_CAST " -E fdk_haac") == 0)
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

/**
 * @brief return handbrake option for markers (-m)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space
 */
gchar* hb_markers(gchar* options)
{
	gchar* temp = xmlGetNoNsProp(options, BAD_CAST "markers");

	if (strcmp(temp, BAD_CAST "yes") == 0 ) {
		g_free(temp);
		return g_strdup(" -m");
	} else if (strcmp(temp, BAD_CAST "no") == 0 ) {
		g_free(temp);
		return g_strdup("");
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options markers attribute\n");
		g_free(temp);
		return NULL;
	}

}

/**
 * @brief return handbrake option for anamorphic (--strict-anamorphic or --loose-anamorphic)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space
 */
gchar* hb_anamorphic(gchar* options)
{
	gchar* temp = xmlGetNoNsProp(options, BAD_CAST "anamorphic");
	if (strcmp(temp, BAD_CAST "strict") == 0 ) {
		g_free(temp);
		return g_strdup(BAD_CAST " --strict-anamorphic");
	} else if (strcmp(temp, BAD_CAST "loose") == 0 ) {
		g_free(temp);
		return g_strdup(BAD_CAST " --loose-anamorphic");
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options anamorphic attribute\n");
		g_free(temp);
		return NULL;
	}
}

/**
 * @brief return handbrake option for deinterlacing (-d)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space
 */
gchar* hb_deinterlace(gchar* options)
{
	gchar* arg = g_strdup(" -d ");
	gchar* temp = xmlGetNoNsProp(options, BAD_CAST "deinterlace");
	
	if (strcmp(temp, BAD_CAST "fast") == 0 ||
		strcmp(temp, BAD_CAST "slow") == 0 ||
		strcmp(temp, BAD_CAST "slower") == 0 ||
		strcmp(temp, BAD_CAST "bob") == 0 ||
		strcmp(temp, BAD_CAST "default") == 0 ) {
		arg = strncat(arg, temp, strlen(temp));
		g_free(temp);
		return arg;
	} else if (strcmp(temp, BAD_CAST "none") == 0) {
		g_free(arg);
		g_free(temp);
		return g_strdup("");
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options deinterlace attribute\n");
		g_free(arg);
		g_free(temp);
		return NULL;
	}
}

/**
 * @brief return handbrake option for decombing (-5)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space
 */
gchar* hb_decomb(gchar* options)
{
	gchar* arg = g_strdup(" -5 ");
	gchar* temp = xmlGetNoNsProp(options, BAD_CAST "decomb");

	if (strcmp(temp, BAD_CAST "fast") == 0 ||
			strcmp(temp, BAD_CAST "bob") == 0 ||
			strcmp(temp, BAD_CAST "default") == 0 ) {
		arg = strncat(arg, temp, strlen(temp));
		g_free(temp);
		return arg;
	} else if (strcmp(temp, BAD_CAST "none") == 0) {
		g_free(arg);
		g_free(temp);
		return g_strdup("");
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options decomb attribute\n");
		g_free(arg);
		g_free(temp);
		return NULL;
	}
}

/**
 * @brief return handbrake option for denoising (-8)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space
 */
gchar* hb_denoise(gchar* options)
{
	gchar* arg = g_strdup(" -8 ");
	gchar* temp = xmlGetNoNsProp(options, BAD_CAST "denoise");

	if (strcmp(temp, BAD_CAST "ultralight") == 0 ||
		strcmp(temp, BAD_CAST "light") == 0 ||
		strcmp(temp, BAD_CAST "medium") == 0 ||
		strcmp(temp, BAD_CAST "strong") == 0 ||
		strcmp(temp, BAD_CAST "default") == 0 ) {
		arg = strncat(arg, temp, strlen(temp));
		g_free(temp);
		return arg;
	} else if (strcmp(temp, BAD_CAST "none") == 0) {
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
	while (cur && strcmp(cur->name, BAD_CAST "handbrake_options")) {
		cur = cur->next;
	}

	return xmlGetNoNsProp(cur, BAD_CAST "format");
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
	while (cur && strcmp(cur->name, BAD_CAST "handbrake_options")) {
		cur = cur->next;
	}

	return xmlGetNoNsProp(cur, BAD_CAST "input_basedir");
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
