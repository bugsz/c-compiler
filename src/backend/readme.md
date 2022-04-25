# Backend



## Examples

+ TO be done



## Features

+ Number data type: integer and double
+ Basic control flow: for, if, while, do-while
+ Function definition and execution
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

+ For more information, please refer to `frontend/readme.md`

+ Since we don’t support print function currently, you could check the output value using the following method:

  ```shell
  clang++ test-link.cpp output.o -o test-link
  ```

  ```cpp
  // test-link.cpp
  #include<bits/stdc++.h>
  
  extern "C" {
      int loop(int a);
      int new_loop();
      int test_while();
      int test_do_while();
  }
  
  using namespace std;
  int main() {
      cout << "Start testing" << endl;
      int a = test_do_while();
      cout << a << endl;
  }
  ```

  



