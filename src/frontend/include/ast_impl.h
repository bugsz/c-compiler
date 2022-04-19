/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-10 01:51:21
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/include/ast_impl.h
 * @Description: 
 */

#pragma once
#include <stdarg.h>

#define INIT_CHILD_NUM 10
#define MAX_TOKEN_LEN 128

#define mknode(token, ...) mknode_impl(token, ##__VA_ARGS__, NULL)
#define append_child(parent, ...) append_child_impl(parent, ##__VA_ARGS__, NULL)

typedef struct ast_node_impl ast_node_t;
typedef ast_node_t* ast_node_ptr;
typedef struct ast_yyltype ast_loc_t;

struct ast_yyltype {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
    char filename[MAX_TOKEN_LEN];
};

struct ast_node_impl {
    char token[MAX_TOKEN_LEN]; // type of node
    ast_node_ptr* child;
    ast_node_ptr parent;
    int n_child; // only use n_child to determine leaf node or not

    ast_loc_t pos; // line number and column number

    char val[MAX_TOKEN_LEN]; // name of node
    int type_id;
};

#ifdef __cplusplus
extern "C" {
#endif
    ast_node_ptr mknode_impl(const char* token, ...);
    void append_child_impl(ast_node_ptr node, ...);
    void print_ast(ast_node_ptr node);
    void postproc_after_parse(ast_node_ptr root);
#ifdef __cplusplus
}
#endif