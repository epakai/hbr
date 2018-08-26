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
#include "keyfile.h"

GString * build_args(struct outfile outfile, struct config config);
GString * build_filename(struct outfile outfile, struct config config,
        gboolean full_path);
gboolean strcmp_list(gchar *s, gchar **list, gsize len);
gboolean valid_bit_rate(int bitrate, int minimum, int maximum);

#endif
