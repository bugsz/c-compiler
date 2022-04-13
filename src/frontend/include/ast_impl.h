#pragma once
#include <stdarg.h>

#define INIT_CHILD_NUM 10
#define MAX_TOKEN_LEN 50

#define mknode(token, ...) mknode_impl(token, ##__VA_ARGS__, NULL)
#define append_child(parent, ...) append_child_impl(parent, ##__VA_ARGS__, NULL)

typedef struct ast_node_impl ast_node_t;
typedef ast_node_t* ast_node_ptr;

struct ast_node_impl {
    char token[MAX_TOKEN_LEN]; // type of node
    ast_node_ptr* child;
    ast_node_ptr parent;
    int n_child; // only use n_child to determine leaf node or not

    int ln, col; // line number and column number

    char val[MAX_TOKEN_LEN]; // name of node
    char attr[MAX_TOKEN_LEN];
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