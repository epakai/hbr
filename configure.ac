#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([hbr], [0.1], [https://github.com/epakai/hbr/issues])
AC_CONFIG_SRCDIR([src/hbr.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL

# Checks for libraries.
PKG_CHECK_MODULES(GLIB2, glib-2.0)
PKG_CHECK_MODULES(GIO2, gio-2.0)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([dup2 getcwd strchr strerror strtol])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
