# C Compiler Frontend


## Compile
* Preresquisites
    * GNU Autotools & Flex/Bison installed
    * A C/C++ compiler with >=`c99` & >=`c++17` support

* Build the project
```bash
./configure
make
```

* Recompile the project from modified source
```bash
sh run-script.sh
./configure
make
```

* Clean up the project folder
```bash
make clean # remove object files

## or

sh clean.sh
```