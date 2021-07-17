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

#include <glib.h>
#include <gio/gio.h>
#include <stdlib.h>

#include "gen_hbr.h"
#include "util.h"

/**
 * @brief Episode structure used to build episode list when -l option is used.
 */
struct episode {
	gint number;
    gint season;
	gchar *name;
};

/**
 * @brief Episode list structure
 */
struct episode_list {
	gint count;
	struct episode *array;
};

static struct episode_list read_episode_list(const gchar *episode_filename);
static void free_episode_list(struct episode_list list);
static void create_outfile_section(GKeyFile *config, gint outfile_count, gint episode,
        gint title, gint season, const gchar *type, const gchar *iso_filename,
        const gchar *audio, const gchar *subtitle, const gchar *chapters,
        const gchar *specific_name);
/**
 * @brief Builds an episode_list from a file
 *
 * @param episode_filename Path to the episode list.
 *
 * @return List of episodes, must be free'd with free_episode_list()
 */
static struct episode_list read_episode_list(const gchar *episode_filename)
{
    int max_count = 30;
    struct episode_list list;
    list.count = 0;
    GDataInputStream *episode_list_file = open_datastream(episode_filename);
    if (episode_list_file == NULL)  {
        hbr_error("Failed to open episode list", episode_filename, NULL, NULL, NULL);
        list.count = 0;
        list.array = NULL;
        return list;
    }

    list.array = calloc(max_count, sizeof(struct episode));
    gchar *line;
    gchar *episode;
    gsize line_length = 0;
    while ((line = g_data_input_stream_read_line_utf8(episode_list_file,
                    &line_length, NULL, NULL)) && list.count < max_count) {
        g_strchug(line);
        gchar **split_line = g_strsplit_set(line, " \t", 2);
        // Check for season/episode string (s1e1)
        if (split_line[0][0] == 's') {
            gchar *season = split_line[0]+1;
            episode = g_strrstr(season, "e");
            if (episode != NULL) {
                episode[0] = '\0';
                episode++;
                gchar *endptr;
                list.array[list.count].number = g_ascii_strtoll(episode, &endptr, 10);
                if (endptr == episode) {
                    hbr_error("No season number found on line: %d", episode_filename,
                            NULL, NULL, NULL, list.count+1);
                    free_episode_list(list);
                    list.count = 0;
                    list.array = NULL;
                    break;
                }
                list.array[list.count].season =  g_ascii_strtoll(season, &endptr, 10);
                if (endptr == season) {
                    hbr_error("No episode number found on line: %d", episode_filename,
                            NULL, NULL, NULL, list.count+1);
                    free_episode_list(list);
                    list.count = 0;
                    list.array = NULL;
                    break;
                }
            }
        } else {
            // Check for episode string (e1) or digit only string
            if (split_line[0][0] == 'e') {
                episode = split_line[0]+1;
            } else {
                episode = split_line[0];
            }
            gchar *endptr;
            list.array[list.count].number = g_ascii_strtoll(episode, &endptr, 10);
            if (endptr == episode) {
                hbr_error("No episode number found on line: %d", episode_filename,
                        NULL, NULL, NULL, list.count+1);
                free_episode_list(list);
                list.count = 0;
                list.array = NULL;
                break;
            }
            list.array[list.count].season = -1;
        }

        // store the rest of the string as the episode name
        list.array[list.count].name = g_strdup(split_line[1]);
        if (list.array[list.count].name == NULL) {
            hbr_error("No episode name found on line: %d", NULL, NULL, NULL,
                    NULL, list.count+1);
            free_episode_list(list);
            list.count = 0;
            list.array = NULL;
            break;
        }
        g_strfreev(split_line);
        g_free(line);
        // Allocate more space if episodes is about to exceed max_count
        if ( list.count == max_count - 1 ) {
            struct episode *temp;
            if ((temp = realloc(list.array, (max_count+20) *
                            sizeof(struct episode))) != NULL){
                list.array = temp;
                max_count += 20;
            }
        }
        list.count++;
    }
    g_input_stream_close((GInputStream *)episode_list_file, NULL, NULL);
    g_object_unref(episode_list_file);
    return list;
}

/**
 * @brief Frees episode_array allocated by read_episode_list()
 *
 * @param list packed list of episodes
 */
static void free_episode_list(struct episode_list list)
{
    int i;
    for ( i = 0; i < list.count; i++) {
        free(list.array[i].name);
    }
    free(list.array);
    list.count = 0;
    list.array = NULL;
}

/**
 * @brief Generates GKeyFile template for hbr input
 *
 * @param outfiles_count Number of outfile sections to generate.
 *                       Ignored if episodes argument is given.
 * @param title DVD title number
 * @param season Season number
 * @param type Indicate a movie or series
 * @param iso_filename Source filename (iso file)
 * @param year Year published
 * @param crop Crop amount (T:B:L:R)
 * @param name General title for a series or movie
 * @param input_basedir Location for source file(s)
 * @param output_basedir Location for output file(s)
 * @param audio Audio track list (comma-separated)
 * @param subtitle Subtitle track list (comma-separated)
 * @param episodes Path for episode list file. Overrides outfiles_count.
 * @param chapters Chapter range (i.e. 2-18)
 *
 * @return GKeyFile pointer for the generated config
 */
