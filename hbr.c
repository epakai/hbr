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
#include <unistd.h>                     // for R_OK, W_OK, X_OK, F_OK
#include <libxml/xpath.h>
#include "gen_xml.h"                    // for gen_arguments, gen_argp, etc
#include "xml.h"                        // parse_xml, outfile_count, etc
#include "hb_options.h"
#include "out_options.h"

const char *argp_program_version = "hbr 1.0";
const char *argp_program_bug_address = "<https://github.com/epakai/hbr/issues>";

/*
 * PROTOTYPES
 */
static error_t parse_enc_opt (int key, char *arg, struct argp_state *);
int call_handbrake(xmlChar *hb_command, int out_count, bool overwrite);
int hb_fork(xmlChar *hb_command, xmlChar *log_filename, int out_count);

static char enc_doc[] = "handbrake runner -- runs handbrake with setting from an "
"xml file with all encoded files placed in current directory";
static char enc_args_doc[] = "<XML FILE>";

static struct argp_option enc_options[] = {
	{"debug",     'd', 0,     0, "print the commands to be run instead of executing", 1},
	{"episode",   'e', "NUM", 0, "only encodes for entry with matching episode number", 1},
	{"preview",   'p', 0,     0, "generate a preview image for each output file", 1},
	{"overwrite", 'y', 0,     0, "overwrite encoded files without confirmation", 1},
	{"version",   'V', 0,     0, "prints program version", 1},
	{"help",      '?', 0,     0, "Give this help list", 2},
	{"usage",     '@', 0,     OPTION_HIDDEN, "Give this help list", 2},
	{ 0, 0, 0, 0, 0, 0 }
};


/**
 * @brief Arguments for hbr. Handled by argp.
 */
struct enc_arguments {
	char *args[1];    /**< Single argument for the input xml file. */
	int episode;      /**< Specifies a particular episode number to be encoded. */
	bool overwrite;   /**< Default to overwriting previous encodes. */
	bool debug;       /**< Print commands instead of executing. */
	int preview;      /**< Generate preview image using ffmpegthumbnailer. */
};

static struct argp enc_argp = {enc_options, parse_enc_opt, enc_args_doc,
	enc_doc, NULL, 0, 0};

