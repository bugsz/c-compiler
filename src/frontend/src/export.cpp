#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include "argparse.hpp"
#include "preprocessor.h"

#include "exception.h"
#include "ast_impl.h"
#include "semantic.h"
#include "config.h"

using namespace std;
using namespace argparse;

extern "C" {
    extern int yyparse(int*, ast_node_ptr, char* tmp_file);
    extern FILE* yyin;
}

extern ArgumentParser& parse_args(int argc, const char** argv);

char global_filename[256];

lib_frontend_ret frontend_entry(int argc, const char** argv) {
    int* n_errs = new int;
    *n_errs = 0;
    string input_file, output_file;
    ast_node_ptr root = mknode("TranslationUnitDecl");
    char tmp_file[] = "tmp_XXXXXX";
    try {
        auto args = parse_args(argc, argv);
        if (!args.is_used("-f") && !args.is_used("-stdin")) {
            throw runtime_error("-f or -stdin required.");
        }
        string pp_filename, pp_res;
        output_file = args.get("-o");
        int fd = mkstemp(tmp_file);
        if (args["-stdin"] == true) {
            input_file = "stdin";
            pp_filename = args["--fno-preprocess"] == false ? tmp_file : input_file;
            pp_res = args["--fno-preprocess"] == false ? preprocess() : "";
            if (!pp_res.empty()) {
                ofstream fout(pp_filename);
                fout.write(pp_res.c_str(), pp_res.size());
                yyin = fopen(pp_filename.c_str(), "r");
            } else {
                yyin = stdin;
            }
        } else {
            input_file = args.get("-f");
            pp_filename = args["--fno-preprocess"] == false ? tmp_file : input_file;
            pp_res = args["--fno-preprocess"] == false ? preprocess(input_file, pp_filename) : "";
            yyin = fopen(pp_filename.c_str(), "r");
            if (!yyin)  throw parse_error("No such file or directory: " + pp_filename);
        }
        strcpy(global_filename, input_file.c_str());
        if (args["-E"] == true && args["--fno-preprocess"] == false) {
            cout << pp_res << endl;
            unlink(tmp_file);
            exit(0);
        }
        parse(n_errs, root, tmp_file);
        fclose(yyin);
        if (*n_errs > 0) {
            throw parse_error(to_string(*n_errs) + " error(s) generated.");
        }
        semantic_check(n_errs, root, args["-w"] == true);
        if (*n_errs > 0) {
            throw parse_error(to_string(*n_errs) + " error(s) generated.");
        }
        if (args["--ast-dump"] == true) print_ast(root);
        if (args["--sym-dump"] == true) print_sym_tab();
        if (args["--static-analyze"] == true) {
            cout << "Static analysis passed." << endl;
            exit(0);
        }
    }
    catch (const exception& err) {
        cerr << err.what() << endl;
        unlink(tmp_file);
        exit(1);
    }
    return { *n_errs, input_file, output_file, root };
}