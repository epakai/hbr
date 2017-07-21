/*
 * hbr-gen - handbrake runner file generator
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

#include <argp.h>                       // for argp_help, argp_parse, etc
#include <ctype.h>                      // for isdigit
#include <errno.h>                      // for errno
#include <stdio.h>                      // for NULL, stderr, stdout, etc
#include <stdlib.h>                     // for atoi, exit
#include <stdbool.h>
#include <unistd.h>                     // for R_OK, W_OK, X_OK, F_OK
#include <libxml/xpath.h>
#include "gen_xml.h"                    // for gen_arguments, gen_argp, etc
#include "xml.h"                        // parse_xml, outfile_count, etc

// Set to enable debug output of commands to be run
bool debug = false;

/*
 * Argument handling setup for argp
 */
const char *argp_program_version = ""; //TODO pull version from one place
const char *argp_program_bug_address = "<https://github.com/epakai/hbr/issues>";
static const char gen_doc[] = "handbrake runner -- generates a xml template "
"with NUM outfile sections, or one outfile per line in an episode list.";
static const char gen_args_doc[] = "{-n NUM|-l FILE}";

static const struct argp_option gen_options[] = {
	{"count",    'n', "NUM",          0, "Number of outfile sections to generate", 1},
	{"episodes", 'l', "FILE",         0, "Episode list", 1},
	{"format",   'f', "mkv|mp4",      0, "Output container format", 2},
	{"title",    't', "NUM",          0, "DVD Title number (for discs with a single title)", 2},
	{"source",   'S', "FILE",         0, "Source filename (DVD iso file)", 2},
	{"crop",     'c', "T:B:L:R",      0, "Pixels to crop, top:bottom:left:right", 2},
	{"type",     'p', "series|movie", 0, "Type of video", 3},
	{"year",     'y', "YEAR",         0, "Movie Release year", 3},
	{"name",     'N', "Name",         0, "Movie or series name", 3},
	{"season",   's', "NUM",          0, "Series season", 3},
	{"basedir",  'b', "PATH",         0, "Base directory for input files", 3},
	{"markers",  'm', 0,              0, "Add chapter markers", 3},
	{"help",     '?', 0,              0, "Give this help list", -1 },
	{"usage",    '@', 0,              OPTION_HIDDEN, "Give this help list", -1 },
	{ 0, 0, 0, 0, 0, 0 }
};

/**
 * @brief Arguments related to xml generation. Handled by argp.
 */
struct gen_arguments {
	int generate;    /**< Number of outfile sections to generate. */
	int title;       /**< DVD title number. */
	int season;      /**< Season number. */
	int video_type;  /**< Type of video (series or movie). */
	bool markers;    /**< Toggle chapter markers*/
	char *source;    /**< Source filename. */
	char *year;      /**< Release year. */
	char *crop;      /**< Crop amount in pixels formatted as top:bottom:left:right */
	char *name;      /**< Movie or series name. */
	char *format;    /**< Output container (mkv or mp4). */
	char *basedir;   /**< Base directory for input files. */
	char *episodes;  /**< Filename for list of episodes. */
};

// Function prototype
static error_t parse_gen_opt(int token, char *arg, struct argp_state *state);

static const struct argp gen_argp = {gen_options, parse_gen_opt, gen_args_doc,
	gen_doc, NULL, 0, 0};
/**
 * @brief
 *
 * @param argc
 * @param argv[]
 *
 * @return
 */
int main(int argc, char * argv[])
{
	struct gen_arguments gen_arguments = {1, 0, 0, -1, 0,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL};

	argp_parse(&gen_argp, argc, argv, ARGP_NO_HELP, 0, &gen_arguments);

	struct crop crop_arg = get_crop(BAD_CAST gen_arguments.crop);
	xmlDocPtr doc = gen_xml(gen_arguments.generate, gen_arguments.title ?: 1,
			gen_arguments.season ?: 1, gen_arguments.video_type,
			gen_arguments.markers, gen_arguments.source ?: "",
			gen_arguments.year ?: "", crop_arg,
			gen_arguments.name ?: "", gen_arguments.format ?: "mkv",
			gen_arguments.basedir ?: "", gen_arguments.episodes);
	if (doc != NULL) {
		print_xml(doc);
	} else {
		fprintf(stderr, "XML generation failed\n");
	}
	return 0;
}

/**
 * @brief Argp Parse options for xml generation.
 *
 * @param token Command line token associated with each argument.
 * @param arg Value for the token being passed.
 * @param state Argp state for the parser
 *
 * @return Error status.
 */
error_t parse_gen_opt(int token, char *arg, struct argp_state *state)
{
	struct gen_arguments *gen_arguments = (struct gen_arguments *) state->input;
	switch (token) {
		case '@':
		case '?':
			//TODO maybe pull executable name from somewhere else
			argp_help(&gen_argp, stdout, ARGP_HELP_SHORT_USAGE, "hbr-gen");
			argp_help(&gen_argp, stdout, ARGP_HELP_PRE_DOC, "hbr-gen");
			argp_help(&gen_argp, stdout, ARGP_HELP_LONG, "hbr-gen");
			printf("Report bugs to %s\n", argp_program_bug_address);
			exit(0);
			break;
		case 'l':
			gen_arguments->episodes = arg;
			gen_arguments->generate = 1;
			break;
		case 'n':
			if (arg != NULL) {
				if ( atoi(arg) > 0 ) {
					gen_arguments->generate = atoi(arg);
				}
			} else {
				gen_arguments->generate = 1;
			}
			break;
		case 'f':
			if (strncmp(arg, "mkv", 3) == 0) {
				gen_arguments->format = arg;
			}
			if (strncmp(arg, "mp4", 3) == 0) {
				gen_arguments->format = arg;
			}
			break;
		case 'S':
			gen_arguments->source = arg;
			break;
		case 'p':
			if (strncmp(arg, "movie", 5) == 0) {
				gen_arguments->video_type = 0;
			}
			if (strncmp(arg, "series", 6) == 0) {
				gen_arguments->video_type = 1;
			}
			break;
		case 't':
			if ( atoi(arg) >= 1  && atoi(arg) <= 99 ) {
				gen_arguments->title = atoi(arg);
			}
			break;
		case 'y':
			gen_arguments->year = arg;
			break;
		case 'c':
			gen_arguments->crop = arg;
			break;
		case 'N':
			gen_arguments->name = arg;
			break;
		case 's':
			if ( atoi(arg) > 0 ) {
				gen_arguments->season = atoi(arg);
			}
			break;
		case 'b':
			gen_arguments->basedir = arg;
			break;
		case 'm':
			gen_arguments->markers =  true;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
