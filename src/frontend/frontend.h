#pragma once
#include <string>

#define MAX_TOKEN_LEN 128

typedef struct ast_node_impl ast_node_t;
typedef ast_node_t* ast_node_ptr;
typedef struct ast_yyltype ast_loc_t;

struct lib_frontend_ret {
    int n_errs;
    std::string input_file;
    std::string output_file;
    ast_node_ptr root;
};

struct ast_yyltype {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
    char filename[MAX_TOKEN_LEN];
};

enum TYPEID {
    TYPEID_OVERFLOW = -1,
    TYPEID_VOID,
    TYPEID_CHAR,
    TYPEID_SHORT,
    TYPEID_INT,
    TYPEID_LONG,
    TYPEID_FLOAT,
    TYPEID_DOUBLE,
    TYPEID_STR,
    TYPEID_VOID_PTR,
    TYPEID_CHAR_PTR,
    TYPEID_SHORT_PTR,
    TYPEID_INT_PTR,
    TYPEID_LONG_PTR,
    TYPEID_FLOAT_PTR,
    TYPEID_DOUBLE_PTR,
    TYPEID_VOID_PPTR,
    TYPEID_CHAR_PPTR,
    TYPEID_SHORT_PPTR,
    TYPEID_INT_PPTR,
    TYPEID_LONG_PPTR,
    TYPEID_FLOAT_PPTR,
    TYPEID_DOUBLE_PPTR,
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

lib_frontend_ret frontend_entry(int argc, const char** argv);

#ifdef __cplusplus
extern "C" {
#endif
    
    void print_ast(ast_node_ptr node);
    void print_sym_tab();
    
#ifdef __cplusplus
}
#endif