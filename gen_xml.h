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

#ifndef _gen_xml_h
#define _gen_xml_h

#include <argp.h>
#include <stdlib.h>
#include <stdbool.h>

static const char gen_doc[] = "handbrake runner -- generates a xml template "
"with NUM outfile sections, or one outfile per line in an episode list.";
static const char gen_args_doc[] = "{-g NUM|-l FILE}";

static const struct argp_option gen_options[] = {
	{"episodes", 'l', "FILE",         0, "Episode list", 1},
	{"generate", 'g', "NUM",          0,
		"Generate xml file with NUM outfile sections.\nMust be specified unless -l is provided.", 0 },
	{"format",   'f', "mkv|mp4",      0, "Output container format", 0},
	{"title",    't', "NUM",          0, "DVD Title number (for discs with a single title)", 0},
	{"type",     'p', "series|movie", 0, "Type of video", 1},
	{"source",   's', "FILE",         0, "Source filename (DVD iso file)", 0},
	{"year",     'y', "YEAR",         0, "Movie Release year", 1},
	{"crop",     'c', "T:B:L:R",      0, "Pixels to crop, top:bottom:left:right", 0},
	{"name",     'n', "Name",         0, "Movie or series name", 1},
	{"season",   'e', "NUM",          0, "Series season", 1},
	{"basedir",  'b', "PATH",         0, "Base directory for input files", 1},
	{"markers",  'm', 0,              0, "Add chapter markers", 1},
	{"help",     '?', 0,      OPTION_HIDDEN, "", 0 },
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

/**
 * @brief Episode structure used to build episode list when -l option is used.
 */
struct episode {
	int number;      /**< Episode number. */
	char * name;     /**< Episode name. */
};

/* parse options related to xml generation */
error_t parse_gen_opt(int key, char *arg, struct argp_state *);

/* Read an episode list into memory */
int read_episode_list(const char *episode_filename, struct episode **episode_array);

/* iterate over the episode list freeing each struct and the list itelf */
void free_episode_array(struct episode *episode_array, int count);

/* Generate xml skeleton for hbr */
void gen_xml(int outfiles_count, int title, int season, int video_type,
		bool markers, const char *source, const char *year,
		const char *crop, const char *name, const char *format,
		const char *basedir, const char *episodes);

static const struct argp gen_argp = {gen_options, parse_gen_opt, gen_args_doc,
	gen_doc, NULL, 0, 0};

#endif
