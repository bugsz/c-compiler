/*  src/wrapper.cpp
 *  Zhiyuan Pan, Zhejiang University
 *  zy_pan@zju.edu.cn
 *  Mon Apr 25, 2022
 */
#include <bits/stdc++.h>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

#include <iostream>

#include "frontend/frontend.h"
#include "backend/backend.h"

using namespace std;

#define GNU_LD  "/usr/bin/gcc"
#define DEFAULT_LINKER  GNU_LD

#ifndef LIBC_DIR
#define LIBC_DIR DEFAULT_LIBC_DIR
#endif

// Wrapper class for system linker
class Linker {
public:
    Linker(string __linker_path = DEFAULT_LINKER) :
        linker_path(__linker_path) {
        ld_args.push_back(linker_path);
    }

    ~Linker() = default;

    void add_object(string filename) {
        ld_args.push_back(filename);
    }

    void set_target(string filename = "a.out") {
        ld_args.push_back("-o");
        ld_args.push_back(filename);
    }

    void set_link_library(string link_dir = LIBC_DIR, string lib_name = "c") {
        ld_args.push_back("-L" + link_dir);
        ld_args.push_back("-l" + lib_name); 
    }
    
    void exec() {
        set_link_library(); // link libc by default
        char** __argv = new char* [ld_args.size() + 1];
        for (int i = 0;i < ld_args.size() + 1;i++) {
            __argv[i] = (i == ld_args.size()) ? NULL : strdup(ld_args[i].c_str());
        }
        if (fork() == 0) {   
            int ret = ::execvp(linker_path.c_str(), __argv);
            if(ret < 0) printf("failed\n");
        } else {
            wait(NULL);
        }
    }

private:
    string linker_path;
    vector<string> ld_args;
};

// This is a function that wraps up frontend and backend.
// After backend generates object code, the function then calls
// system linker (e.g., GNU ld) to link runtime libraries to
// obtain final executable files
int main(int argc, const char** argv) {
    // frontend API call is here
    lib_frontend_ret ret = frontend_entry(argc, argv);
    string obj_filename = ret.output_file + ".o";
    // backend API call is here
    backend_entry(ret, obj_filename);

    // wrapper is here
    Linker ld = Linker();
    ld.add_object(obj_filename); // *TODO*: use filename from backend output in the future ...
    ld.set_target(ret.output_file); // *TODO*: use filename from frontend output in the future ...
    if(!ret.obj_only){
        ld.exec();
        unlink(obj_filename.c_str());
    }
    return 0;
}