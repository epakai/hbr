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


static void hbr_verror(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, va_list argp);
static void hbr_vwarn(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, va_list argp);
static void hbr_vinfo(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, va_list argp);

static enum message_level {
    HBR_INFO,
    HBR_WARN,
    HBR_ERROR
} MESSAGE_LEVEL = HBR_INFO;

/// Only print info or worse messages
void message_level_info(void) { MESSAGE_LEVEL = HBR_INFO; }
/// Only print warning  or worse messages
void message_level_warn(void) { MESSAGE_LEVEL = HBR_WARN; }
/// Only print error or worse messages
void message_level_error(void) { MESSAGE_LEVEL = HBR_ERROR; }

void hbr_print_message(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, va_list argp);
/**
 * @brief Message printing routine for hbr_verror(),
 *        hbr_vwarn(), hbr_vinfo()
 */
void hbr_print_message(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, va_list argp) {
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    vfprintf(stderr, format, argp);
    if (path) {
        fprintf(stderr, ": (%s)", path);
    }
    if (section) {
        fprintf(stderr, " [%s]", section);
    }
    if (key) {
        if (value) {
            fprintf(stderr, " %s=%s\n", key, value);
        } else {
            fprintf(stderr, " %s= \n", key);
        }
    } else {
        fprintf(stderr, "\n");
    }
}

/**
 * @brief Error message printed on stderr. Resulting output:
 *        hbr   ERROR: Format string: (path) [section] key=value
 *
 * @param format  Format string to be printed
 * @param path    intended for a relevant file path
 * @param section intended for the group/section of a keyfile
 * @param key     intended for the relevant key name
 * @param value   intended for the key name's value (if it is defined)
 * @param ...     args to fill placeholders in format
 */
void hbr_error(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, ...)
{
    va_list argp;
    va_start(argp, value);
    hbr_verror(format, path, section, key, value, argp);
    va_end(argp);
}

/**
 * @brief Variadic implementation for hbr_error
 */
    __attribute__((__format__ (__printf__, 1, 0)))
static void hbr_verror(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, va_list argp)
{
    if (MESSAGE_LEVEL <= HBR_ERROR) {
        fprintf(stderr, "hbr   ERROR: ");
        hbr_print_message(format, path, section, key, value, argp);
    }
}

/**
 * @brief Warning message printed on stderr. Resulting output:
 *        hbr WARNING: Format string: (path) [section] key=value
 *
 * @param format  Format string to be printed
 * @param path    intended for a relevant file path
 * @param section intended for the group/section of a keyfile
 * @param key     intended for the relevant key name
 * @param value   intended for the key name's value (if it is defined)
 * @param ...     args to fill placeholders in format
 */
void hbr_warn(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, ...)
{
    va_list argp;
    va_start(argp, value);
    hbr_vwarn(format, path, section, key, value, argp);
    va_end(argp);
}

/**
 * @brief Variadic implementation for hbr_warn
 */
    __attribute__((__format__ (__printf__, 1, 0)))
static void hbr_vwarn(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, va_list argp)
{
    if (MESSAGE_LEVEL <= HBR_WARN) {
        fprintf(stderr, "hbr WARNING: ");
        hbr_print_message(format, path, section, key, value, argp);
    }
}

/**
 * @brief Informational message printed on stderr. Resulting output:
 *        hbr    INFO: Format string: (path) [section] key=value
 *
 * @param format  Format string to be printed
 * @param path    intended for a relevant file path
 * @param section intended for the group/section of a keyfile
 * @param key     intended for the relevant key name
 * @param value   intended for the key name's value (if it is defined)
 * @param ...     args to fill placeholders in format
 */
void hbr_info(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, ...)
{
    va_list argp;
    va_start(argp, value);
    hbr_vinfo(format, path, section, key, value, argp);
    va_end(argp);
}

/**
 * @brief Variadic implementation for hbr_info
 */
    __attribute__((__format__ (__printf__, 1, 0)))
static void hbr_vinfo(const gchar *format, const gchar *path, const gchar *section,
        const gchar *key, const gchar *value, va_list argp)
{
    if (MESSAGE_LEVEL <= HBR_INFO) {
        fprintf(stderr, "hbr    INFO: ");
        hbr_print_message(format, path, section, key, value, argp);
    }
}

/**
 * @brief Opens a file as a data stream for reading
 *
 * @param infile path of file to be read
 *
 * @return data stream, free'd with g_object_unref()
 */
GDataInputStream * open_datastream(const gchar *infile)
{
    GFile *file = g_file_new_for_path(infile);
    GError *error = NULL;
    GFileInputStream *filestream = g_file_read(file, NULL, &error);
    g_object_unref(file);
    if (error != NULL) {
        if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
            hbr_error("File not found", infile, NULL, NULL, NULL);
        } else if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY)) {
            hbr_error("File specified is a directory", infile, NULL, NULL, NULL);
        } else {
            hbr_error("File not readable", infile, NULL, NULL, NULL);
        }
        g_error_free(error);
        return NULL;
    }
    GDataInputStream *datastream = g_data_input_stream_new((GInputStream *) filestream);
    g_object_unref(filestream);
    return datastream;
}
