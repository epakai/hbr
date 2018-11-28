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

#include <ctype.h>                      // for toupper
#include <errno.h>                      // for errno
#include <stdio.h>                      // for NULL, stderr, stdout, etc
#include <stdlib.h>                     // for exit
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <unistd.h>                     // for R_OK, W_OK, X_OK, F_OK
#include "keyfile.h"
#include "build_args.h"

/*
 * PROTOTYPES
 */
GKeyFile * fetch_or_generate_keyfile();
void encode_loop(GKeyFile *inkeyfile, GKeyFile *merged_config);
void generate_thumbnail(gchar *filename, int outfile_count, int total_outfiles);
int call_handbrake(GPtrArray *args, int out_count, gboolean overwrite, gchar *filename);
int hb_fork(gchar *args[], gchar *log_filename, int out_count);
gboolean good_output_option_path();

static gboolean opt_debug     = FALSE; // Print commands instead of executing
static gboolean opt_preview   = FALSE; // Generate preview image using ffmpegthumbnailer
static gboolean opt_overwrite = FALSE; // Default to overwriting previous encodes
static int      opt_episode   = -1;    // Specifies a particular episode number to be encoded
static gchar    *opt_hbversion = NULL;  // Override handbrake version detection
static gchar    *opt_output   = NULL;  // Override location to write output files
static gchar    **opt_input_files = NULL;  // List of files for hbr to use as input

void print_version() {
    printf("hbr (handbrake runner) 0.0\n" //TODO someday we'll release and have a version number
            "Copyright (C) 2018 Joshua Honeycutt\n"
            "License GPLv2: GNU GPL version 2 <http://gnu.org/licenses/gpl2.html>\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n");
    exit(0);
}

static GOptionEntry entries[] =
{
    {"debug",     'd', 0, G_OPTION_ARG_NONE,     &opt_debug,
        "print the commands to be run instead of executing", NULL},
    {"preview",   'p', 0, G_OPTION_ARG_NONE,     &opt_preview,
        "generate a preview image for each output file", NULL},
    {"overwrite", 'y', 0, G_OPTION_ARG_NONE,     &opt_overwrite,
        "overwrite encoded files without confirmation", NULL},
    {"episode",   'e', 0, G_OPTION_ARG_INT,      &opt_episode,
        "encodes first entry with matching episode number", "NUMBER"},
    {"output",    'o', 0, G_OPTION_ARG_FILENAME, &opt_output,
        "override location to write output files", "PATH"},
    {"hbversion", 'H', 0, G_OPTION_ARG_STRING,   &opt_hbversion,
        "override handbrake version detection", "X.Y.Z"},
    {"version",   'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, print_version,
        "prints version info and exit", NULL},
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &opt_input_files,
        NULL, NULL},
    { NULL }
};

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
    // parse command line options/args
    GOptionContext *context = g_option_context_new("[FILE...]");
    g_option_context_set_summary(context,
            "handbrake runner -- runs handbrake with setting from key-value pair file(s)");
    g_option_context_set_description(context,
            "Report bugs at <https://github.com/epakai/hbr/issues>\n");
    g_option_context_add_main_entries (context, entries, NULL);
    GError *error = NULL;
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_print ("Option parsing failed: %s\n", error->message);
        g_error_free(error);
        g_option_context_free(context);
        exit(1);
    }

    // parse hbr config or create a default
    GKeyFile *config;
    if ((config = fetch_or_generate_keyfile()) == NULL) {
        g_option_context_free(context);
        g_strfreev(opt_input_files);
        g_key_file_free(config);
        exit(1);
    }

    // print help when there are no file arguments to process
    if (opt_input_files == NULL) {
        gchar *temp = g_option_context_get_help(context, TRUE, NULL);
        printf("%s", temp);
        g_free(temp);
        g_option_context_free(context);
        g_strfreev(opt_input_files);
        g_key_file_free(config);
        exit(1);
    }

    // check the output path
    if (!good_output_option_path()) {
        g_option_context_free(context);
        g_strfreev(opt_input_files);
        g_key_file_free(config);
        exit(1);
    }

    // setup options pointers and lookup tables
    determine_handbrake_version(NULL);

    // loop over each input file
    int i = 0;
    while (opt_input_files[i] != NULL) {
        // parse input file
        GKeyFile *current_infile = parse_validate_key_file(opt_input_files[i], config);
        if (current_infile == NULL) {
            g_option_context_free(context);
            g_key_file_free(config);
            //TODO ERROR HERE
            exit(1); // TODO maybe just error here and skip the current infile
        }

        // merge config sections from global config and current infile
        GKeyFile *merged = merge_key_group(current_infile, "CONFIG",
                config, "CONFIG", "MERGED_CONFIG");

        // override output path if option given
        if (opt_output != NULL) {
            g_key_file_set_string(merged, "MERGED_CONFIG", "output_basedir", opt_output);
        }

        // encode each outfile
        encode_loop(current_infile, merged);

        // clean up
        g_key_file_free(current_infile);
        g_key_file_free(merged);
        i++;
    }
    // destroy hash tables created by determine_handbrake_version()
    g_hash_table_destroy(options_index);
    g_hash_table_foreach(depends_index, free_slist_in_hash, NULL);
    g_hash_table_destroy(depends_index);
    g_hash_table_foreach(conflicts_index, free_slist_in_hash, NULL);
    g_hash_table_destroy(conflicts_index);
    g_key_file_free(config);
    g_option_context_free(context);
    g_strfreev(opt_input_files);
    exit(0);
}

