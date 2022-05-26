# c-compiler

+ A compiler for our simple C language



## Author

+ Pan Zhiyuan @pan2013e
+ Meng Junyi @MJYINMC
+ Su Zhe @bugsz



## Feature

### Stable

+ Simple variable / constant definition and use, both locally and globally
+ Multiple simple type: int, short, long, double, string, char
+ One-level pointer to a simple variable, and corresponding ref and deref operation
+ One-dimensional array, support using array name as a pointer
+ Function definition, declaration and call
+ Basic control flow (for, if, while, do-while)
+ Supported binary operators: `+, -, *, /, >, <, >=, <=, =, &&, ||, <<, >>, &, |, ^,` and all ops like `+=, -=,` ...
+ Supported unary operators: `+, -, *, &, (), ~, !`
+ Can distinguish `i++` and `++i`
+ Support type conversion, both explicitly (by using unary operator () ) and implicitly (by assignment or operation)
+ Support scope
+ Support built_in function provided by `libc` (printf, puts, malloc, scanf, sqrt...)
+ Support for `va_args`
+ Support macro definition and function
+ Support header file



### Experimental

+ Two-dimensional array (On `more_flexible_syntax` branch)



## Compile

### Prerequisites

#### Frontend

+ GNU Autotools(automake, autoconf)
+ flex & bison
  * Note: The program passed tests with flex version >= 2.6.4 & bison version >= 3.7.5. Lower version of bison may result in compilation failure. 
  * For macOS, you need to install the latest version of bison through Homebrew or other sources. Then, you need to modify the generated Makefile by replacing `bison`（in Makefile, just search for `YACC = bison -y`） with the newer vesion's path, for example, `/opt/homebrew/opt/bison/bin/bison`.


#### Backend

+ GNU CMake >= 3.13.4 (lower is ok but you have to change cmakelist.txt)
+ Clang/Clang++ compiler with >=`c99` & >=`c++17` support
+ llvm-13





### Build

+ Execute the following command under the root of the project, then you should see `./c-compiler` in the folder. If your system `cc` and `c++` is not clang/clang++, may cause issue, so set CC=clang CXX=clang++ to avoid trouble.

  ```shell
  mkdir build && cd build
  CC=clang CXX=clang++ cmake .. -DALL ON
  make
  ```



+ `cmake` command above finishes the task of compiling both frontend and backend separately into static library(with `ALL` option `ON`). And `make` takes these two libraries and put them together into an executable. 
  + Options `ALL`: When it is `ON`, cmake will build the frontend library, otherwise only backend library will be build. Our suggestion is to use `cmake ..` when you already had the static frontend library.

  + Options `LIBC_DIR`: Specify libc path if you have multiple versions of libc on your system, otherwise, we will use the system libc which is discovered by cmake `find_library()`.

## Usage

```
❯ ./c-compiler --help
Usage: C-Compiler [options]

Optional arguments:
-h --help                               shows help message and exits [default: false]
-v --version                            prints version information and exits [default: false]
-o --output                             specify output file name [default: "a.out"]
-f --file                               specify source file name [default: ""]
-c                                      compile to object code only [default: false]
-E                                      preprocess only [default: false]
-w -fno-warnings --fno-warnings         disable warnings [default: false]
-D -I -U -i                             preprocessor flags, space must be used to separate flag and parameter
-stdin --stdin                          use input from stdin [default: false]
-ast-dump --ast-dump                    print AST [default: false]
-sym-dump --sym-dump                    print symbol table [default: false]
-static-analyze --static-analyze        run static program analysis only [default: false]
-fno-preprocess --fno-preprocess        disable preprocessing [default: false]
```



## Credits

[p-ranav/argparse](https://github.com/p-ranav/argparse)
[danmar/simplecpp](https://github.com/danmar/simplecpp)

