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
    int n_child;

    int ln, col; // line number and column number

    char name[MAX_TOKEN_LEN]; // name of node
    char attr[MAX_TOKEN_LEN];
    union {
        char op_type[3];
        char char_val;
        short short_val;
        int int_val;
        long long_val;
        float float_val;
        double double_val;
    } literal_val;
};

struct sym_tab_impl {

};

#ifdef __cplusplus
extern "C" {
#endif
    ast_node_ptr mknode_impl(const char* token, ...);
    void append_child_impl(ast_node_ptr node, ...);
    void print_node(ast_node_ptr node);
    void print_ast(ast_node_ptr node);
#ifdef __cplusplus
}
#endif