gboolean good_output_option_path()
{
    if (opt_output != NULL) {
        // check if path exists
        GFile *output_path = g_file_new_for_commandline_arg(opt_output);
        if (!g_file_query_exists(output_path, NULL)) {
            fprintf(stderr, "Invalid output path: %s\n", g_file_get_path(output_path));
            g_object_unref(output_path);
            return FALSE;
        }
        // check if path is a directory (also allows symlinks to directories)
        if (g_file_query_file_type(output_path, G_FILE_QUERY_INFO_NONE, NULL)
                != G_FILE_TYPE_DIRECTORY) {
            fprintf(stderr, "Output path is not a directory: %s\n",
                    g_file_get_path(output_path));
            g_object_unref(output_path);
            return FALSE;
        }
    }
    // no path is a good path
    return TRUE;
}

GKeyFile *fetch_or_generate_keyfile()
{
    // Try $XDG_CONFIG_HOME, then home dir
    GString *config_dir = g_string_new(NULL);
    const gchar *xdg_home = g_getenv("XDG_CONFIG_HOME");
    if (xdg_home) {
        g_string_printf(config_dir, "%s%s", xdg_home, "/hbr/");
    } else {
        const gchar *home = g_get_home_dir();
        g_string_printf(config_dir, "%s%s", home, "/.config/hbr/");
    }
    if (g_mkdir_with_parents(config_dir->str, 0700) == -1) {
        fprintf(stderr, "Failed to create config at: %s\n", config_dir->str);
        exit(1);
    }
    GString *config_file = g_string_new(NULL);
    g_string_printf(config_file, "%s%s", config_dir->str, "hbr.conf");
    g_string_free(config_dir, TRUE);
    // check config file exists
    if (g_file_test (config_file->str, (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
        // parse config file
        GKeyFile *keyfile = parse_validate_key_file(config_file->str, NULL);
        if (keyfile == NULL) {
            // Quit, parse_key_file() will report errors
            g_string_free(config_file, TRUE);
            return NULL;
        }
        g_string_free(config_file, TRUE);
        return keyfile;
    } else {
        // Create and write a default config
        GKeyFile *keyfile = generate_default_key_file();
        GError *error = NULL;
        if (!g_key_file_save_to_file(keyfile, config_file->str, &error)) {
            g_warning("Error writing config file: %s", error->message);
            g_error_free(error);
        } else {
            g_message("Default config file generated at: %s\n", config_file->str);
        }
        g_string_free(config_file, TRUE);
        return keyfile;
    }
}

/**
 * @brief Loops through each encode or the specified encode
 *
 * @param keyfile Document to read outfile specifications from
 * @param hb_options general option string for HandBrakeCLI
 */
void encode_loop(GKeyFile *inkeyfile, GKeyFile *merged_config) {
    // loop for each out_file tag in keyfile
    gsize out_count = 0;
    gchar **outfiles = get_outfile_list(inkeyfile, &out_count);
    if (out_count < 1) {
        fprintf(stderr, "No valid outfile sections found. Quitting.\n");
        exit(1);
    }
    int i = 0;
    // Handle -e option to encode a single episode
    if (opt_episode >= 0) {
        // adjust parameters for following loop so it runs once
        gchar *group = get_group_from_episode(inkeyfile, opt_episode);
        if (group != NULL) {
            // replace outfiles list with list of single episode
            gchar ** new_outfiles = g_malloc(2*sizeof(gchar *));
            new_outfiles[0] = g_strdup(group);
            new_outfiles[1] = NULL;
            g_free(group);
            g_strfreev(outfiles);
            outfiles = new_outfiles;
            out_count = 1;
        } else {
            fprintf(stderr, "Could not find episode %d. Quitting.\n", opt_episode);
            exit(1);
        }
    }
    // encode all the episodes if loop parameters weren't modified above
    for (; i < out_count; i++) {
        // merge current outfile section with config section
        GKeyFile *current_outfile = merge_key_group(inkeyfile, outfiles[i],
                merged_config, "MERGED_CONFIG", "CURRENT_OUTFILE");

        // build full HandBrakeCLI command
        GPtrArray *args = build_args(current_outfile, "CURRENT_OUTFILE", opt_debug);
        GString *filename = build_filename(current_outfile, "CURRENT_OUTFILE", FALSE);

        // output current encode information (codes are for bold text)
        printf("%c[1m", 27);
        if (opt_debug) {
            printf("# ");
        }
        printf("Encoding: %d/%lu: %s\n", i+1, out_count, filename->str);
        printf("%c[0m", 27);

        // grab filename with *full path* for log and thumbnail creation
        g_string_free(filename, TRUE);
        filename = build_filename(current_outfile, "CURRENT_OUTFILE", TRUE);
        if (opt_debug) {
            // print full handbrake command
            gchar *temp = g_strjoinv(" ", (gchar**)args->pdata);
            printf("HandBrakeCLI %s\n", temp);
            g_free(temp);
        } else {
            if (call_handbrake(args, i, opt_overwrite, filename->str) == -1) {
                fprintf(stderr,
                        "%d: Handbrake call failed for outfile: %s"
                        "%s was not encoded\n",
                        i, outfiles[i], filename->str);
                g_string_free(filename, TRUE);
                g_ptr_array_free(args, TRUE);
                g_key_file_free(current_outfile);
                continue;
            }
        }
        // produce a thumbnail
        if (opt_preview) {
            generate_thumbnail(filename->str, i, out_count);
        }
        g_string_free(filename, TRUE);
        g_ptr_array_free(args, TRUE);
        g_key_file_free(current_outfile);
    }
    g_strfreev(outfiles);
}

/**
 * @brief Generates a thumbnail for the given filename
 *
 * @param filename
 * @param outfile_count Number of the current outfile being processed
 * @param total_outfiles Total number of outfiles being processed
 */
void generate_thumbnail(gchar *filename, int outfile_count, int total_outfiles){
    GString *ft_command = g_string_new("ffmpegthumbnailer");
    g_string_append_printf(ft_command,
            " -i\"%s\" -o\"%s.png\" -s0 -q10 2>&1 >/dev/null", filename, filename);
    printf("%c[1m", 27);
    printf("Generating preview: %d/%d: %s.png\n", outfile_count+1, total_outfiles, filename);
    printf("%c[0m", 27);
    if (opt_debug) {
        printf("%s\n", ft_command->str);
    } else {
        system((char *) ft_command->str);
    }
    g_string_free(ft_command, TRUE);
}

/**
 * @brief Handle the logic around calling handbrake
 *
 * @param args String containing argumenst HandBrakeCLI
 * @param out_count Which outfile section is being encoded
 * @param overwrite Overwrite existing files
 *
 * @return error status from hb_fork(), 0 is success
 */
int call_handbrake(GPtrArray *args, int out_count, gboolean overwrite, gchar *filename)
{
    GString *log_filename = g_string_new(filename);
    log_filename = g_string_append(log_filename, ".log");

    g_ptr_array_insert(args, 0, g_strdup("HandBrakeCLI"));
    // file doesn't exist, go ahead
    if ( access((char *) filename, F_OK ) != 0 ) {
        int r = hb_fork((gchar **)args->pdata, log_filename->str, out_count);
        g_string_free(log_filename, TRUE);
        return r;
    }
    // file isn't writable, error
    if ( access((char *) filename, W_OK ) != 0 ) {
        fprintf(stderr, "%d: filename: \"%s\" is not writable\n",
                out_count, filename);
        g_string_free(log_filename, TRUE);
        return 1;
    }
    // overwrite option was set, go ahead
    if (overwrite) {
        int r = hb_fork((gchar **)args->pdata, log_filename->str, out_count);
        g_string_free(log_filename, TRUE);
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
            g_string_free(log_filename, TRUE);
            return 1;
        } else {
            int r = hb_fork((gchar **)args->pdata, log_filename->str, out_count);
            g_string_free(log_filename, TRUE);
            return r;
        }
    }
}

/**
 * @brief Test write access, fork and exec, redirect output for logging
 *
 * @param args String containing arguments to HandBrakeCLI
 * @param log_filename Filename to log to
 * @param out_count Which outfile section is being encoded
 *
 * @return 1 on error, 0 on success
 */
int hb_fork(gchar *args[], gchar *log_filename, int out_count)
{
    // check if current working directory is writeable
    char *cwd = getcwd(NULL, 0);
    if ( access((char *) cwd, W_OK|X_OK) != 0 ) {
        fprintf(stderr, "%d: Current directory: \"%s\" is not writable\n",
                out_count, cwd);
        free(cwd);
        return 1;
    }

    // test logfile was opened
    errno = 0;
    FILE *logfile = fopen((const char *)log_filename, "w");
    if (logfile == NULL) {
        fprintf(stderr, "hb_fork(): Failed to open logfile: %s: (%s)",
                strerror(errno), log_filename);
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
        execvp("HandBrakeCLI", args);
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
