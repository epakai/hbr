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

#include <argp.h>                       // for argp_help, argp_parse, etc
#include <ctype.h>                      // for isdigit
#include <errno.h>                      // for errno
#include <stdio.h>                      // for NULL, stderr, stdout, etc
#include <stdlib.h>                     // for atoi, exit
#include <stdbool.h>
#include <glib.h>
#include <unistd.h>                     // for R_OK, W_OK, X_OK, F_OK
#include "keyfile.h"
#include "hb_options.h"
#include "out_options.h"

/**
 * @brief argp info for help/usage output (-?/--help)
 */
const char *argp_program_version = ""; //TODO pull version from one place
const char *argp_program_bug_address = "<https://github.com/epakai/hbr/issues>";
static char enc_doc[] = "handbrake runner -- runs handbrake with setting from an "
"key-value pair file with all encoded files placed in current directory";
static char enc_args_doc[] = "<XML FILE>";
static struct argp_option enc_options[] = {
	{"debug",     'd', 0,     0, "print the commands to be run instead of executing", 1},
	{"episode",   'e', "NUM", 0, "only encodes for entry with matching episode number", 1},
	{"preview",   'p', 0,     0, "generate a preview image for each output file", 1},
	{"overwrite", 'y', 0,     0, "overwrite encoded files without confirmation", 1},
	{"version",   'V', 0,     0, "prints program version", 1},
	{"help",      '?', 0,     0, "Give this help list", -1},
	{"usage",     '@', 0,     OPTION_HIDDEN, "Give this help list", -1},
	{ 0, 0, 0, 0, 0, 0 }
};

/*
 * PROTOTYPES
 */
struct config fetch_or_generate_config();
void encode_loop(GKeyFile* keyfile, gchar *hb_options);
static error_t parse_enc_opt (int token, char *arg, struct argp_state *);
void generate_thumbnail(gchar *filename, int outfile_count, int total_outfiles);
int call_handbrake(gchar *hb_command, int out_count, bool overwrite);
int hb_fork(gchar *hb_command, gchar *log_filename, int out_count);

/**
 * @brief Arguments for hbr. Handled by argp.
 */
struct enc_arguments {
	char *args[1];    /**< Single argument for the input file. */
	int episode;      /**< Specifies a particular episode number to be encoded. */
	bool overwrite;   /**< Default to overwriting previous encodes. */
	bool debug;       /**< Print commands instead of executing. */
	int preview;      /**< Generate preview image using ffmpegthumbnailer. */
} enc_arguments;

static struct argp enc_argp = {enc_options, parse_enc_opt, enc_args_doc,
	enc_doc, NULL, 0, 0};

/**
 * @brief Reads the input file and calls handbrake for each specified outfile
 *
 * @param argc Command line argument count
 * @param argv[] Arguments to be parsed by argp
 *
 * @return 0 - success, 1 - failure
 */
int main(int argc, char * argv[])
{
	enc_arguments.episode = -1;
	enc_arguments.overwrite = false;
	enc_arguments.debug = false;
	enc_arguments.preview = 0;

    // parse hbr config
    struct config global_config = fetch_or_generate_config();

	// parse normal options
	argp_parse (&enc_argp, argc, argv, ARGP_NO_HELP, 0, &enc_arguments);
	
	// parse input file
	GKeyFile* keyfile = parse_key_file(enc_arguments.args[0]);
	if (keyfile == NULL) {
        free_config(global_config);
		return 1;
	}

	//assemble call to HandBrakeCLI
	gchar *hb_options = NULL;
	hb_options = hb_options_string(global_config, keyfile);
	if ( hb_options == NULL){
        free_config(global_config);
        free_config(local_config);
		g_free(hb_options);
		g_free(keyfile);
		return 1;
	}

    // encode each outfile
	encode_loop(keyfile, hb_options);

    // clean up and exit
    free_config(global_config);
    free_config(local_config);
	g_free(keyfile);
	g_free(hb_options);
	return 0;
}

struct config fetch_or_generate_config()
{
    //TODO
    // if $XDG_CONFIG_HOME set
    // try $XDG_CONFIG_HOME/hbr/hbr.conf
    // try $HOME/.config/hbr/hbr.conf
    //
    // if found
    // keyfile = parse_key_file(path)
    // return get_global_config(keyfile);
    // else
    // create default config
    // try to write default config
    // if write fails print error and continue with default config in memory
    // return struct config
}

/**
 * @brief Loops through each encode or the specified encode
 *
 * @param keyfile Document to read outfile specifications from
 * @param hb_options general option string for HandBrakeCLI
 */
