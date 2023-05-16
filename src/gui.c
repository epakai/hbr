/*
 * hbr gui - handbrake runner
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
#include <gtk/gtk.h>

#include "config.h"
#include "util.h"
#include "keyfile.h"
#include "validate.h"
#include "build_args.h"
#include "options.h"


/// Print commands instead of executing
static gboolean opt_debug         = FALSE;

static gboolean print_version(
        __attribute__((unused)) const gchar *option_name,
        __attribute__((unused)) const gchar *value,
        __attribute__((unused)) gpointer data,
        __attribute__((unused)) GError **error) {
    printf("hbr gui (handbrake runner) %s\n"
            "Copyright (C) 2018 Joshua Honeycutt\n"
            "License GPLv2: GNU GPL version 2 <http://gnu.org/licenses/gpl2.html>\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n", VERSION);
    exit(EXIT_SUCCESS);
}

/**
 * @brief Command line options for GOption parser
 */
static GOptionEntry entries[] =
{
    {"debug",     'd', 0, G_OPTION_ARG_NONE,      &opt_debug,
        "print the commands to be run instead of executing", NULL},
    {"version",   'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, (gpointer) print_version,
        "prints version info and exit", NULL},
    { NULL }
};

/* Global data for options */
extern option_data_t option_data;
option_data_t option_data;

static void activate (GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *button_box;

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Window");
    gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);

    button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_container_add (GTK_CONTAINER (window), button_box);

    button = gtk_button_new_with_label ("Hello World");
    g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
    gtk_container_add (GTK_CONTAINER (button_box), button);

    gtk_widget_show_all (window);
}

/**
 * @brief Reads the input file and calls handbrake for each specified outfile
 *
 * @param argc Command line argument count
 * @param argv[] Arguments to be parsed by glib command line parser
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
        exit(EXIT_FAILURE);
    }

    determine_handbrake_version(NULL);
    arg_hash_generate();

    // Create a new application
    GtkApplication *app = gtk_application_new ("com.example.GtkApplication",
            G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    int gtk_return = g_application_run (G_APPLICATION (app), argc, argv);

    arg_hash_cleanup();
    g_option_context_free(context);
    exit(gtk_return);
}
