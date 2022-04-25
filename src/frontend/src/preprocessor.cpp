#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include "simplecpp.h"

#include "exception.h"
#include "config.h"

using namespace simplecpp;
using namespace std;

static DUI dui;

void parse_defines(int argc, const char** argv) {
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (*arg == '-') {
            char c = arg[1];
            if (c != 'D' && c != 'U' && c != 'I' && c != 'i')
                continue;
            const char *value = arg[2] ? (argv[i] + 2) : argv[++i];
            switch (c) {
            case 'D': // define symbol
                dui.defines.push_back(value);
                break;
            case 'U': // undefine symbol
                dui.undefined.insert(value);
                break;
            case 'I': // include path
                dui.includePaths.push_back(value);
                break;
            case 'i': // include file
                if (strncmp(arg, "-include=", 9) == 0)
                    dui.includes.push_back(arg + 9);
                break;
            }
        }
    }
}

static const char* err_msg[] = {
    "#error: ", "#warning: ", "missing header: ",
    "include nested too deeply: ", "syntax error: ",
    "portability: ", "unhandled char error: ",
    "explicit include not found: "
};

// read input from stdin
string preprocess() {
    OutputList outputList;
    vector<string> files;
    TokenList rawtokens(cin, files, "stdin", &outputList);
    rawtokens.removeComments();
    map<string, TokenList*> included = load(rawtokens, files, dui, &outputList);
    for (pair<string, TokenList *> i : included)
        i.second->removeComments();
    TokenList outputTokens(files);
    simplecpp::preprocess(outputTokens, rawtokens, files, included, dui, &outputList);
    stringstream err;
    for (const Output& output : outputList) {
        err << COLOR_BOLD << output.location.file() << ':' << output.location.line << ": "
            << COLOR_NORMAL COLOR_RED << err_msg[output.type]
            << COLOR_NORMAL << output.msg;
    }
    if (err.str().size() > 0) {
        cleanup(included);
        throw parse_error(err.str());
    }
    string s = outputTokens.stringify();
    cleanup(included);
    return s;
}

string preprocess(string filename, string outfile) {
    OutputList outputList;
    vector<string> files;
    ifstream f(filename);
    TokenList rawtokens(f, files, filename, &outputList);
    rawtokens.removeComments();
    map<string, TokenList*> included = load(rawtokens, files, dui, &outputList);
    for (pair<string, TokenList *> i : included)
        i.second->removeComments();
    TokenList outputTokens(files);
    simplecpp::preprocess(outputTokens, rawtokens, files, included, dui, &outputList);
    stringstream err;
    for (const Output& output : outputList) {
        err << COLOR_BOLD << output.location.file() << ':' << output.location.line << ": "
            << COLOR_NORMAL COLOR_RED << err_msg[output.type]
            << COLOR_NORMAL << output.msg;
    }
    if (err.str().size() > 0) {
        cleanup(included);
        throw parse_error(err.str());
    }
    fstream out(outfile, ios::out);
    string s = outputTokens.stringify();
    out.write(s.c_str(), s.size());
    f.close();
    out.close();
    cleanup(included);
    return s;
}