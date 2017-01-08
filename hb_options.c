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
xmlChar* hb_options_string(xmlDocPtr doc)
{
	xmlNode *root_element = xmlDocGetRootElement(doc);
	// Find the handbrake_options element
	xmlNode *cur = root_element->children;
	while (cur && xmlStrcmp(cur->name, (const xmlChar *) "handbrake_options")) {
		cur = cur->next;
	}

	// allocate and initialize options string
	xmlChar *opt_str = xmlCharStrdup("\0");
	xmlChar *options[11];

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
		opt_str = xmlStrcat(opt_str, (const xmlChar *) options[i]);
		xmlFree(options[i]);
	}

	opt_str = xmlStrncat(opt_str, (const xmlChar *) " ", 1);
	return opt_str;
}

/**
 * @brief return handbrake option for file format (-f)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space, NULL on failure
 */
xmlChar* hb_format(xmlNode *options)
{
	xmlChar *temp = xmlGetNoNsProp(options, (const xmlChar *) "format");
	// format is constrained by the XML DTD to be "mkv" or "mp4"
	if (xmlStrcmp(temp, (const xmlChar *) "mkv") == 0){
		xmlFree(temp);
		return xmlCharStrdup(" -f av_mkv");
	} else if (xmlStrcmp(temp, (const xmlChar *) "mp4") == 0){
		xmlFree(temp);
		return xmlCharStrdup(" -f av_mp4");
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
xmlChar* hb_video_encoder(xmlNode *options)
{
	xmlChar *arg = xmlCharStrdup(" -e ");
	xmlChar *temp = xmlGetNoNsProp(options, (const xmlChar *) "video_encoder");
	if (xmlStrcmp(temp, (const xmlChar *) "x264") == 0 ||
			xmlStrcmp(temp, (const xmlChar *) "mpeg4") == 0 ||
			xmlStrcmp(temp, (const xmlChar *) "mpeg2") == 0 ||
			xmlStrcmp(temp, (const xmlChar *) "VP8") == 0 ||
			xmlStrcmp(temp, (const xmlChar *) "theora") == 0 ) {
		arg = xmlStrncat(arg, temp, xmlStrlen(temp));
		xmlFree(temp);
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
xmlChar* hb_video_quality(xmlNode *options, xmlDocPtr doc)
{
	xmlChar *quality_string;
	if ((quality_string = xmlGetNoNsProp(options, (const xmlChar *) "video_quality"))[0] == '\0') {
		xmlFree(quality_string);
		// Empty string lets Handbrake pick the default
		return xmlCharStrdup("");
	}
	
	xmlChar *encoder = hb_video_encoder(options);
	xmlChar *temp = xmlCharStrdup(" -q ");

	// test for max of two digits
	if ( xmlStrlen(quality_string) > 2
			|| !isdigit(quality_string[0])
			|| !isdigit(quality_string[1])) {
		goto invalid_quality;
	}
	long quality = strtol((char *) quality_string, NULL, 10);
	if ( 0 <= quality && quality <=63 ) {
		if ( xmlStrncmp(encoder, (const xmlChar *) " -e VP8", 7) == 0 ||
				xmlStrncmp(encoder, (const xmlChar *) " -e theora", 10) == 0) {
			goto good_quality;
		}
	}
	if ( 0 <= quality && quality <=51 ) {
		if ( xmlStrncmp(encoder, (const xmlChar *) " -e x264", 8) == 0) {
			goto good_quality;
		}
	}
	if ( 1 <= quality && quality <=31 ) {
		if ( xmlStrncmp(encoder, (const xmlChar *) " -e mpeg4", 9) == 0 ||
				xmlStrncmp(encoder, (const xmlChar *) " -e mpeg2", 9) == 0) {
			goto good_quality;
		}
	}
invalid_quality:
	fprintf(stderr, "Invalid video quality '%s' for %s encoder in \"%s\""
			"line number: ~%ld\n",
			quality_string, encoder, doc->URL, xmlGetLineNo(options)-9);
	xmlFree(quality_string);
	xmlFree(encoder);
	xmlFree(temp);
	return NULL;
good_quality:
	temp = xmlStrcat(temp, quality_string);
	xmlFree(quality_string);
	xmlFree(encoder);
	return temp;
}

/**
 * @brief return handbrake option for audio encoder (-E)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space
 */
xmlChar* hb_audio_encoder(xmlNode *options)
{
	xmlChar *arg = xmlCharStrdup(" -E ");
	xmlChar *encoder = xmlGetNoNsProp(options, (const xmlChar *) "audio_encoder");
	if (xmlStrcmp(encoder, (const xmlChar *) "av_aac") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "fdk_aac") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "fdk_haac") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "copy:aac") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "ac3") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "copy:ac3") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "copy:dts") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "copy:dtshd") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "mp3") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "copy:mp3") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "vorbis") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "flac16") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "flac24") == 0 ||
			xmlStrcmp(encoder, (const xmlChar *) "copy") == 0 ) {
		arg = xmlStrncat(arg, encoder, xmlStrlen(encoder));
		xmlFree(encoder);
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
xmlChar* hb_audio_quality(xmlNode *options, xmlDocPtr doc)
{
	xmlChar *arg = xmlCharStrdup(" -Q ");
	xmlChar *quality = xmlGetNoNsProp(options, (const xmlChar *) "audio_quality");
	if (quality[0] == '\0') {
		xmlFree(arg);
		xmlFree(quality);
		// Empty string lets Handbrake pick the default
		return xmlCharStrdup("");
	}
	xmlChar *codec = hb_audio_encoder(options);
	if (xmlStrcmp(codec, (const xmlChar *) " -E mp3") == 0
			|| xmlStrcmp(codec, (const xmlChar *) " -E vorbis") == 0) {
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
		arg = xmlStrncat(arg, (const xmlChar *) temp, 5);
		xmlFree(quality);
		xmlFree(codec);
		return arg;
	} else {
invalid_bitrate:
		fprintf(stderr,
				"Invalid quality'%s' for %s codec in \"%s\" line number: ~%ld\n",
				quality, xmlStrsub(codec, 4, 7), doc->URL, xmlGetLineNo(options)-7);
		xmlFree(arg);
		xmlFree(quality);
		xmlFree(codec);
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
xmlChar* hb_audio_bitrate(xmlNode *options, xmlDocPtr doc)
{
	xmlChar *arg = xmlCharStrdup(" -B ");
	xmlChar *bitrate = xmlGetNoNsProp(options, (const xmlChar *) "audio_bitrate");
	if (bitrate[0] == '\0') {
		xmlFree(arg);
		xmlFree(bitrate);
		// Empty string lets Handbrake pick the default
		return xmlCharStrdup("");
	}
	int br = strtol((char *) bitrate, NULL, 10);
	xmlChar *codec = hb_audio_encoder(options);
	if ( ((xmlStrcmp(codec, (const xmlChar *) " -E mp3") == 0)
			|| (xmlStrcmp(codec, (const xmlChar *) " -E copy:mp3") == 0))
			&& valid_bit_rate(br, 32, 320)) {
		goto good_bitrate;
	}
	if ( ((xmlStrcmp(codec, (const xmlChar *) " -E av_aac") == 0)
				|| (xmlStrcmp(codec, (const xmlChar *) " -E copy:aac") == 0))
			&& valid_bit_rate(br, 192, 1536 )) {
		goto good_bitrate;
	}
	if ( (xmlStrcmp(codec, (const xmlChar *) " -E vorbis") == 0)
			&& valid_bit_rate(br, 192, 1344)) {
		goto good_bitrate;
	}
	if ( ((xmlStrcmp(codec, (const xmlChar *) " -E fdk_aac") == 0)
				|| (xmlStrcmp(codec, (const xmlChar *) " -E copy:dtshd") == 0)
				|| (xmlStrcmp(codec, (const xmlChar *) " -E copy") == 0)
				|| (xmlStrcmp(codec, (const xmlChar *) " -E copy:dts") == 0))
			&& valid_bit_rate(br, 160, 1344 )) {
		goto good_bitrate;
	}
	if ( ((xmlStrcmp(codec, (const xmlChar *) " -E ac3") == 0)
				|| ( xmlStrcmp(codec, (const xmlChar *) " -E copy:ac3") == 0))
			&& valid_bit_rate(br, 224, 640)) {
		goto good_bitrate;
	}
	if ( (xmlStrcmp(codec, (const xmlChar *) " -E fdk_haac") == 0)
			&& valid_bit_rate(br, 80, 256)) {
		goto good_bitrate;
	}

	fprintf(stderr,
			"Invalid bitrate '%s' for %s codec in \"%s\" line number: ~%ld\n",
			bitrate, xmlStrsub(codec, 4, 11), doc->URL, xmlGetLineNo(options)-7);
	xmlFree(bitrate);
	xmlFree(codec);
	xmlFree(arg);
	return NULL;
good_bitrate:
	arg = xmlStrncat(arg, bitrate, xmlStrlen(bitrate));
	xmlFree(bitrate);
	xmlFree(codec);
	return arg;
}

/**
 * @brief return handbrake option for markers (-m)
 *
 * @param options handbrake_options pointer
 *
 * @return command line option string starting with a space
 */
xmlChar* hb_markers(xmlNode *options)
{
	xmlChar *temp = xmlGetNoNsProp(options, (const xmlChar *) "markers");

	if (xmlStrcmp(temp, (const xmlChar *) "yes") == 0 ) {
		xmlFree(temp);
		return xmlCharStrdup(" -m");
	} else if (xmlStrcmp(temp, (const xmlChar *) "no") == 0 ) {
		xmlFree(temp);
		return xmlCharStrdup("");
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options markers attribute\n");
		xmlFree(temp);
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
xmlChar* hb_anamorphic(xmlNode *options)
{
	xmlChar *temp = xmlGetNoNsProp(options, (const xmlChar *) "anamorphic");
	if (xmlStrcmp(temp, (const xmlChar *) "strict") == 0 ) {
		xmlFree(temp);
		return xmlStrdup((const xmlChar *) " --strict-anamorphic");
	} else if (xmlStrcmp(temp, (const xmlChar *) "loose") == 0 ) {
		xmlFree(temp);
		return xmlStrdup((const xmlChar *) " --loose-anamorphic");
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options anamorphic attribute\n");
		xmlFree(temp);
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
xmlChar* hb_deinterlace(xmlNode *options)
{
	xmlChar *arg = xmlCharStrdup(" -d ");
	xmlChar *temp = xmlGetNoNsProp(options, (const xmlChar *) "deinterlace");
	
	if (xmlStrcmp(temp, (const xmlChar *) "fast") == 0 ||
		xmlStrcmp(temp, (const xmlChar *) "slow") == 0 ||
		xmlStrcmp(temp, (const xmlChar *) "slower") == 0 ||
		xmlStrcmp(temp, (const xmlChar *) "bob") == 0 ||
		xmlStrcmp(temp, (const xmlChar *) "default") == 0 ) {
		arg = xmlStrncat(arg, temp, xmlStrlen(temp));
		xmlFree(temp);
		return arg;
	} else if (xmlStrcmp(temp, (const xmlChar *) "none") == 0) {
		xmlFree(arg);
		xmlFree(temp);
		return xmlCharStrdup("");
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options deinterlace attribute\n");
		xmlFree(arg);
		xmlFree(temp);
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
xmlChar* hb_decomb(xmlNode *options)
{
	xmlChar *arg = xmlCharStrdup(" -5 ");
	xmlChar *temp = xmlGetNoNsProp(options, (const xmlChar *) "decomb");

	if (xmlStrcmp(temp, (const xmlChar *) "fast") == 0 ||
			xmlStrcmp(temp, (const xmlChar *) "bob") == 0 ||
			xmlStrcmp(temp, (const xmlChar *) "default") == 0 ) {
		arg = xmlStrncat(arg, temp, xmlStrlen(temp));
		xmlFree(temp);
		return arg;
	} else if (xmlStrcmp(temp, (const xmlChar *) "none") == 0) {
		xmlFree(arg);
		xmlFree(temp);
		return xmlCharStrdup("");
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options decomb attribute\n");
		xmlFree(arg);
		xmlFree(temp);
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
xmlChar* hb_denoise(xmlNode *options)
{
	xmlChar *arg = xmlCharStrdup(" -8 ");
	xmlChar *temp = xmlGetNoNsProp(options, (const xmlChar *) "denoise");

	if (xmlStrcmp(temp, (const xmlChar *) "ultralight") == 0 ||
		xmlStrcmp(temp, (const xmlChar *) "light") == 0 ||
		xmlStrcmp(temp, (const xmlChar *) "medium") == 0 ||
		xmlStrcmp(temp, (const xmlChar *) "strong") == 0 ||
		xmlStrcmp(temp, (const xmlChar *) "default") == 0 ) {
		arg = xmlStrncat(arg, temp, xmlStrlen(temp));
		xmlFree(temp);
		return arg;
	} else if (xmlStrcmp(temp, (const xmlChar *) "none") == 0) {
		xmlFree(arg);
		xmlFree(temp);
		return xmlCharStrdup("");
	} else {
		// This condition shouldn't be reached unless the DTD is modified
		fprintf(stderr, "Invalid handbrake_options denoise attribute\n");
		xmlFree(arg);
		xmlFree(temp);
		return NULL;
	}
}

/**
 * @brief return string for file format (mp4 or mkv)
 *
 * @param doc XML document to read values from
 *
 * @return either "mp4" or "mkv", caller must xmlFree()
 */

xmlChar* get_format(xmlDocPtr doc)
{
	xmlNode *root_element = xmlDocGetRootElement(doc);
	// Find the handbrake_options element
	xmlNode *cur = root_element->children;
	while (cur && xmlStrcmp(cur->name, (const xmlChar *) "handbrake_options")) {
		cur = cur->next;
	}

	return xmlGetNoNsProp(cur, (const xmlChar *) "format");
}

/**
 * @brief return string of the base directory for input files
 *
 * @param doc XML document to read values from
 *
 * @return path for input files, caller must xmlFree()
 */
xmlChar* get_input_basedir(xmlDocPtr doc)
{
	xmlNode *root_element = xmlDocGetRootElement(doc);
	// Find the handbrake_options element
	xmlNode *cur = root_element->children;
	while (cur && xmlStrcmp(cur->name, (const xmlChar *) "handbrake_options")) {
		cur = cur->next;
	}

	return xmlGetNoNsProp(cur, (const xmlChar *) "input_basedir");
}

/**
 * @brief Ensure bitrate is one of the preset values
 *
 * @param bitrate bitrate for audio
 * @param minimum set range for valid bitrate (depending on codec)
 * @param maximum set range for valid bitrate (depending on codec)
 *
 * @return boolean status, 1 is valid, 0 is invalid
 */
int valid_bit_rate(int bitrate, int minimum, int maximum)
{
	// list is slightly reordered to put common rates first
	int valid_bitrates[] = { 128, 160, 192, 224, 256, 320,
		384, 448, 512, 40, 48, 56, 64, 80, 96, 112, 576, 640,
		768, 960, 1152, 1344, 1536, 2304, 3072, 4608, 6144 };
	int i;
	for (i=0; i<27; i++) {
		if (bitrate == valid_bitrates[i]){
			if (bitrate > minimum && bitrate < maximum) {
				return 1; //valid
			}
		}
	}
	return 0;
}