void encode_loop(GKeyFile* keyfile, gchar *hb_options) {
	// loop for each out_file tag in keyfile
	int out_count = outfile_count(keyfile);
	if (out_count < 1) {
		fprintf(stderr, "No valid outfiles found in \"%s\"\n", keyfile->URL);
	}
	int i = 1; // start at outfile 1
	// Handle -e option to encode a single episode
	if (enc_arguments.episode >= 0) {
		// adjust parameters for following loop so it runs once
		int outfile_number = get_outfile_from_episode(keyfile, enc_arguments.episode);
		i = outfile_number;
		out_count = outfile_number;
	}
	// encode all the episodes if loop parameters weren't modified above
	for (; i <= out_count; i++) {
		gchar *out_options = out_options_string(keyfile, i);
		if ( out_options == NULL){
			xmlNode* outfile_node = get_outfile(keyfile, i);
			fprintf(stderr,
					"%d: Bad outfile element in \"%s\" line number: "
					"%ld  was not encoded\n",
					i, keyfile->URL, xmlGetLineNo(outfile_node));
			g_free(out_options);
			continue;
		}
		// build full HandBrakeCLI command
		gchar *hb_command = g_strdup("HandBrakeCLI");
		hb_command = strcat(hb_command, hb_options);
		hb_command = strcat(hb_command, out_options);

		const gchar *filename_start = strstr(hb_command, "-o")+4;
		const gchar *filename_end = strstr(filename_start, "\"");
		gchar *filename = xmlStrsub(filename_start, 0, filename_end-filename_start);
		
		// output current encode information
		printf("%c[1m", 27);
		printf("Encoding: %d/%d: %s\n", i, out_count, filename);
		printf("%c[0m", 27);
		if (enc_arguments.debug) {
			// print full handbrake command
			printf("%s\n", hb_command);
		} else {
			if (call_handbrake(hb_command, i, enc_arguments.overwrite) == -1) {
				xmlNode* outfile_node = get_outfile(keyfile, i);
				fprintf(stderr,
						"%d: Handbrake call failed in \"%s\" line number: "
						"%ld  was not encoded\n",
						i, keyfile->URL, xmlGetLineNo(outfile_node));
				g_free(out_options);
				g_free(hb_command);
				continue;
			}
		}
		// produce a thumbnail
		if (enc_arguments.preview) {
			generate_thumbnail(filename, i, out_count);
		}
		g_free(filename);
		g_free(out_options);
		g_free(hb_command);
	}
}

/**
 * @brief Argp Parse options for general hbr use
 *
 * @param token Command line token for each argument.
 * @param arg Value for the token being passed.
 * @param state Argp state for the parser
 *
 * @return Error status.
 */
