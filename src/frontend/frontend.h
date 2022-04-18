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
};

enum TYPEID {
    TYPEID_VOID = 0,
    TYPEID_CHAR,
    TYPEID_SHORT,
    TYPEID_INT,
    TYPEID_LONG,
    TYPEID_FLOAT,
    TYPEID_DOUBLE,
    TYPEID_STR
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
    
    void print_ast(const char* filename, ast_node_ptr node);
    void print_sym_tab();
    
#ifdef __cplusplus
}
#endif