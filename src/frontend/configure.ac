AC_PREREQ([2.71])
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

CXXFLAGS="${CXXFLAGS} -std=c++17"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
