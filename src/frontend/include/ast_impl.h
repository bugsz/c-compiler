#pragma once
#include <stdarg.h>

#define INIT_CHILD_NUM 10
#define MAX_TOKEN_LEN 50

#define mknode(token, ...) mknode_impl((token), ##__VA_ARGS__, NULL)

typedef struct ast_node_impl ast_node_t;
typedef ast_node_t* ast_node_ptr;

struct ast_node_impl {
    char token[MAX_TOKEN_LEN];
    ast_node_ptr* child;
    int n_child;
};

extern "C" {
    ast_node_ptr mknode_impl(const char* token, ...);
    void print_ast(ast_node_ptr node);
}