/*************************************************/

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
	// TODO: separate gen arg parsing, generation,
	// enc arg parsing, and encoding into separate functions
	struct enc_arguments enc_arguments;
	enc_arguments.episode = -1;
	enc_arguments.overwrite = false;
	enc_arguments.debug = false;
	enc_arguments.preview = 0;
	struct gen_arguments gen_arguments = {-1, 0, 0, -1, 0,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL};

	// check for request to generate new xml file
	argp_parse(&gen_argp, argc, argv,
			ARGP_IN_ORDER|ARGP_NO_EXIT|ARGP_SILENT,
			0, &gen_arguments);

	if ( gen_arguments.generate > 0 ) {
		// Call gen_xml with passed arguments or defaults
		
		struct crop crop_arg = get_crop(BAD_CAST gen_arguments.crop);
		xmlDocPtr doc = gen_xml(gen_arguments.generate, gen_arguments.title ?: 1,
				gen_arguments.season ?: 1, gen_arguments.video_type,
				gen_arguments.markers, gen_arguments.source ?: "",
				gen_arguments.year ?: "", crop_arg,
				gen_arguments.name ?: "", gen_arguments.format ?: "mkv",
				gen_arguments.basedir ?: "", gen_arguments.episodes);
		if (doc != NULL) {
			print_xml(doc);
		}
		return 0;
	}
	// parse normal options
	argp_parse (&enc_argp, argc, argv, ARGP_NO_HELP, 0, &enc_arguments);
	// parse xml document to tree
	xmlDocPtr xml_doc = parse_xml(enc_arguments.args[0]);
	if (xml_doc == NULL) {
		return 1;
	}
	xmlNode *root_element = xmlDocGetRootElement(xml_doc);
	if (xmlStrcmp(root_element->name, BAD_CAST "handbrake_encode")) {
		fprintf(stderr,
				"Wrong document type: handbrake_encode element not found in \"%s\"",
				xml_doc->URL);
		return 1;
	}

	//assemble call to HandBrakeCLI
	xmlChar *hb_options = NULL;
	hb_options = hb_options_string(xml_doc);
	if ( hb_options == NULL){
		xmlFree(hb_options);
		xmlFreeDoc(xml_doc);
		return 1;
	}

	// loop for each out_file tag in xml_doc
	int out_count = outfile_count(xml_doc);
	if (out_count < 1) {
		fprintf(stderr, "No valid outfiles found in \"%s\"\n", xml_doc->URL);
	}
	int i = 1; // start at outfile 1
	// Handle -e option to encode a single episode
	if (enc_arguments.episode >= 0) {
		// adjust parameters for following loop so it runs once
		int outfile_number = get_outfile_from_episode(xml_doc, enc_arguments.episode);
		i = outfile_number;
		out_count = outfile_number;
	}
	// encode all the episodes if loop parameters weren't modified above
	for (; i <= out_count; i++) {
		xmlChar *out_options = out_options_string(xml_doc, i);
		if ( out_options == NULL){
			xmlNode* outfile_node = get_outfile(xml_doc, i);
			fprintf(stderr,
					"%d: Bad outfile element in \"%s\" line number: "
					"%ld  was not encoded\n",
					i, xml_doc->URL, xmlGetLineNo(outfile_node));
			xmlFree(out_options);
			continue;
		}
		// build full HandBrakeCLI command
		xmlChar *hb_command = xmlCharStrdup("HandBrakeCLI");
		hb_command = xmlStrcat(hb_command, hb_options);
		hb_command = xmlStrcat(hb_command, out_options);

		const xmlChar *filename_start = xmlStrstr(hb_command, BAD_CAST "-o")+4;
		const xmlChar *filename_end = xmlStrstr(filename_start, BAD_CAST "\"");
		xmlChar *filename = xmlStrsub(filename_start, 0, filename_end-filename_start);
		
		printf("%c[1m", 27);
		printf("Encoding: %d/%d: %s\n", i, out_count, filename);
		printf("%c[0m", 27);
		if (enc_arguments.debug) {
			printf("%s\n", hb_command);
		} else {
			if (call_handbrake(hb_command, i, enc_arguments.overwrite) == -1) {
				xmlNode* outfile_node = get_outfile(xml_doc, i);
				fprintf(stderr,
						"%d: Handbrake call failed in \"%s\" line number: "
						"%ld  was not encoded\n",
						i, xml_doc->URL, xmlGetLineNo(outfile_node));
				xmlFree(out_options);
				xmlFree(hb_command);
				continue;
			}
		}
		// produce a thumbnail
		if (enc_arguments.preview) {
			xmlChar *ft_command = xmlCharStrdup("ffmpegthumbnailer");
			ft_command = xmlStrncat(ft_command, (xmlChar *) " -i \"", 5);
			ft_command = xmlStrncat(ft_command, filename, xmlStrlen(filename));
			ft_command = xmlStrncat(ft_command, (xmlChar *) "\" -o \"", 6);
			ft_command = xmlStrncat(ft_command, filename, xmlStrlen(filename));
			ft_command = xmlStrncat(ft_command, (xmlChar *) ".png\" -s0 -q10 &>/dev/null", 14);
			printf("%c[1m", 27);
			printf("Generating preview: %d/%d: %s.png\n", i, out_count, filename);
			printf("%c[0m", 27);
			if (enc_arguments.debug) {
				printf("%s\n", ft_command);
			} else {
				system((char *) ft_command);
			}
			xmlFree(ft_command);
		}
		xmlFree(filename);
		xmlFree(out_options);
		xmlFree(hb_command);
	}
	xmlFreeDoc(xml_doc);
	xmlFree(hb_options);
	xmlCleanupParser();

	return 0;
}

/**
 * @brief Argp Parse options for general hbr use
 *
 * @param key Key value associated with each object.
 * @param arg Value for the key being passed.
 * @param state Argp state for the parser
 *
 * @return Error status.
 */
