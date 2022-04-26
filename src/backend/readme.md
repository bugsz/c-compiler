# Backend



## Examples
+ Please refer to `project-root/test/test.c`



## Features

+ Number data type: integer(32-bit) and double
+ String data type, **but only limited to constant**
+ Global variable support
+ Basic control flow: for, if, while, do-while
+ Function definition and execution, and also recursion
+ built-in function like `printf`
+ ...



## Compilation

### Prerequisites

+ GNU CMake >= 3.13.4 (lower is ok but you have to change cmakelist.txt)
+ A C/C++ compiler with >=`c99` & >=`c++17` support
+ llvm-12



### Command

+ Execute the following command under the root folder:

  ```shell
  mkdir build && cd build
  cmake ..
  make
  ```

+ Then you should notice a `./zjucc-backend` as a output program.

+ To check the IR code emitted and object file, execute

  ```shell
  ./zjucc_backend -f ../test/loop.c --ast-dump
  ```

  For more information, please refer to `frontend/readme.md`

+ Two files will be generated: `output.ll` and `output.o`. Either use `clang++ output.ll -o ./main` or `clang++ output.o -o ./main` to generate executable file.

  



