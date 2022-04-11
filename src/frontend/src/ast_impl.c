#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ast_impl.h"

ast_node_ptr mknode_impl(const char* token, ...) {
    int _init_child_num = INIT_CHILD_NUM;
    ast_node_ptr node = malloc(sizeof(ast_node_t)), temp;
    strncpy(node->token, token, MAX_TOKEN_LEN);
    node->n_child = 0;
    node->child = malloc(sizeof(ast_node_ptr) * _init_child_num);
    va_list argp;
    va_start(argp, token);
    while ((temp = va_arg(argp, void*))) {
        node->n_child++;
        if(node->n_child > _init_child_num) {
            _init_child_num *= 2;
            node->child = realloc(node->child, sizeof(ast_node_ptr) * _init_child_num);
        }
        node->child[node->n_child - 1] = temp;
    }
    va_end(argp);
    return node;
}

void print_ast(ast_node_ptr node) {
    if(node == NULL) {
        return;
    }
    printf("%s", node->token);
    if(node->n_child > 0) {
        printf("(");
        for(int i = 0; i < node->n_child; i++) {
            print_ast(node->child[i]);
            if(i != node->n_child - 1) {
                printf(",");
            }
        }
        printf(")");
    }
}