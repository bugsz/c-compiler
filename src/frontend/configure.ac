AC_INIT([C-Compiler],[0.1.0.0410_alpha],[zy_pan@zju.edu.cn])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADERS([include/config.h])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LEX([noyywrap])
AC_PROG_YACC
AC_PROG_MAKE_SET

AC_FUNC_ALLOCA
AC_CHECK_HEADERS([inttypes.h libintl.h malloc.h unistd.h])

AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strdup])

if [[ "$TERM" = "xterm-256color" ]]; then
    AC_DEFINE([COLOR_TERM], [1], [Define to 1 if colorized output is enabled])
else
    AC_DEFINE([COLOR_TERM], [0], [Define to 1 if colorized output is enabled])
fi

CXXFLAGS="${CXXFLAGS} -std=c++17"

AH_BOTTOM([#include "_config.h"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
