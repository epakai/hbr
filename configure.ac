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

# Doxygen for source code documentation
AC_CHECK_PROGS([DOXYGEN], [doxygen])
AS_IF([test -z "$DOXYGEN"],
      [AC_MSG_WARN([Doxygen not found - Source documentation cannot be generated])],
      [
       # Dot for code graphs in source documentation
       AC_CHECK_PROGS([GRAPHVIZ], [dot])
       AS_IF([test -z "$GRAPHVIZ"],
             [
              AC_MSG_WARN([Graphviz not found - Source documentation will not have call graphs])
              AC_SUBST([CALL_GRAPH], ["no"])
              ],
              AC_SUBST([CALL_GRAPH], ["yes"])
              )

       # Markdown to convert README text to markdown for inclusion in doxygen output
       AC_CHECK_PROGS([MARKDOWN], [markdown])
       AS_IF([test -z "$MARKDOWN"],
             [
              AC_MSG_WARN([Markdown not found - Source documentation will not include README])
              AC_SUBST([MAINPAGE_MDFILE], [""])
              ],
              AC_SUBST([MAINPAGE_MDFILE], ["README.md"])
              )

       AC_ARG_ENABLE([doxygen-html],
                     AS_HELP_STRING([--disable-doxygen-html], [Do not generate HTML documentation]))
       AC_ARG_ENABLE([doxygen-xml],
                     AS_HELP_STRING([--enable-doxygen-xml], [Generate XML documentation]))
       AC_ARG_ENABLE([doxygen-rtf],
                     AS_HELP_STRING([--enable-doxygen-rtf], [Generate RTF documentation]))
       AC_ARG_ENABLE([doxygen-latex],
                     AS_HELP_STRING([--enable-doxygen-latex], [Generate LaTeX documentation]))
       ])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([doc/Makefile doc/Doxyfile])])
AS_IF([test "$enable_doxygen_html" != "no"], AC_SUBST([GENERATE_HTML], ["yes"]), 
AC_SUBST([GENERATE_HTML], ["no"]))
AS_IF([test "$enable_doxygen_xml" = "yes"], AC_SUBST([GENERATE_XML], ["yes"]), 
AC_SUBST([GENERATE_XML], ["no"]))
AS_IF([test "$enable_doxygen_rtf" = "yes"], AC_SUBST([GENERATE_RTF], ["yes"]), 
AC_SUBST([GENERATE_RTF], ["no"]))
AS_IF([test "$enable_doxygen_latex" = "yes"], AC_SUBST([GENERATE_LATEX], ["yes"]), 
AC_SUBST([GENERATE_LATEX], ["no"]))

# Checks for libraries.
AC_CHECK_LIB(glib-2.0, g_keyfile_new)
PKG_CHECK_MODULES(GLIB2, glib-2.0)
AC_CHECK_LIB(gio-2.0, g_data_input_stream_new)
PKG_CHECK_MODULES(GIO2, gio-2.0)
AC_CHECK_LIB(gobject-2.0, g_object_unref)
PKG_CHECK_MODULES(GOBJECT2, gobject-2.0)

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
