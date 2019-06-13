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

#include <stdio.h>

#include "util.h"

// TODO i think path and section should follow key/value

void hbr_error(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, ...)
{
    va_list argp;
    va_start(argp, value);
    hbr_verror(format, path, section, key, value, argp);
    va_end(argp);
}

void hbr_verror(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, va_list argp)
{
    fprintf(stderr, "hbr   ERROR: ");
    vfprintf(stderr, format, argp);
    if (path) {
        fprintf(stderr, ": (%s)", path);
    }
    if (section) {
        fprintf(stderr, " [%s]", section);
    }
    if (key) {
        if (value) {
            fprintf(stderr, " %s=%s.\n", key, value);
        } else {
            fprintf(stderr, " %s= .\n", key);
        }
    } else {
        fprintf(stderr, "\n");
    }
}

void hbr_warn(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, ...)
{
    va_list argp;
    va_start(argp, value);
    hbr_vwarn(format, path, section, key, value, argp);
    va_end(argp);
}

void hbr_vwarn(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, va_list argp)
{
    fprintf(stderr, "hbr WARNING: ");
    vfprintf(stderr, format, argp);
    if (path) {
        fprintf(stderr, ": (%s)", path);
    }
    if (section) {
        fprintf(stderr, " [%s]", section);
    }
    if (key) {
        if (value) {
            fprintf(stderr, " %s=%s.\n", key, value);
        } else {
            fprintf(stderr, " %s= .\n", key);
        }
    } else {
        fprintf(stderr, "\n");
    }
}

void hbr_info(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, ...)
{
    va_list argp;
    va_start(argp, value);
    hbr_vinfo(format, path, section, key, value, argp);
    va_end(argp);
}

void hbr_vinfo(gchar *format, gchar *path, gchar *section, gchar *key,
        gchar *value, va_list argp)
{
    fprintf(stderr, "hbr    INFO: ");
    vfprintf(stderr, format, argp);
    if (path) {
        fprintf(stderr, ": (%s)", path);
    }
    if (section) {
        fprintf(stderr, " [%s]", section);
    }
    if (key) {
        if (value) {
            fprintf(stderr, " %s=%s.\n", key, value);
        } else {
            fprintf(stderr, " %s= .\n", key);
        }
    } else {
        fprintf(stderr, "\n");
    }
}
