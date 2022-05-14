/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-11 16:52:51
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/include/_config.h
 * @Description: 
 */

#pragma once

#if !HAVE_UNISTD_H
#error Incompatible operating system. <unistd.h> required
#endif

#define parse yyparse

#if COLOR_TERM == 1
#define COLOR_RED "\033[1;31m"
#define COLOR_NORMAL "\033[0m"
#define COLOR_BOLD "\033[1m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_CYAN "\033[1;36m"
#define COLOR_PURPLE "\033[1;35m"
#else
#define COLOR_RED ""
#define COLOR_NORMAL ""
#define COLOR_BOLD ""
#define COLOR_GREEN ""
#define COLOR_CYAN ""
#define COLOR_PURPLE ""
#endif

#if defined __GNUG__ || defined __clang__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

enum TYPEID {
    TYPEID_OVERFLOW = -1,
    TYPEID_VOID,
    TYPEID_CHAR,
    TYPEID_SHORT,
    TYPEID_INT,
    TYPEID_LONG,
    TYPEID_FLOAT,
    TYPEID_DOUBLE,
    TYPEID_VOID_PTR,
    TYPEID_CHAR_PTR,
    TYPEID_SHORT_PTR,
    TYPEID_INT_PTR,
    TYPEID_LONG_PTR,
    TYPEID_FLOAT_PTR,
    TYPEID_DOUBLE_PTR,
    TYPEID_VOID_PPTR,
    TYPEID_CHAR_PPTR,
    TYPEID_SHORT_PPTR,
    TYPEID_INT_PPTR,
    TYPEID_LONG_PPTR,
    TYPEID_FLOAT_PPTR,
    TYPEID_DOUBLE_PPTR,
    TYPEID_STR
};

#ifdef __APPLE__
#include <sys/resource.h>
#define COREDUMP { \
    struct rlimit rl; \
    getrlimit(RLIMIT_CORE, &rl); \
    rl.rlim_cur = rl.rlim_max; \
    setrlimit(RLIMIT_CORE, &rl); \
}
#else
#define COREDUMP ;
#endif

#include "ast_impl.h"

#ifdef __cplusplus
#include <string>
struct lib_frontend_ret {
    int n_errs;
    std::string input_file;
    std::string output_file;
    ast_node_ptr root;
};
#endif