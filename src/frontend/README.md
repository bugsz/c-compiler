# C Compiler Frontend

## Examples
Check [examples](test/README.md)

## Author
Zhiyuan Pan, Zhejiang University

[zy_pan@zju.edu.cn](mailto:zy_pan@zju.edu.cn)

## Compile
* Preresquisites
    * GNU Autotools(automake, autoconf)
    * flex & bison
      * Note: The program passed tests with flex version >= 2.6.4 & bison version >= 3.7.5. Lower version of bison may result in compilation failure. 
      * For macOS, you need to install the latest version of bison through Homebrew or other sources. Then, you need to modify the generated Makefile by replacing `bison`（in Makefile, just search for `YACC = bison -y`） with the newer vesion's path, for example, `/opt/homebrew/opt/bison/bin/bison`.
    * A C/C++ compiler with >=`c99` & >=`c++17` support

* Build the project
The `configure` script in the repository is generated on x86_64 Linux. If you use the same environment, just type the following commands.

```bash
./configure
make
```

Otherwise, you need to regenerate the scripts. If you add new source files to the project, you should also run the following commands.

```bash
sh run-script.sh
./configure
make
```

* Clean up the project folder
```bash
make clean # remove lib, executable & object files

## or

sh clean.sh
```

## Use the compiler frontend as a library

Example:

```cpp
#include "frontend.h"

int main(int argc, const char** argv) {
    struct lib_frontend_ret ret = frontend_entry(argc, argv);
}
```

Compile:

```bash
g++ example.cpp -L${PATH_TO_ZJUCC} -lzjucc
```

## Usage
```
$ ./zjucc --help 
Usage: C-Compiler [options] 

Optional arguments:
-h --help                               shows help message and exits [default: false]
-v --version                            prints version information and exits [default: false]
-o --output                             specify output file name [default: "a.out"]
-f --file                               specify source file name [required]
-E                                      preprocess only [default: false]
-ast-dump --ast-dump                    print AST [default: false]
-sym-dump --sym-dump                    print symbol table [default: false]
-w -fno-warnings --fno-warnings         disable warnings [default: false]
-fno-preprocess --fno-preprocess        disable preprocessing [default: false]
```

## TODO List
* More C syntaxes
* More optimizations at compile time
* Fix incorrect col. number in prompts when preprocessor is on

## Credits
[p-ranav/argparse](https://github.com/p-ranav/argparse)
[danmar/simplecpp](https://github.com/danmar/simplecpp)
