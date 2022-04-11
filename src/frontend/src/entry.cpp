#include <iostream>
#include <cstdlib>
#include "argparse.hpp"

#include "ast.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define PACKAGE_NAME "c-compiler"
#define VERSION "unknown"
#endif

extern "C" {
    extern int yyparse(const char*, int*, ast_node_ptr);
    extern FILE* yyin;
}

using namespace std;
using namespace argparse;

int main(int argc, char** argv) {
    ArgumentParser program(PACKAGE_NAME, VERSION);
    program.add_argument("-o", "--output")
        .default_value(string("a.out"))
        .help("output file");
    program.add_argument("-f", "--file")
        .default_value(string(""))
        .help("input file");
    program.add_argument("-ast-dump")
        .default_value(string(""))
        .help("print AST");
    try {
        program.parse_args(argc, argv);
        program.is_used("-f") ?
            (program.get("-f") == "" ? throw runtime_error("Need value for -f")
                : yyin = fopen(program.get("-f").c_str(), "r"))
            : yyin = stdin;
        if (!yyin)  throw runtime_error("File not found");
        ast_node_ptr root = mknode("TranslationUnitDecl");
        int* n_errs = new int;
        *n_errs = 0;
        if (yyparse(program.get("-f").c_str(), n_errs, root)) throw runtime_error(to_string(*n_errs) + " errors generated.");
    }
    catch (const exception& err) {
        cerr << err.what() << endl;
        exit(1);
    }
    return 0;
}