GKeyFile *gen_hbr(gint outfiles_count, gint title, gint season, const gchar *type,
        const gchar *iso_filename, const gchar *year, const gchar *crop,
        const gchar *name, const gchar *input_basedir, const gchar *output_basedir,
        const gchar *audio, const gchar *subtitle, const gchar *chapters,
        const gchar *episodes)
{
    struct episode_list list;
    list.count = 0;
    list.array = NULL;
    if (episodes != NULL) {
        list = read_episode_list(episodes);
        outfiles_count = list.count;
        if (outfiles_count == 0) {
            hbr_error("Failed to parse episode list", NULL, NULL, NULL, NULL);
            free_episode_list(list);
            return NULL;
        }
    }
    if (outfiles_count <= 0 || outfiles_count > 999) {
        hbr_error("Invalid number of outfile sections (%d)", NULL, NULL, NULL,
                NULL, outfiles_count);
        free_episode_list(list);
        return NULL;
    }

    GKeyFile *config = g_key_file_new();
    const gchar *group = "CONFIG";

    gchar *inferred_type;
    if (episodes && type == NULL) {
        inferred_type = g_strdup("series");
    } else if (!type) {
        inferred_type = g_strdup("movie");
    } else {
        inferred_type = g_strdup(type);
    }
    gboolean is_movie = (g_strcmp0(inferred_type, "movie") == 0);
    gboolean is_series = (g_strcmp0(inferred_type, "series") == 0);

    // create CONFIG section with values
    // keys that are always in CONFIG
    g_key_file_set_value(config, group, "input_basedir", input_basedir ? input_basedir : "");
    g_key_file_set_value(config, group, "output_basedir", output_basedir ? output_basedir : "");
    if (is_movie || is_series) {
        g_key_file_set_value(config, group, "type", inferred_type);
    } else {
        hbr_error("Unknown type=%s. Should be \'movies\' or \'series\'", NULL,
                NULL, NULL, NULL, inferred_type);
        g_free(inferred_type);
        g_key_file_unref(config);
        free_episode_list(list);
        return NULL;
    }
    if (is_movie) {
        g_key_file_set_value(config, group, "year", year ? year : "");
    }
    g_key_file_set_value(config, group, "name", name ? name : "");

    // keys that are only in config if a value is given
    if (title) {
        g_key_file_set_integer(config, group, "title", title);
    }
    if (season) {
        g_key_file_set_integer(config, group, "season", season);
    }
    if (iso_filename) {
        g_key_file_set_value(config, group, "iso_filename", iso_filename);
    }
    if (crop) {
        g_key_file_set_value(config, group, "crop", crop);
    }
    if (audio) {
        g_key_file_set_value(config, group, "audio", audio);
    }
    if (subtitle) {
        g_key_file_set_value(config, group, "subtitle", subtitle);
    }
    if (chapters) {
        g_key_file_set_value(config, group, "chapters", chapters);
    }

    int i;
    for (i = 0; i< outfiles_count; i++) {
        if (episodes != NULL){
            create_outfile_section(config, i+1, list.array[i].number, title,
                    list.array[i].season, inferred_type, iso_filename, audio, subtitle,
                    chapters, list.array[i].name);
        } else {
            int episode;
            if (is_series) {
                episode = i;
            }
            if (is_movie) {
                episode = 0;
            }
            if (season != 0) {
                // season was already set in the CONFIG section
                season = -1;
            }
            create_outfile_section(config, i+1, episode, title, season, inferred_type,
                    iso_filename, audio, subtitle, chapters, "");
        }
    }
    if (episodes != NULL) {
        free_episode_list(list);
    }
    g_free(inferred_type);
    return config;
}

/**
 * @brief Create an outfile section in an GKeyfile given the parent.
 *
 * @param config a GKeyFile pointer to add this outfile section to
 * @param outfile_count
 * @param episode Episode number
 * @param title numeric dvd title (1-99)
 * @param season Season number
 * @param type 0 for movie, 1 for series, null pointer to remain empty
 * @param iso_filename path and filename for the source ISO
 * @param audio Comma separated list of audio track numbers
 * @param subtitle Comma separated list of subtitle track numbers
 * @param chapters Chapter range (i.e. 2-18)
 * @param specific_name Name of the episode or particular movie version
 */
static void create_outfile_section(GKeyFile *config, gint outfile_count, gint episode,
        gint title, gint season, const gchar *type, const gchar *iso_filename,
        const gchar *audio, const gchar *subtitle, const gchar *chapters,
        const gchar *specific_name)
{
    gboolean is_series = (g_strcmp0(type, "series") == 0);
    gchar *group = g_strdup_printf("OUTFILE%d", outfile_count);
    if (!iso_filename) {
        g_key_file_set_value(config, group, "iso_filename", "");
    }
    if (title == 0) {
        g_key_file_set_integer(config, group, "title", 0);
    }
    if (is_series) {
        if (season >= 0) {
            g_key_file_set_integer(config, group, "season", season);
        }
        if (episode) {
            g_key_file_set_integer(config, group, "episode", episode);
        }
    }
    g_key_file_set_value(config, group, "specific_name",
            specific_name ? specific_name : "");
    if (!chapters) {
        g_key_file_set_value(config, group, "chapters", "");
    }
    if (!audio) {
        g_key_file_set_value(config, group, "audio", "");
    }
    if (!subtitle) {
        g_key_file_set_value(config, group, "subtitle", "");
    }
    g_free(group);
}

/**
 * @brief Writes an GKeyFile to STDOUT with formatting and no short tags
 *
 * @param config pointer to a key file to be printed
 */
void print_hbr(GKeyFile *config)
{
    gchar *keyfile_string = g_key_file_to_data(config, NULL, NULL);
    g_print("%s", keyfile_string);
    g_free(keyfile_string);
}
