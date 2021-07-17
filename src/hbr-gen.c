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

#include <argp.h>    // for argp_help, argp_parse, error_t, ARGP_ERR_UNKNOWN
#include <stdio.h>   // for NULL, printf, stdout
#include <stdlib.h>  // for atoi, exit

#include "util.h"
#include "gen_hbr.h"

/// argp version info
const char *argp_program_version = ""; //TODO pull version from one place
/// argp bug reporting info
const char *argp_program_bug_address = "<https://github.com/epakai/hbr/issues>";
/// hbr-gen description for --help
static const char gen_doc[] = "handbrake runner generator -- generates a hbr template "
"with NUM outfile sections, or one outfile per line in an episode list.";
/// usage documentation to specify required options
static const char gen_args_doc[] = "{-n NUM|-l FILE}";

/// Command line options for argp parser
static const struct argp_option gen_options[] = {
    {"count",          'n', "NUM",          0, "Number of outfile sections to generate", 1},
    {"episodes",       'l', "FILE",         0, "Episode list", 1},
    {"title",          't', "NUM",          0, "DVD Title number", 2},
    {"source",         'f', "FILE",         0, "Source filename", 2},
    {"crop",           'c', "T:B:L:R",      0, "Pixels to crop, top:bottom:left:right", 2},
    {"type",           'p', "series|movie", 0, "Type of video", 3},
    {"year",           'y', "YEAR",         0, "Movie Release year", 3},
    {"name",           'N', "NAME",         0, "Movie or series name", 3},
    {"season",         'S', "NUM",          0, "Series season", 3},
    {"input-basedir",  'i', "PATH",         0, "Base directory for input files", 3},
    {"output-basedir", 'o', "PATH",         0, "Base directory for input files", 3},
    {"audio",          'a', "AUDIO",        0, "Comma-separated audio track list", 3},
    {"subtitle",       's', "SUBTITLE",     0, "Comma-separated subtitle track list", 3},
    {"chapters",       'C', "CHAPTERS",     0, "Chapter range", 3},
    {"help",           '?', NULL,              0, "Give this help list", -1 },
    {"usage",          '@', NULL,              OPTION_HIDDEN, "Give this help list", -1 },
    { NULL, 0, NULL, 0, NULL, 0 }
};

/**
 * @brief Arguments related to xml generation. Handled by argp.
 */
struct gen_arguments {
    int generate;         // Number of outfile sections to generate.
    int title;            // DVD title number.
    int season;           // Season number.
    char *type;           // Type of video (series or movie).
    char *iso_filename;   // Source filename.
    char *year;           // Release year.
    char *crop;           // Crop amount in pixels formatted as top:bottom:left:right
    char *name;           // Movie or series name.
    char *input_basedir;  // Base directory for input files.
    char *output_basedir; // Base directory for input files.
    char *audio;          // Audio track list (comma-separated)
    char *subtitle;       // Subtitle track list (comma-separated)
    char *chapters;       // Chapter range (i.e. 2-18)
    char *episodes;       // Filename for list of episodes.
};

// Function prototype
static error_t parse_gen_opt(int token, char *arg, struct argp_state *state);

/// argp struct of all parsing/help info
static const struct argp gen_argp = {gen_options, parse_gen_opt, gen_args_doc,
    gen_doc, NULL, NULL, NULL};
/**
 * @brief Produces a hbr template based on arguments or an episode list
 *
 * @param argc   argument count
 * @param argv[] argument array
 *
 * @return 0 - success
 */
int main(int argc, char * argv[])
{
    struct gen_arguments gen_arguments = {1, 0, 0,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    argp_parse(&gen_argp, argc, argv, ARGP_NO_HELP, NULL, &gen_arguments);

    GKeyFile *config = gen_hbr(gen_arguments.generate, gen_arguments.title,
        gen_arguments.season, gen_arguments.type,
        gen_arguments.iso_filename, gen_arguments.year, gen_arguments.crop,
        gen_arguments.name, gen_arguments.input_basedir,
        gen_arguments.output_basedir, gen_arguments.audio,
        gen_arguments.subtitle, gen_arguments.chapters,
        gen_arguments.episodes);
    if (config != NULL) {
    print_hbr(config);
    g_key_file_free(config);
    } else {
    hbr_error("hbr file generation failed", NULL, NULL, NULL, NULL);
    }
    return 0;
}

/**
 * @brief Argp Parse options for hbr generation.
 *
 * @param token Command line token associated with each argument.
 * @param arg Value for the token being passed.
 * @param state Argp state for the parser
 *
 * @return Error status.
 */
static error_t parse_gen_opt(int token, char *arg, struct argp_state *state)
{
    struct gen_arguments *gen_arguments = (struct gen_arguments *) state->input;
    switch (token) {
    case '@':
    case '?':
        //TODO maybe pull executable name from somewhere else
        argp_help(&gen_argp, stdout, ARGP_HELP_SHORT_USAGE, (char *)"hbr-gen");
        argp_help(&gen_argp, stdout, ARGP_HELP_PRE_DOC, (char *)"hbr-gen");
        argp_help(&gen_argp, stdout, ARGP_HELP_LONG, (char *)"hbr-gen");
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
        gen_arguments->iso_filename = arg;
        break;
    case 'p':
        gen_arguments->type = arg;
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
    case 'S':
        if ( atoi(arg) > 0 ) {
        gen_arguments->season = atoi(arg);
        }
        break;
    case 'i':
        gen_arguments->input_basedir = arg;
        break;
    case 'o':
        gen_arguments->output_basedir = arg;
        break;
    case 'a':
        gen_arguments->audio = arg;
        break;
    case 's':
        gen_arguments->subtitle = arg;
        break;
    case 'C':
        gen_arguments->chapters = arg;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}
