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

lib_frontend_ret frontend_entry(int argc, const char** argv) {
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
    ast_node_ptr root;
    string input_file, output_file;
    try {
        program.parse_args(argc, argv);
        input_file = program.get("-f");
        output_file = program.get("-o");
        program.is_used("-f") ?
            (input_file == "" ? throw runtime_error("Need value for -f")
                : yyin = fopen(input_file.c_str(), "r"))
            : yyin = stdin;
        if (!yyin)  throw runtime_error("File not found");
        root = mknode("TranslationUnitDecl");
        *n_errs = 0;
        parse(input_file.c_str(), n_errs, root);
        semantic_check(input_file.c_str(), n_errs, root, program["-w"] == true);
        if (program["--ast-dump"] == true) print_ast(input_file.c_str(), root);
        if (program["--sym-dump"] == true) print_sym_tab();
        if (*n_errs > 0) {
            throw runtime_error(to_string(*n_errs) + " error(s) generated.");
        }
    }
    catch (const exception& err) {
        cerr << err.what() << endl;
    }
    return {*n_errs, input_file, output_file, root};
}