#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <cassert>
#include "argparse.hpp"

#include "preprocessor.h"
#include "exception.h"
#include "ast_impl.h"
#include "semantic.h"
#include "config.h"

extern "C" {
    extern int yyparse(int*, ast_node_ptr);
    extern FILE* yyin;
}

using namespace std;
using namespace argparse;

char global_filename[256];

lib_frontend_ret frontend_entry(int argc, const char** argv) {
    #ifdef DEBUG
    COREDUMP
    #endif
    ArgumentParser program(PACKAGE_NAME, VERSION);
    program.add_argument("-o", "--output")
        .default_value(string("a.out"))
        .help("specify output file name");
    program.add_argument("-f", "--file")
        .required()
        .help("specify source file name");
    program.add_argument("-E")
        .default_value(false)
        .implicit_value(true)
        .help("preprocess only");
    program.add_argument("-ast-dump", "--ast-dump")
        .help("print AST")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-sym-dump", "--sym-dump")
        .help("print symbol table")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-w", "-fno-warnings", "--fno-warnings")
        .help("disable warnings")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-fno-preprocess", "--fno-preprocess")
        .help("disable preprocessing")
        .default_value(false)
        .implicit_value(true);
    int* n_errs = new int;
    string filename;
    ast_node_ptr root;
    try {
        program.parse_args(argc, argv);
        parse_defines(argc, argv);
        filename = program.get("-f");
        string pp_filename;
        pp_filename = program["--fno-preprocess"] == false ? preprocess(filename) : filename;
        strcpy(global_filename, pp_filename.c_str());
        if (program["-E"] == true && program["--fno-preprocess"] == false) {
            fstream bin(pp_filename, ios::in);
            assert(bin.is_open());
            while (!bin.eof()) {
                string line;
                getline(bin, line);
                cout << line << endl;
            }
            bin.close();
            exit(0);
        }
        yyin = fopen(pp_filename.c_str(), "r");
        if (!yyin)  throw parse_error("No such file or directory.");
        root = mknode("TranslationUnitDecl");
        *n_errs = 0;
        parse(n_errs, root);
        fclose(yyin);
        semantic_check(n_errs, root, program["-w"] == true);
        if (*n_errs > 0)  throw parse_error(to_string(*n_errs) + " error(s) generated.");
        if (program["--ast-dump"] == true) print_ast(root);
        if (program["--sym-dump"] == true) print_sym_tab();
        remove(pp_filename.c_str());
    }
    catch (const parse_error& err) {
        cerr << err.what() << endl;
        exit(1);
    } catch (const exception& err) {
        cerr << err.what() << endl;
        cerr << program;
        exit(1);
    }
    return { *n_errs, filename, program.get("-o"), root };
}