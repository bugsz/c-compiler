#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <cassert>
#include <unistd.h>
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
        .default_value(string(""))
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
    program.add_argument("-stdin", "--stdin")
        .help("use input from stdin")
        .default_value(false)
        .implicit_value(true);
    int* n_errs = new int;
    string input_file, output_file;
    ast_node_ptr root;
    try {
        program.parse_args(argc, argv);
        parse_defines(argc, argv);
        string pp_filename(""), pp_res;
        output_file = program.get("-o");
        if (program["-stdin"] == true) {
            input_file = "stdin";
            pp_filename = "stdin";
            pp_res = program["--fno-preprocess"] == false ? preprocess() : "";
            if (!pp_res.empty()) {
                int fd[2];
                pipe(fd);
                write(fd[1], pp_res.data(), pp_res.size());
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
            }
            yyin = stdin;
        } else {
            input_file = program.get("-f");
            if(input_file.find(".c") == string::npos)
                pp_filename = input_file + ".pp.c";
            else {
                pp_filename = input_file;
                pp_filename.replace(pp_filename.find_last_of(".c"), 4, "pp.c");
            }
            pp_res = program["--fno-preprocess"] == false ? preprocess(input_file) : "";
            yyin = fopen(pp_filename.c_str(), "r");
            if (!yyin)  throw parse_error("No such file or directory: " + pp_filename);
        }
        strcpy(global_filename, pp_filename.c_str());
        if (program["-E"] == true && program["--fno-preprocess"] == false) {
            if(program["--ast-dump"] == true || program["--sym-dump"] == true) {
                cerr << COLOR_BOLD << input_file << ": "
                    << COLOR_PURPLE "warning: "
                    << COLOR_NORMAL COLOR_BOLD "ast-dump and sym-dump flags are useless in preprocess-only mode"
                    << COLOR_NORMAL << endl;
            }
            cout << pp_res << endl;
            exit(0);
        }
        root = mknode("TranslationUnitDecl");
        *n_errs = 0;
        parse(n_errs, root);
        fclose(yyin);
        semantic_check(n_errs, root, program["-w"] == true);
        if (*n_errs > 0)  throw parse_error(to_string(*n_errs) + " error(s) generated.");
        if (program["--ast-dump"] == true) print_ast(root);
        if (program["--sym-dump"] == true) print_sym_tab();
        program["-stdin"] == false ? remove(pp_filename.c_str()) : 0;
    }
    catch (const parse_error& err) {
        cerr << err.what() << endl;
        exit(1);
    } catch (const exception& err) {
        cerr << err.what() << endl;
        cerr << program;
        exit(1);
    }
    return { *n_errs, input_file, output_file, root };
}