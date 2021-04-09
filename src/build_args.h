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

#ifndef _build_args_h
#define _build_args_h

#include <glib.h>
#include <gio/gio.h>

#include "options.h"

GPtrArray * build_args(GKeyFile *config, const gchar *group, gboolean quoted);
void build_arg_string(GKeyFile *config, const gchar *group, GPtrArray *args,
        gint i, gboolean param_quoted);
void build_arg_boolean(GKeyFile *config, const gchar *group, GPtrArray *args,
        gint i);
void build_arg_integer(GKeyFile *config, const gchar *group, GPtrArray *args,
        gint i);
void build_arg_double(GKeyFile *config, const gchar *group, GPtrArray *args,
        gint i);
void build_arg_string_list(GKeyFile *config, const gchar *group,
        GPtrArray *args, gint i, gboolean param_quoted);
void build_arg_integer_list(GKeyFile *config, const gchar *group,
        GPtrArray *args, gint i);
void build_arg_double_list(GKeyFile *config, const gchar *group,
        GPtrArray *args, gint i);
gchar *build_filename(GKeyFile *config, const gchar *group);
void append_year(GKeyFile *config, const gchar *group, GString *path);

#endif
