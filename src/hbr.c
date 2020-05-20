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
#include <stdlib.h>                     // for NULL, exit
#include <sys/wait.h>                   // for wait
#include <sys/types.h>                  // for pid_t
#include <glib.h>
#include <glib/gstdio.h>

#include "util.h"
#include "keyfile.h"
#include "build_args.h"

// PROTOTYPES
GKeyFile * fetch_or_generate_keyfile(void);
void encode_loop(GKeyFile *inkeyfile, GKeyFile *merged_config,
        const gchar *infile);
void generate_thumbnail(gchar *filename, int outfile_count, int total_outfiles,
        gboolean debug);
int call_handbrake(GPtrArray *args, int out_count, gboolean overwrite,
        gboolean skip, gchar *filename);
int hb_fork(gchar *args[], gchar *log_filename);
gboolean good_output_option_path(void);
gboolean make_output_directory(GKeyFile *outfile, const gchar *group,
        const gchar* infile_path);

// Command line options
static gboolean opt_debug         = FALSE; // Print commands instead of executing
static gboolean opt_preview       = FALSE; // Generate preview image using ffmpegthumbnailer
static gboolean opt_overwrite     = FALSE; // Default to overwriting previous encodes
static gboolean opt_skip_existing = FALSE; // Skip encode if existing file would be overwritten
static int      opt_episode       = -1;    // Specifies a particular episode number to be encoded
static gchar    *opt_hbversion    = NULL;  // Override handbrake version detection
static gchar    *opt_config       = NULL;  // Override config file location
static gchar    *opt_output       = NULL;  // Override location to write output files
static gchar    **opt_input_files = NULL;  // List of files for hbr to use as input

static gchar    *config_file_path = NULL;

static gboolean print_version(
        __attribute__((unused)) const gchar *option_name,
        __attribute__((unused)) const gchar *value,
        __attribute__((unused)) gpointer data,
        __attribute__((unused)) GError **error) {
    printf("hbr (handbrake runner) 0.0\n" //TODO someday we'll release and have a version number
            "Copyright (C) 2018 Joshua Honeycutt\n"
            "License GPLv2: GNU GPL version 2 <http://gnu.org/licenses/gpl2.html>\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n");
    exit(0);
}

