/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-11 08:58:07
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/src/ast_impl.c
 * @Description: 
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "ast_impl.h"
#include "semantic.h"
#include "config.h"

const char* typeid_deref[] = {
    "void", "char", "short", "int", "long", "float", "double", "string",
    "void *", "char *", "short *", "int *", "long *", "float *", "double *"
};

ast_node_ptr mknode_impl(const char* token, ...) {
    ast_node_ptr node = malloc(sizeof(ast_node_t)), temp;
    memset(node->token, 0, sizeof(node->token));
    memset(node->val, 0, sizeof(node->val));
    memset(&(node->pos), 0, sizeof(node->pos));
    strncpy(node->token, token, MAX_TOKEN_LEN);
    node->type_id = 0;
    node->n_child = 0;
    node->child = malloc(sizeof(ast_node_ptr) * INIT_CHILD_NUM);
    memset(node->child, 0, sizeof(node->child) * INIT_CHILD_NUM);
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
    unsigned int _size = INIT_CHILD_NUM;
    ast_node_ptr temp;
    va_list argp;
    va_start(argp, node);
    while ((temp = va_arg(argp, void*))) {
        node->n_child++;
        if(unlikely(node->n_child > _size)) {
            _size *= 5;
            ast_node_ptr* new_node = realloc(node->child, sizeof(ast_node_ptr) * _size);
            node->child = new_node;
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
    char position[MAX_TOKEN_LEN] = { 0 }, type[MAX_TOKEN_LEN] = {0};
    if (node->pos.last_line * node->pos.last_column) {
        sprintf(position, " <%d:%d>", node->pos.last_line, node->pos.last_column);
    }
    if (strcmp(node->token, "TranslationUnitDecl") == 0 ||
        strcmp(node->token, "CompoundStmt") == 0 ||
        strcmp(node->token, "DeclStmt") == 0 ||
        strcmp(node->token, "ForStmt") == 0 ||
        strcmp(node->token, "IfStmt") == 0 ||
        strcmp(node->token, "WhileStmt") == 0 ||
        strcmp(node->token, "DoStmt") == 0 ||
        strcmp(node->token, "ForDelimiter") == 0 ||
        strcmp(node->token, "VariadicParms") == 0 ||
        strcmp(node->token, "NullStmt") == 0 ||
        strcmp(node->token, "InitializerList") == 0) {
    } else {
        sprintf(type, " '%s'", strcmp(node->token, "FunctionDecl") == 0 ? get_function_type(node->val) : typeid_deref[node->type_id]);
    }
    printf("%s %p%s %s%s\n", node->token, node, position, node->val, type);
}

static void print_ast_impl(ast_node_ptr node) {
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
            print_ast_impl(node->child[i]);
        }
        tabs -= 2;
    }
}

void print_ast(ast_node_ptr root) {
    printf("------------------------------------------\n");
    printf("Abstract Syntax Tree\n");
    printf("------------------------------------------\n");
    print_ast_impl(root);
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
            for (int t = 0;t < node->n_child;t++) {
                node->child[t]->parent = node;
            }
        }
    }
}

static void merge_node_from_root(ast_node_ptr node) {
    if (node == NULL) return;
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
    merge_node_from_root(root);
    reverse_array(root->child, root->n_child);
}