static error_t parse_enc_opt (int token, char *arg, struct argp_state *state)
{
	struct enc_arguments *enc_arguments = (struct enc_arguments *) state->input;
	switch (token) {
		case '@': // --usage
		case '?':
			//TODO maybe pull executable name from somewhere else
			argp_help(&enc_argp, stdout, ARGP_HELP_SHORT_USAGE, "hbr");
			argp_help(&enc_argp, stdout, ARGP_HELP_PRE_DOC, "hbr");
			argp_help(&enc_argp, stdout, ARGP_HELP_LONG, "hbr");
			printf("Report bugs to %s\n", argp_program_bug_address);
			exit(0);
			break;
		case 'd':
			enc_arguments->debug = true;
			break;
		case 'e':
			enc_arguments->episode = atoi(arg);
			break;
		case 'y':
			enc_arguments->overwrite =  true;
			break;
		case 'V':
			printf("%s\n", argp_program_version);
			exit(0);
			break;
		case 'p':
			enc_arguments->preview =  1;
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num >= 1) {
				/* Too many arguments. */
				argp_usage (state);
			}
			enc_arguments->args[state->arg_num] = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 1) {
				/* Not enough arguments. */
				argp_usage (state);
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/**
 * @brief Generates a thumbnail for the given filename
 *
 * @param filename
 * @param outfile_count Number of the current outfile being processed
 * @param total_outfiles Total number of outfiles being processed
 */
void generate_thumbnail(gchar *filename, int outfile_count, int total_outfiles){
	gchar *ft_command = g_strdup("ffmpegthumbnailer");
	ft_command = strncat(ft_command, (gchar *) " -i \"", 5);
	ft_command = strncat(ft_command, filename, strlen(filename));
	ft_command = strncat(ft_command, (gchar *) "\" -o \"", 6);
	ft_command = strncat(ft_command, filename, strlen(filename));
	ft_command = strncat(ft_command, (gchar *) ".png\" -s0 -q10 &>/dev/null", 14);
	printf("%c[1m", 27);
	printf("Generating preview: %d/%d: %s.png\n", outfile_count, total_outfiles, filename);
	printf("%c[0m", 27);
	if (enc_arguments.debug) {
		printf("%s\n", ft_command);
	} else {
		system((char *) ft_command);
	}
	g_free(ft_command);
}

/**
 * @brief Handle the logic around calling handbrake
 *
 * @param hb_command String containing complete call to HandBrakeCLI with all arguments
 * @param out_count Which outfile section is being encoded
 * @param overwrite Overwrite existing files
 *
 * @return error status from hb_fork(), 0 is success
 */
int call_handbrake(gchar *hb_command, int out_count, bool overwrite)
{
	// separate filename and construct log filename
	const gchar *filename_start = strstr(hb_command, "-o")+4;
	const gchar *filename_end = strstr(filename_start, "\"");
	gchar *filename = xmlStrsub(filename_start, 0, filename_end-filename_start);
	gchar *log_filename = g_strdup(filename);
	log_filename = strcat(log_filename, ".log");
	// file doesn't exist, go ahead
	if ( access((char *) filename, F_OK ) != 0 ) {
		int r = hb_fork(hb_command, log_filename, out_count);
		g_free(filename);
		g_free(log_filename);
		return r;
	}
	// file isn't writable, error
	if ( access((char *) filename, W_OK ) != 0 ) {
		fprintf(stderr, "%d: filename: \"%s\" is not writable\n",
				out_count, filename);
		g_free(filename);
		g_free(log_filename);
		return 1;
	}
	// overwrite option was set, go ahead
	if (overwrite) {
		int r = hb_fork(hb_command, log_filename, out_count);
		g_free(filename);
		g_free(log_filename);
		return r;
	} else {
		char c;
		printf("File: \"%s\" already exists.\n", filename);
		printf("Run hbr with '-y' option to automatically overwrite.\n");
		do {
			printf("Do you want to overwrite? (y/n) ");
			scanf(" %c", &c);
			c = toupper(c);
		} while (c != 'N' && c != 'Y');
		if ( c == 'N' ) {
			g_free(filename);
			g_free(log_filename);
			return 1;
		} else {
			int r = hb_fork(hb_command, log_filename, out_count);
			g_free(filename);
			g_free(log_filename);
			return r;
		}
	}
}

/**
 * @brief Test write access, fork and exec, redirect output for logging
 *
 * @param hb_command String containing complete call to HandBrakeCLI with all arguments
 * @param log_filename Filename to log to
 * @param out_count Which outfile section is being encoded
 *
 * @return 1 on error, 0 on success
 */
int hb_fork(gchar *hb_command, gchar *log_filename, int out_count)
{
	// check if current working directory is writeable
	char *cwd = getcwd(NULL, 0);
	if ( access((char *) cwd, W_OK|X_OK) != 0 ) {
		fprintf(stderr, "%d: Current directory: \"%s\" is not writable\n", out_count, cwd);
		free(cwd);
		return 1;
	}
	
	// test logfile was opened
	errno = 0;
	FILE *logfile = fopen((const char *)log_filename, "w");
	if (logfile == NULL) {
		perror("hb_fork(): Failed to open logfile");
		free(cwd);
		return 1;
	}
	
	// test pipe was opened
	int hb_err[2];
	pid_t hb_pid;
	if (pipe(hb_err)<0){
		fclose(logfile);
		free(cwd);
		return 1;
	}

	// fork to call HandBrakeCLI
	hb_pid = fork();
	if (hb_pid > 0) {
		//close write end of pipe on parent
		close(hb_err[1]);
	} else if (hb_pid == 0) {
		// replace stderr with our pipe for HandBrakeCLI
		close(hb_err[0]);
		close(2);
		dup2(hb_err[1], 2);
		close(hb_err[1]);
		//TODO fix so this errors before handbrake is called and outputs a log file
		execl("/bin/sh", "sh", "-c", hb_command, (char *) NULL);
		_exit(1);
	} else {
		perror("hb_fork(): Failed to fork");
		free(cwd);
		close(hb_err[1]);
		fclose(logfile);
		return 1;
	}
	// buffer output from handbrake and write to logfile
	char *buf = (char *) malloc(1024*sizeof(char));
	int bytes;
	while ( (bytes = read(hb_err[0], buf, 1024)) > 0) {
		fwrite(buf, sizeof(char), bytes, logfile);
	}
	free(buf);
	close(hb_err[1]);
	fclose(logfile);
	free(cwd);
	return 0;
}
