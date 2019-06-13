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

#ifndef _util_h
#define _util_h

#include <glib.h>
#include <stdarg.h>

void hbr_error(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, ...);
void hbr_verror(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, va_list argp);

void hbr_warn(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, ...);
void hbr_vwarn(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, va_list argp);

void hbr_info(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, ...);
void hbr_vinfo(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, va_list argp);

#endif
