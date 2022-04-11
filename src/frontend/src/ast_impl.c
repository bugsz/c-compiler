#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ast_impl.h"

ast_node_ptr mknode_impl(const char* token, ...) {
    ast_node_ptr node = malloc(sizeof(ast_node_t)), temp;
    strncpy(node->token, token, MAX_TOKEN_LEN);
    node->n_child = 0;
    node->child = malloc(sizeof(ast_node_ptr) * INIT_CHILD_NUM);
    node->parent = NULL;
    va_list argp;
    va_start(argp, token);
    while ((temp = va_arg(argp, void*))) {
        append_child(node, temp);
    }
    va_end(argp);
    return node;
}

void append_child_impl(ast_node_ptr node, ...) {
    unsigned int _size = sizeof(node->child) / sizeof(ast_node_ptr);
    ast_node_ptr temp;
    va_list argp;
    va_start(argp, node);
    while ((temp = va_arg(argp, void*))) {
        node->n_child++;
        if(node->n_child > _size) {
            _size *= 2;
            node->child = realloc(node->child, sizeof(ast_node_ptr) * _size);
        }
        node->child[node->n_child - 1] = temp;
        node->child[node->n_child - 1]->parent = node;
    }
    va_end(argp);
}

static void print_whitespaces(int n) {
    for(int i = 0; i < n; i++) {
        printf(" ");
    }
}

void print_node(ast_node_ptr node) {
    printf("%s\n", node->token);
}

void print_ast(ast_node_ptr node) {
    if (node == NULL)
        return;
    static int tabs = 0;
    if (node->parent == NULL)
        tabs = 0;
    print_node(node);
    if(node->n_child > 0) {
        tabs += 2;
        for (int i = 0; i < node->n_child; i++) {
            print_whitespaces(tabs);
            print_ast(node->child[i]);
        }
        tabs -= 2;
    }
}