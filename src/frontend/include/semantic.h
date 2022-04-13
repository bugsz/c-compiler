#pragma once

#include "ast_impl.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    int get_literal_type(const char* literal);
    void add_symbol(const char* name, int type);
    int get_symbol_type(const char* name);
    int is_declared(const char* name);
    void print_sym_tab();

#ifdef __cplusplus
}
#endif

void semantic_check(const char* filename, int* n_errs, ast_node_ptr root);