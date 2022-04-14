AC_INIT([C-Compiler],[0.1.0.0410_alpha],[zy_pan@zju.edu.cn])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([src/tokenizer.l])
AC_CONFIG_SRCDIR([src/parser.y])
AC_CONFIG_HEADERS([include/config.h])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_LEX([noyywrap])
AC_PROG_YACC
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([unordered_map])

if [[ "$TERM" = "xterm-256color" ]]; then
    AC_DEFINE([COLOR_TERM], [1], [Define to 1 if colorized output is enabled])
else
    AC_DEFINE([COLOR_TERM], [0], [Define to 1 if colorized output is enabled])
fi

CXXFLAGS="${CXXFLAGS} -std=c++17"

AH_BOTTOM([#include "_config.h"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
