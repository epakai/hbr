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

#ifndef _gen_hbr_h
#define _gen_hbr_h

#include <glib.h>

/**
 * @brief Episode structure used to build episode list when -l option is used.
 */
struct episode {
	gint number;
    gint season;
	gchar *name;
};

/**
 * @brief
 */
struct episode_list {
	gint count;
	struct episode *array;
};

struct episode_list read_episode_list(const gchar *episode_filename);

void free_episode_list(struct episode_list list);

GKeyFile *gen_hbr(gint outfiles_count, gint title, gint season, const gchar *type,
		const gchar *iso_filename, const gchar *year, const gchar *crop,
        const gchar *name, const gchar *input_basedir, const gchar *output_basedir,
        const gchar *audio, const gchar *subtitle, const gchar *chapters,
        const gchar *episodes);

void create_outfile_section(GKeyFile *config, gint outfile_count, gint episode,
        gint title, gint season, const gchar *type, const gchar *iso_filename,
        const gchar *audio, const gchar *subtitle, const gchar *chapters,
        const gchar *specific_name);

void print_hbr(GKeyFile *config);

#endif
