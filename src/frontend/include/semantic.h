/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-13 18:47:20
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/include/semantic.h
 * @Description: 
 */

#pragma once

#include "ast_impl.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    int get_literal_type(const char* literal);
    int get_bin_expr_type(ast_node_ptr left, ast_node_ptr right);
    int get_un_expr_type(ast_node_ptr expr);
    
    void add_symbol(const char* name, int type);
    int get_symbol_type(const char* name);
    int get_type_size(int type_id);
    const char* get_function_type(const char* name);
    int is_declared(const char* name);
    void print_sym_tab();

#ifdef __cplusplus
}
#endif

void semantic_check(int* n_errs, ast_node_ptr root, int warning_flag);