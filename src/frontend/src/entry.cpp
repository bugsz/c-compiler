/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-13 00:13:08
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/src/entry.cpp
 * @Description: 
 */

#include <iostream>
#include <cstdlib>
#include "argparse.hpp"

#include "ast_impl.h"
#include "semantic.h"
#include "config.h"

extern "C" {
    extern int yyparse(const char*, int*, ast_node_ptr);
    extern FILE* yyin;
}

using namespace std;
using namespace argparse;

int main(int argc, char** argv) {
    COREDUMP
    ArgumentParser program(PACKAGE_NAME, VERSION);
    program.add_argument("-o", "--output")
        .default_value(string("a.out"))
        .help("output file");
    program.add_argument("-f", "--file")
        .default_value(string(""))
        .help("input file");
    program.add_argument("--ast-dump")
        .help("print AST")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--sym-dump")
        .help("print symbol table")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-w")
        .help("disable warnings")
        .default_value(false)
        .implicit_value(true);
    int* n_errs = new int;
    try {
        program.parse_args(argc, argv);
        string filename = program.get("-f");
        program.is_used("-f") ?
            (filename == "" ? throw runtime_error("Need value for -f")
                : yyin = fopen(filename.c_str(), "r"))
            : yyin = stdin;
        if (!yyin)  throw runtime_error("File not found");
        ast_node_ptr root = mknode("TranslationUnitDecl");
        *n_errs = 0;
        parse(filename.c_str(), n_errs, root);
        semantic_check(filename.c_str(), n_errs, root, program["-w"] == true);
        if (program["--ast-dump"] == true) print_ast(filename.c_str(), root);
        if (program["--sym-dump"] == true) print_sym_tab();
        if (*n_errs > 0) {
            throw runtime_error(to_string(*n_errs) + " error(s) generated.");
        }
    }
    catch (const exception& err) {
        cerr << err.what() << endl;
        exit(1);
    }
    return *n_errs;
}