#include <stdio.h>
#include <iostream>
#include "argparse.hpp"

extern "C" {
    extern void yyparse();
    extern FILE* yyin;
}

using namespace std;
using namespace argparse;

int main(int argc, char** argv) {
    ArgumentParser program("C Compiler");
    program.add_argument("-o")
        .default_value(string("a.out"))
        .help("output file");
    program.add_argument("-f")
        .default_value(string(""))
        .help("input file");
    try {
        program.parse_args(argc, argv);
        program.is_used("-f") ?
            (program.get("-f") == "" ? throw runtime_error("Need value for -f")
                : yyin = fopen(program.get("-f").c_str(), "r"))
            : yyin = stdin;
        if (!yyin)  throw runtime_error("File not found");
    } catch (const exception& err) {
        cerr << err.what() << endl;
        exit(1);
    }
    yyparse();
    return 0;
}