static GOptionEntry entries[] =
{
    {"debug",     'd', 0, G_OPTION_ARG_NONE,      &opt_debug,
        "print the commands to be run instead of executing", NULL},
    {"config",    'c', 0, G_OPTION_ARG_FILENAME,  &opt_config,
        "use named configuration file instead of default", NULL},
    {"preview",   'p', 0, G_OPTION_ARG_NONE,      &opt_preview,
        "generate a preview image for each output file", NULL},
    {"overwrite", 'y', 0, G_OPTION_ARG_NONE,      &opt_overwrite,
        "overwrite encoded files without confirmation", NULL},
    {"skip",      'n', 0, G_OPTION_ARG_NONE,      &opt_skip_existing,
        "skip encoding if output file already exists", NULL},
    {"episode",   'e', 0, G_OPTION_ARG_INT,       &opt_episode,
        "encodes first entry with matching episode number", "NUMBER"},
    {"output",    'o', 0, G_OPTION_ARG_FILENAME,  &opt_output,
        "override location to write output files", "PATH"},
    {"hbversion", 'H', 0, G_OPTION_ARG_STRING,    &opt_hbversion,
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
        hbr_error("Option parsing failed: %s\n", NULL, NULL, NULL, NULL, error->message);
        g_error_free(error);
        g_option_context_free(context);
        exit(1);
    }

    // error and exit if these conflicting options are both present
    if (opt_overwrite && opt_skip_existing) {
        hbr_error("Option 'overwrite' (-y) is not compatible with 'skip' (-n).",
                NULL, NULL, NULL, NULL);
        g_option_context_free(context);
        exit(1);
    }

    // setup options pointers and lookup tables
    determine_handbrake_version(NULL);
    arg_hash_generate();

    // parse hbr config or create a default
    GKeyFile *config;
    if ((config = fetch_or_generate_keyfile()) == NULL) {
        g_option_context_free(context);
        g_strfreev(opt_input_files);
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

    // check the output path from command line option
    if (!good_output_option_path()) {
        g_option_context_free(context);
        g_strfreev(opt_input_files);
        g_key_file_free(config);
        exit(1);
    }

    // loop over each input file
    int i = 0;
    while (opt_input_files[i] != NULL) {
        // parse input file
        GKeyFile *current_infile = parse_validate_key_file(opt_input_files[i], config);
        if (current_infile == NULL) {
            hbr_error("Could not complete input file", opt_input_files[i], NULL,
                    NULL, NULL);
            i++;
            continue;
        }

        // merge config sections from global config and current infile
        GKeyFile *merged = merge_key_group(current_infile, "CONFIG",
                config, "CONFIG", "MERGED_CONFIG");

        /*
         * merged may be null if both CONFIG sections are empty or
         * one group doesn't exist (CONFIG groups are already validated
         * to exist at this point though)
         */
        if (merged == NULL) {
            hbr_error("Failed to merge global config (%s) and local config",
                    opt_input_files[i], NULL, NULL, NULL, config_file_path);
        } else {
            // override output path if option given
            if (opt_output != NULL) {
                g_key_file_set_string(merged, "MERGED_CONFIG", "output_basedir",
                        opt_output);
            }

            // encode each outfile
            encode_loop(current_infile, merged, opt_input_files[i]);
        }

        // clean up
        g_key_file_free(current_infile);
        g_key_file_free(merged);
        i++;
    }
    arg_hash_cleanup();
    g_key_file_free(config);
    g_option_context_free(context);
    g_strfreev(opt_input_files);
    exit(0);
}

/**
 * @brief Verify --output path exists and is (symlink to) a directory
 *
 * @return TRUE when path is good
 */
gboolean good_output_option_path(void)
{
    if (opt_output != NULL) {
        // check if path exists
        GFile *output_path = g_file_new_for_commandline_arg(opt_output);
        if (!g_file_query_exists(output_path, NULL)) {
            hbr_error("Invalid output path", g_file_get_path(output_path), NULL,
                    NULL, NULL);
            g_object_unref(output_path);
            return FALSE;
        }
        // check if path is a directory (also allows symlinks to directories)
        if (g_file_query_file_type(output_path, G_FILE_QUERY_INFO_NONE, NULL)
                != G_FILE_TYPE_DIRECTORY) {
            hbr_error("Output path is not a directory",
                    g_file_get_path(output_path), NULL, NULL, NULL);
            g_object_unref(output_path);
            return FALSE;
        }
    }
    // no path is a good path
    return TRUE;
}

/**
 * @brief Tries to read an existing config file or generate a new one
 *
 * @return Parsed config keyfile
 */
GKeyFile *fetch_or_generate_keyfile(void)
{
    // Try $XDG_CONFIG_HOME, then home dir
    GString *config_dir = g_string_new(NULL);
    GString *alt_config_dir = g_string_new(NULL);
    const gchar *xdg_config_home = g_getenv("XDG_CONFIG_HOME");
    const gchar *home = g_get_home_dir();

    gchar *basename = NULL;
    if (opt_config) {
        // only consider opt_config if specified
        gchar *dirname = g_path_get_dirname(opt_config);
        basename = g_path_get_basename(opt_config);
        g_string_printf(config_dir, "%s", dirname);
        g_string_printf(alt_config_dir, "%s", dirname);
        g_free(dirname);
    } else if (xdg_config_home) {
        g_string_printf(config_dir, "%s%s", xdg_config_home, "/hbr/");
        g_string_printf(alt_config_dir, "%s%s", home, "/.config/hbr/");
    } else {
        g_string_printf(config_dir, "%s%s", home, "/.config/hbr/");
        g_string_printf(alt_config_dir, "%s%s", xdg_config_home, "/hbr/");
    }
    if (g_mkdir_with_parents(config_dir->str, 0700) == -1) {
        hbr_error("Failed to create config file", config_dir->str, NULL, NULL,
                NULL);
        exit(1);
    }

    GString *config_file = g_string_new(NULL);
    GString *alt_config_file = g_string_new(NULL);
    if (opt_config) {
        g_string_printf(config_file, "%s%c%s", config_dir->str, G_DIR_SEPARATOR, basename);
        g_string_printf(alt_config_file, "%s%c%s", config_dir->str, G_DIR_SEPARATOR, basename);
    } else {
        g_string_printf(config_file, "%s%s", config_dir->str, "hbr.conf");
        g_string_printf(alt_config_file, "%s%s", config_dir->str, "hbr.conf");
    }
    g_string_free(config_dir, TRUE);
    g_string_free(alt_config_dir, TRUE);

    GKeyFile *keyfile = NULL;
    // check config file exists
    if (g_file_test (config_file->str,
                (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
        // parse config file
        keyfile = parse_validate_key_file(config_file->str, NULL);
        if (keyfile == NULL) {
            // Quit, parse_validate_key_file() will report errors
            g_string_free(config_file, TRUE);
            return NULL;
        }
    } else if (g_file_test (alt_config_file->str,
                (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
        // parse config file
        keyfile = parse_validate_key_file(config_file->str, NULL);
        if (keyfile == NULL) {
            // Quit, parse_validate_key_file() will report errors
            g_string_free(config_file, TRUE);
            return NULL;
        }
    } else {
        // Create and write a default config
        keyfile = generate_default_key_file();
        GError *error = NULL;
        if (!g_key_file_save_to_file(keyfile, config_file->str, &error)) {
            hbr_error("Error writing config file: %s", config_file->str, NULL,
                    NULL, NULL, error->message);
            g_error_free(error);
            g_string_free(config_file, TRUE);
            return NULL;
        } else {
            hbr_info("Default config file generated", config_file->str, NULL,
                    NULL, NULL);
        }
    }
    config_file_path = g_strdup(config_file->str);
    g_string_free(config_file, TRUE);
    g_string_free(alt_config_file, TRUE);
    return keyfile;
}

/**
 * @brief Loops through each encode or the specified encode to call handbrake
 *
 * @param inkeyfile     Input keyfile
 * @param merged_config Input keyfile merged with global config
 * @param infile        Input file path for error reporting
 */
void encode_loop(GKeyFile *inkeyfile, GKeyFile *merged_config,
        const gchar *infile) {
    // loop for each out_file tag in keyfile
    gsize out_count = 0;
    gchar **outfiles = get_outfile_list(inkeyfile, &out_count);
    if (out_count < 1) {
        hbr_error("No valid outfile sections found. Quitting", infile, NULL,
                NULL, NULL);
        exit(1);
    }
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
            hbr_error("Could not find specified episode (-e %d). Quitting",
                    NULL, NULL, NULL, NULL, opt_episode);
            exit(1);
        }
    }
    // encode all the episodes if loop parameters weren't modified above
    for (gsize i = 0; i < out_count; i++) {
        // merge current outfile section with config section
        GKeyFile *current_outfile = merge_key_group(inkeyfile, outfiles[i],
                merged_config, "MERGED_CONFIG", "CURRENT_OUTFILE");

        if (current_outfile == NULL) {
            hbr_error("Failed to merge config and outfile sections. Skipping.",
                    infile, outfiles[i], NULL, NULL);
            continue;
        }

        // Determine if we should produce debug output or actually run HandBrake.
        // This is a special case where outfile config's debug=true only matters
        // if opt_debug is not true
        gboolean debug = opt_debug;
        if (g_key_file_get_boolean(current_outfile, "CURRENT_OUTFILE", "debug", NULL)) {
            debug = TRUE;
        }

        // build full HandBrakeCLI command
        GPtrArray *args = build_args(current_outfile, "CURRENT_OUTFILE", debug);
        gchar *filename = build_filename(current_outfile, "CURRENT_OUTFILE");
        gchar *basename = g_path_get_basename(filename);

        // output current encode information (codes are for bold text)
        g_print("%c[1m", 27);
        if (debug) {
            g_print("# ");
        }
        g_print("Encoding: %lu/%lu: %s\n", i+1, out_count, basename);
        g_print("%c[0m", 27);

        // Create directory
        if (!debug) {
            gchar *dirname = g_path_get_dirname(filename);
            if (g_mkdir_with_parents(dirname, 0777) != 0) {
                // TODO BUG: g_mkdir_with_parents fails due to permissions
                // if the directory already exists, but hbr would not have
                // permissions to create it.
                hbr_error("Failed to make directory for encode", infile,
                        NULL, NULL, NULL);
                g_key_file_free(current_outfile);
                g_free(filename);
                g_free(basename);
                g_strfreev(outfiles);
                g_free(dirname);
                return;
            }
            g_free(dirname);
            if (!make_output_directory(current_outfile, "CURRENT_OUTFILE", infile)) {
                // skip this outfile, error output comes from make_extra_directory()
                g_key_file_free(current_outfile);
                g_free(filename);
                g_free(basename);
                continue;
            }
        }

        if (debug) {
            // print full handbrake command
            gchar *temp = g_strjoinv(" ", (gchar**)args->pdata);
            g_print("HandBrakeCLI %s\n", temp);
            g_free(temp);
        } else {
            if (call_handbrake(args, i, opt_overwrite, opt_skip_existing, filename) == -1) {
                hbr_error("%lu: Handbrake call failed. %s was not encoded",
                    outfiles[i], NULL, NULL, NULL, i, filename);
                g_object_unref(filename);
                g_free(basename);
                g_ptr_array_free(args, TRUE);
                g_key_file_free(current_outfile);
                continue;
            }
        }
        // produce a thumbnail
        gboolean preview = FALSE;
        if (g_key_file_has_key(current_outfile, "CURRENT_OUTFILE", "preview", NULL)) {
            preview = g_key_file_get_boolean(current_outfile, "CURRENT_OUTFILE", "preview", NULL);
        }
        if (opt_preview || preview) {
            generate_thumbnail(filename, i, out_count, debug);
        }

        g_free(filename);
        g_free(basename);
        g_ptr_array_free(args, TRUE);
        g_key_file_free(current_outfile);
    }
    g_strfreev(outfiles);
}

/**
 * @brief Create the output directory where files are to be written
 *
 * @param outfile     Keyfile to fetch values from
 * @param group       Group to fetch values from
 * @param infile_path Path to keyfile (for error output)
 *
 * @return True on success
 */
gboolean make_output_directory(GKeyFile *outfile, const gchar *group,
        const gchar* infile_path)
{
    // create output directory
    GError *error = NULL;
    gchar *output_path = g_key_file_get_string(outfile, group,
            "output_basedir", &error);
    gchar *filename = build_filename(outfile, group);
    gchar *dirname = g_path_get_dirname(filename);

    if (g_mkdir_with_parents(dirname, 0777) != 0) {
        hbr_error("Failed to create output directory", infile_path, NULL,
                NULL, NULL);
        g_free(output_path);
        g_free(filename);
        g_free(dirname);
        return FALSE;
    }
    g_free(output_path);
    g_free(filename);
    g_free(dirname);
    return TRUE;
}

/**
 * @brief Generates a thumbnail for the given filename
 *
 * @param filename       Video filename to generate a thumbnail for
 * @param outfile_count  Number of the current outfile being processed
 * @param total_outfiles Total number of outfiles being processed
 * @param debug          When true, command is printed instead of run
 */
void generate_thumbnail(gchar *filename, int outfile_count, int total_outfiles,
        gboolean debug)
{
    GString *ft_command = g_string_new("ffmpegthumbnailer");
    g_string_append_printf(ft_command,
            " -i\"%s\" -o\"%s.png\" -s0 -q10 2>&1 >/dev/null", filename,
            filename);
    g_print("%c[1m", 27);
    g_print("# Generating preview: %d/%d: %s.png\n", outfile_count+1,
            total_outfiles, filename);
    g_print("%c[0m", 27);
    if (debug) {
        g_print("%s\n", ft_command->str);
    } else {
        system((char *) ft_command->str);
    }
    g_string_free(ft_command, TRUE);
}

/**
 * @brief Handle the logic around calling handbrake
 *
 * @param args      String containing argumenst HandBrakeCLI
 * @param out_count Which outfile section is being encoded
 * @param overwrite Overwrite existing files
 * @param skip      Skip encoding existing files
 * @param filename  output filename (also used to generate log filename)
 *
 * @return error status from hb_fork(), 0 is success
 */
int call_handbrake(GPtrArray *args, int out_count, gboolean overwrite,
        gboolean skip, gchar *filename)
{
    GString *log_filename = g_string_new(filename);
    log_filename = g_string_append(log_filename, ".log");

    g_ptr_array_insert(args, 0, g_strdup("HandBrakeCLI"));
    // file doesn't exist, go ahead
    if ( g_access((char *) filename, F_OK ) != 0 ) {
        int r = hb_fork((gchar **)args->pdata, log_filename->str);
        g_string_free(log_filename, TRUE);
        return r;
    }
    // file isn't writable, error
    if ( g_access((char *) filename, W_OK ) != 0 ) {
        hbr_error("%d: File is not writable", filename, NULL, NULL, NULL,
                out_count);
        g_string_free(log_filename, TRUE);
        return 1;
    }
    // overwrite option was set, go ahead
    if (overwrite) {
        int r = hb_fork((gchar **)args->pdata, log_filename->str);
        g_string_free(log_filename, TRUE);
        return r;
    // skip existing files
    } else if (skip) {
        g_print("File: \"%s\" already exists. Skipping encode.\n", filename);
        g_string_free(log_filename, TRUE);
        return 1;
    // prompt user
    } else {
        g_print("File: \"%s\" already exists.\n", filename);
        g_print("Run hbr with '-y' option to automatically overwrite, or '-n'"
                " to skip existing files.\n");
        char c;
        do {
            g_print("Do you want to overwrite? (y/n) ");
            if (scanf(" %c", &c) != 1) {
                continue;
            }
            c = toupper(c);
            // clear any extra input
            while ( getchar() != '\n' ) {}
        } while (c != 'N' && c != 'Y');
        if ( c == 'N' ) {
            g_string_free(log_filename, TRUE);
            return 1;
        } else {
            int r = hb_fork((gchar **)args->pdata, log_filename->str);
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
 *
 * @return 1 on error, 0 on success
 */
int hb_fork(gchar *args[], gchar *log_filename)
{
    // test logfile was opened
    errno = 0;
    FILE *logfile = fopen((const char *)log_filename, "w");
    if (logfile == NULL) {
        hbr_error("hb_fork(): Failed to open logfile: %s", log_filename,
                NULL, NULL, NULL, g_strerror(errno));
        return 1;
    }

    // test pipe was opened
    int hb_err[2];
    pid_t hb_pid;
    if (pipe(hb_err)<0){
        fclose(logfile);
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
        close(hb_err[1]);
        fclose(logfile);
        return 1;
    }
    // buffer output from handbrake and write to logfile
    char *buf = g_malloc(1024*sizeof(char));
    if (buf == NULL) {
        hbr_error("hb_fork(): failed to allocate memory. Quitting", NULL, NULL,
                NULL, NULL);
        close(hb_err[1]);
        fclose(logfile);
        exit(1);
    }
    int bytes;
    while ( (bytes = read(hb_err[0], buf, 1024)) > 0) {
        fwrite(buf, sizeof(char), bytes, logfile);
    }
    g_free(buf);
    close(hb_err[1]);
    fclose(logfile);
    wait(NULL);
    //TODO we can do a little better return value here if we check wait's parameter
    return 0;
}