static error_t parse_enc_opt (int key, char *arg, struct argp_state *state)
{
	struct enc_arguments *enc_arguments = (struct enc_arguments *) state->input;
	char prog_name[] = "hbr";
	switch (key) {
		case '@': // --usage
		case '?':
			argp_help(&enc_argp, stdout, ARGP_HELP_SHORT_USAGE, prog_name);
			argp_help(&enc_argp, stdout, ARGP_HELP_PRE_DOC, prog_name);
			argp_help(&enc_argp, stdout, ARGP_HELP_LONG, prog_name);

			printf("\nUsage: hbr %s [OPTION...]\n", gen_args_doc);
			argp_help(&gen_argp, stdout, ARGP_HELP_PRE_DOC, prog_name);
			argp_help(&gen_argp, stdout, ARGP_HELP_LONG, prog_name);
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
				printf("Usage: hbr %s [OPTION...]\n", gen_args_doc);
				argp_usage (state);
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
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
int call_handbrake(xmlChar *hb_command, int out_count, bool overwrite)
{
	// separate filename and construct log filename
	const xmlChar *filename_start = xmlStrstr(hb_command, BAD_CAST "-o")+4;
	const xmlChar *filename_end = xmlStrstr(filename_start, BAD_CAST "\"");
	xmlChar *filename = xmlStrsub(filename_start, 0, filename_end-filename_start);
	xmlChar *log_filename = xmlStrdup(filename);
	log_filename = xmlStrcat(log_filename, BAD_CAST ".log");
	if ( access((char *) filename, F_OK ) == 0 ) {
		if ( access((char *) filename, W_OK ) == 0 ) {
			if (!overwrite) {
				char c;
				printf("File: \"%s\" already exists.\n", filename);
				printf("Run hbr with '-y' option to automatically overwrite.\n");
				do{
					printf("Do you want to overwrite? (y/n) ");
					scanf(" %c", &c);
					c = toupper(c);
				} while (c != 'N' && c != 'Y');
				if ( c == 'N' ) {
					xmlFree(filename);
					xmlFree(log_filename);
					return 1;
				}else {
					int r = hb_fork(hb_command, log_filename, out_count);
					xmlFree(filename);
					xmlFree(log_filename);
					return r;
				}
			} else if (overwrite) {
				// call handbrake (overwrite file)
				int r = hb_fork(hb_command, log_filename, out_count);
				xmlFree(filename);
				xmlFree(log_filename);
				return r;
			}
		} else {
			fprintf(stderr, "%d: filename: \"%s\" is not writable\n", out_count, filename);
			xmlFree(filename);
			xmlFree(log_filename);
			return 1;
		}
	}
	int r = hb_fork(hb_command, log_filename, out_count);
	xmlFree(filename);
	xmlFree(log_filename);
	return r;
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
int hb_fork(xmlChar *hb_command, xmlChar *log_filename, int out_count)
{
	// check if current working directory is writeable
	char *cwd = getcwd(NULL, 0);
	if ( access((char *) cwd, W_OK|X_OK) == 0 ) {
		errno = 0;
		FILE *logfile = fopen((const char *)log_filename, "w");
		// test logfile was opened
		if (logfile != NULL) {
			int hb_err[2];
			pid_t hb_pid;
			// test pipe was opened
			if (pipe(hb_err)<0){
				fclose(logfile);
				xmlFree(log_filename);
				return 1;
			} else {
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
				char *buf = (char *) malloc(80*sizeof(char));
				int bytes;
				while ( (bytes = read(hb_err[0], buf, 80)) != 0) {
					fwrite(buf, bytes, 80, logfile);
				}
				free(buf);
			}
			close(hb_err[1]);
			fclose(logfile);
		} else {
			perror("hb_fork(): Failed to open logfile");
			free(cwd);
			return 1;
		}
	} else {
		fprintf(stderr, "%d: Current directory: \"%s\" is not writable\n", out_count, cwd);
		free(cwd);
		return 1;
	}
	free(cwd);
	return 0;
}
