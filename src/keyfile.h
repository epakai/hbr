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

#ifndef _keyfile_h
#define _keyfile_h
#include <glib.h>

GKeyFile * parse_key_file(char *infile);
GKeyFile * copy_group_new(GKeyFile *keyfile, gchar *group, gchar *new_group);
GKeyFile * merge_key_group(GKeyFile *pref, gchar *p_group, GKeyFile *alt,
        gchar *a_group, gchar* new_group);
void remove_conflicts(gchar *, GKeyFile *, gchar *, GKeyFile *, gchar *);
GKeyFile * generate_default_key_file();

gint get_outfile_count(GKeyFile *keyfile);
gchar ** get_outfile_list(GKeyFile *keyfile, gsize *outfile_count);
gchar * get_group_from_episode(GKeyFile *keyfile, int episode_number);
gboolean validate_input_file(gchar *infile);
gboolean validate_config_file(gchar *infile);
#endif
