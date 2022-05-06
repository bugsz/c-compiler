/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-27 15:38:00
 * @LastEditors: Pan Zhiyuan
 * @Description: 
 */
#include <string>

#include "argparse.hpp"
#include "preprocessor.h"

#include "config.h"

using namespace std;
using namespace argparse;

static ArgumentParser program(PACKAGE_NAME, VERSION);

static void init_args() {
    program.add_argument("-o", "--output")
        .default_value(string("a.out"))
        .help("specify output file name");
    program.add_argument("-f", "--file")
        .default_value(string(""))
        .help("specify source file name");
    program.add_argument("-c")
        .default_value(false)
        .implicit_value(true)
        .help("compile to object code only");
    program.add_argument("-E")
        .default_value(false)
        .implicit_value(true)
        .help("preprocess only");
    program.add_argument("-w", "-fno-warnings", "--fno-warnings")
        .help("disable warnings")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-I", "-i", "-U", "-D")
        .help("preprocessor flags, space must be used to separate flag and parameter");
    program.add_argument("-stdin", "--stdin")
        .help("use input from stdin")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-ast-dump", "--ast-dump")
        .help("print AST")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-sym-dump", "--sym-dump")
        .help("print symbol table")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-static-analyze", "--static-analyze")
        .default_value(false)
        .implicit_value(true)
        .help("run static program analysis only");
    program.add_argument("-fno-preprocess", "--fno-preprocess")
        .help("disable preprocessing")
        .default_value(false)
        .implicit_value(true);
}

ArgumentParser& parse_args(int argc, const char** argv) {
    init_args();
    parse_defines(argc, argv);
    program.parse_args(argc, argv);
    return program;
}