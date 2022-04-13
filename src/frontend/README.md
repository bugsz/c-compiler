# C Compiler Frontend

## Usage
```
$ ./zjucc --help 
Usage: C-Compiler [options] 

Optional arguments:
-h --help       shows help message and exits [default: false]
-v --version    prints version information and exits [default: false]
-o --output     output file [default: "a.out"]
-f --file       input file [default: ""]
--ast-dump      print AST [default: false]
```

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

## Examples
### AST
* The names of frontend AST nodes follow the Clang convention.

C Source
```c
int global_a = 0;
int global_b = (1 < 2) + 3;

int func(int a, int b, int c);

int main() {
    int a = 0;
    int b = 2;
    int c = 0;
    
    a = b;
    b = func(a, b, c);
}
```

AST
```
$ ./zjucc -f test/test_ast.c --ast-dump

TranslationUnitDecl 0x600002aa80c0
`-VarDecl 0x600002aa8240
| `-Literal 0x600002aa8180
`-VarDecl 0x600002aa86c0
| `-BinaryOperator 0x600002aa8600
|   `-BinaryOperator 0x600002aa8480
|     `-Literal 0x600002aa8300
|     `-Literal 0x600002aa83c0
|   `-Literal 0x600002aa8540
`-FunctionDecl 0x600002aa8b40
| `-ParmVarDecl 0x600002aa8780
| `-ParmVarDecl 0x600002aa8840
| `-ParmVarDecl 0x600002aa8900
`-FunctionDecl 0x600002aa9ec0
| `-CompoundStmt 0x600002aa9e00
|   `-DeclStmt 0x600002aa92c0
|     `-VarDecl 0x600002aa8cc0
|       `-Literal 0x600002aa8c00
|   `-DeclStmt 0x600002aa9140
|     `-VarDecl 0x600002aa8e40
|       `-Literal 0x600002aa8d80
|   `-DeclStmt 0x600002aa9080
|     `-VarDecl 0x600002aa8fc0
|       `-Literal 0x600002aa8f00
|   `-BinaryOperator 0x600002aa95c0
|     `-DeclRefExpr 0x600002aa9440
|     `-DeclRefExpr 0x600002aa9500
|   `-BinaryOperator 0x600002aa9c80
|     `-DeclRefExpr 0x600002aa9680
|     `-CallExpr 0x600002aa9bc0
|       `-DeclRefExpr 0x600002aa9740
|       `-DeclRefExpr 0x600002aa9800
|       `-DeclRefExpr 0x600002aa98c0
|       `-DeclRefExpr 0x600002aa9980
```

### Symbol Table

C Source
```c
int a;
int b;

int func(double t) {
    return 2 * t;
}

int main(int a) {
    int b = 3;
    {
        int b = 4;
        int c = 2;
    }
    return 0;
}

int d = 2;
```

Symbol Table
```bash
$ ./zjucc -f test/test_sym.c --sym-dump

----------------------------------------
Symbol Table of test/test_ast.c:
----------------------------------------
Scope: 0x16cf5b220 TranslationUnit Global
d : int
b : int
a : int
    Scope: 0x16cf5b190 func Local
    t : double
    Scope: 0x16cf5b190 main Local
    b : int
    a : int
        Scope: 0x16cf5b100 Annonymous Local
        c : int
        b : int
----------------------------------------
```