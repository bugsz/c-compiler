#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "ast_impl.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define COLOR_RED ""
#define COLOR_GREEN ""
#define COLOR_NORMAL ""
#endif

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
        if(unlikely(node->n_child > _size)) {
            _size *= 2;
            node->child = realloc(node->child, sizeof(ast_node_ptr) * _size);
        }
        node->child[node->n_child - 1] = temp;
        temp->parent = node;
    }
    va_end(argp);
}

static void print_whitespaces(int n) {
    if (unlikely(n < 3)) {
        printf("`-");
        return;
    }
    printf("|");
    for (int i = 0; i < n - 3; i++) {
        printf(" ");
    }
    printf("`-");
}

static void print_node(ast_node_ptr node) {
    printf("%s %p\n", node->token, node);
}

void print_ast(ast_node_ptr node) {
    if (unlikely(node == NULL))
        return;
    static int tabs = 0;
    if (unlikely(node->parent == NULL))
        tabs = 0;
    print_node(node);
    if(likely(node->n_child > 0)) {
        tabs += 2;
        for (int i = 0; i < node->n_child; i++) {
            print_whitespaces(tabs);
            print_ast(node->child[i]);
        }
        tabs -= 2;
    }
}

static void shrink_to_fit(ast_node_ptr node) {
    if (node->n_child == 0) {
        free(node->child);
        node->child = NULL;
        return;
    }
    ast_node_ptr* new_child = malloc(sizeof(ast_node_ptr) * node->n_child);
    for (int i = 0; i < node->n_child; i++) {
        new_child[i] = node->child[i];
    }
    free(node->child);
    node->child = new_child;
}

static void shrink_ast_to_fit(ast_node_ptr node) {
    shrink_to_fit(node);
    for (int i = 0; i < node->n_child; i++) {
        shrink_ast_to_fit(node->child[i]);
    }
}

static void insert_node_idx(ast_node_ptr node, ast_node_ptr new_node, int idx) {
    ast_node_ptr* new_child = malloc(sizeof(ast_node_ptr) * (node->n_child + 1));
    for (int i = 0; i < idx; i++) {
        new_child[i] = node->child[i];
    }
    new_child[idx] = new_node;
    for (int i = idx; i < node->n_child; i++) {
        new_child[i + 1] = node->child[i];
    }
    free(node->child);
    node->child = new_child;
    node->n_child++;
}

static void merge_node(ast_node_ptr node) {
    if (unlikely(node->parent == NULL && strcmp(node->token, "TO_BE_MERGED") == 0)) {
        printf("Internal error\n");
        exit(1);
    }
    for (int i = 0;i < node->n_child;i++) {
        if (strcmp(node->child[i]->token, "TO_BE_MERGED") == 0) {
            ast_node_ptr to_delete = node->child[i];
            node->child[i] = to_delete->child[0];
            for (int j = 1;j < to_delete->n_child;j++) {
                insert_node_idx(node, to_delete->child[j], i + j);
            }
        }
    }
}

static void merge_node_from_root(ast_node_ptr node) {
    merge_node(node);
    for (int i = 0; i < node->n_child; i++) {
        merge_node_from_root(node->child[i]);
    }
}

static void reverse_array(ast_node_ptr* array, int n) {
    for (int i = 0; i < n / 2; i++) {
        ast_node_ptr temp = array[i];
        array[i] = array[n - i - 1];
        array[n - i - 1] = temp;
    }
}

void postproc_after_parse(ast_node_ptr root) {
    shrink_ast_to_fit(root);
    merge_node_from_root(root);
    reverse_array(root->child, root->n_child);
}