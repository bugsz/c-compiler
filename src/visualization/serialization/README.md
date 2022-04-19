# C Compiler Serializer

## Usage

```bash
$ ./serializer --help 
Usage: C-Compiler [options] 

Optional arguments:
-h --help       shows help message and exits [default: false]
-v --version    prints version information and exits [default: false]
-o --output     output file [default: "a.out"]
-f --file       input file [default: ""]
--ast-dump      print AST [default: false]
```

## Compile

```bash
sh compile.sh
```

## Example

C Source Code

```c
int a;
int d = 3;

int positive(int x) {
    int res;
    if (x > 0) {
        res = 1;
    } else {
        res = 0;
    }
    return res;
}

int main() {
    int a;
    int b = 5;
    int c;
    a = 3;
    c = a + b;
    b = positive(c);
}
```

```json
 ./serializer -f ../../frontend/test/function.c --ast-dump
 ------------------------------------------
Abstract Syntax Tree of ../../frontend/test/function.c
------------------------------------------
TranslationUnitDecl 0x7fe9ddf04a00  'void'
`-VarDecl 0x7fe9ddf04b30 <1:5> a 'int'
`-VarDecl 0x7fe9ddf04d90 <2:5> d 'int'
| `-Literal 0x7fe9ddf04c60 <2:9> 3 'int'
`-FunctionDecl 0x7fe9ddf06680 <4:5> positive 'int (*) (int)'
| `-ParmVarDecl 0x7fe9ddf04ec0 <4:18> x 'int'
| `-CompoundStmt 0x7fe9ddf06550
|   `-DeclStmt 0x7fe9ddf05120
|     `-VarDecl 0x7fe9ddf04ff0 <5:9> res 'int'
|   `-IfStmt 0x7fe9ddf06090
|     `-BinaryOperator 0x7fe9ddf054b0 <6:11> > 'int'
|       `-DeclRefExpr 0x7fe9ddf05250 <6:9> x 'int'
|       `-Literal 0x7fe9ddf05380 <6:13> 0 'int'
|     `-CompoundStmt 0x7fe9ddf05970
|       `-BinaryOperator 0x7fe9ddf05840 <7:13> = 'int'
|         `-DeclRefExpr 0x7fe9ddf055e0 <7:9> res 'int'
|         `-Literal 0x7fe9ddf05710 <7:15> 1 'int'
|     `-CompoundStmt 0x7fe9ddf05e30
|       `-BinaryOperator 0x7fe9ddf05d00 <9:13> = 'int'
|         `-DeclRefExpr 0x7fe9ddf05aa0 <9:9> res 'int'
|         `-Literal 0x7fe9ddf05bd0 <9:15> 0 'int'
|   `-ReturnStmt 0x7fe9ddf062f0  'int'
|     `-DeclRefExpr 0x7fe9ddf061c0 <11:12> res 'int'
`-FunctionDecl 0x7fe9ddf08560 <14:5> main 'int (*) ()'
| `-CompoundStmt 0x7fe9ddf08430
|   `-DeclStmt 0x7fe9ddf07000
|     `-VarDecl 0x7fe9ddf067b0 <15:9> a 'int'
|   `-DeclStmt 0x7fe9ddf06da0
|     `-VarDecl 0x7fe9ddf06a10 <16:9> b 'int'
|       `-Literal 0x7fe9ddf068e0 <16:13> 5 'int'
|   `-DeclStmt 0x7fe9ddf06c70
|     `-VarDecl 0x7fe9ddf06b40 <17:9> c 'int'
|   `-BinaryOperator 0x7fe9ddf074c0 <18:7> = 'int'
|     `-DeclRefExpr 0x7fe9ddf07260 <18:5> a 'int'
|     `-Literal 0x7fe9ddf07390 <18:9> 3 'int'
|   `-BinaryOperator 0x7fe9ddf07ab0 <19:7> = 'int'
|     `-DeclRefExpr 0x7fe9ddf075f0 <19:5> c 'int'
|     `-BinaryOperator 0x7fe9ddf07980 <19:11> + 'int'
|       `-DeclRefExpr 0x7fe9ddf07720 <19:9> a 'int'
|       `-DeclRefExpr 0x7fe9ddf07850 <19:13> b 'int'
|   `-BinaryOperator 0x7fe9ddf080a0 <20:7> = 'int'
|     `-DeclRefExpr 0x7fe9ddf07be0 <20:5> b 'int'
|     `-CallExpr 0x7fe9ddf07f70  'int'
|       `-DeclRefExpr 0x7fe9ddf07d10 <20:9> positive 'int'
|       `-DeclRefExpr 0x7fe9ddf07e40 <20:18> c 'int'

{
    "token": "TranslationUnitDecl",
    "position": "",
    "val": "",
    "type": "void",
    "children": [
    {
        "token": "VarDecl",
        "position": " <1:5>",
        "val": "a",
        "type": "int",
        "children": []
    },
    {
        "token": "VarDecl",
        "position": " <2:5>",
        "val": "d",
        "type": "int",
        "children": [
        {
            "token": "Literal",
            "position": " <2:9>",
            "val": "3",
            "type": "int",
            "children": []
        }]
    },
    {
        "token": "FunctionDecl",
        "position": " <4:5>",
        "val": "positive",
        "type": "int",
        "children": [
        {
            "token": "ParmVarDecl",
            "position": " <4:18>",
            "val": "x",
            "type": "int",
            "children": []
        },
        {
            "token": "CompoundStmt",
            "position": "",
            "val": "",
            "type": "",
            "children": [
            {
                "token": "DeclStmt",
                "position": "",
                "val": "",
                "type": "",
                "children": [
                {
                    "token": "VarDecl",
                    "position": " <5:9>",
                    "val": "res",
                    "type": "int",
                    "children": []
                }]
            },
            {
                "token": "IfStmt",
                "position": "",
                "val": "",
                "type": "",
                "children": [
                {
                    "token": "BinaryOperator",
                    "position": " <6:11>",
                    "val": ">",
                    "type": "int",
                    "children": [
                    {
                        "token": "DeclRefExpr",
                        "position": " <6:9>",
                        "val": "x",
                        "type": "int",
                        "children": []
                    },
                    {
                        "token": "Literal",
                        "position": " <6:13>",
                        "val": "0",
                        "type": "int",
                        "children": []
                    }]
                },
                {
                    "token": "CompoundStmt",
                    "position": "",
                    "val": "",
                    "type": "",
                    "children": [
                    {
                        "token": "BinaryOperator",
                        "position": " <7:13>",
                        "val": "=",
                        "type": "int",
                        "children": [
                        {
                            "token": "DeclRefExpr",
                            "position": " <7:9>",
                            "val": "res",
                            "type": "int",
                            "children": []
                        },
                        {
                            "token": "Literal",
                            "position": " <7:15>",
                            "val": "1",
                            "type": "int",
                            "children": []
                        }]
                    }]
                },
                {
                    "token": "CompoundStmt",
                    "position": "",
                    "val": "",
                    "type": "",
                    "children": [
                    {
                        "token": "BinaryOperator",
                        "position": " <9:13>",
                        "val": "=",
                        "type": "int",
                        "children": [
                        {
                            "token": "DeclRefExpr",
                            "position": " <9:9>",
                            "val": "res",
                            "type": "int",
                            "children": []
                        },
                        {
                            "token": "Literal",
                            "position": " <9:15>",
                            "val": "0",
                            "type": "int",
                            "children": []
                        }]
                    }]
                }]
            },
            {
                "token": "ReturnStmt",
                "position": "",
                "val": "",
                "type": "int",
                "children": [
                {
                    "token": "DeclRefExpr",
                    "position": " <11:12>",
                    "val": "res",
                    "type": "int",
                    "children": []
                }]
            }]
        }]
    },
    {
        "token": "FunctionDecl",
        "position": " <14:5>",
        "val": "main",
        "type": "int",
        "children": [
        {
            "token": "CompoundStmt",
            "position": "",
            "val": "",
            "type": "",
            "children": [
            {
                "token": "DeclStmt",
                "position": "",
                "val": "",
                "type": "",
                "children": [
                {
                    "token": "VarDecl",
                    "position": " <15:9>",
                    "val": "a",
                    "type": "int",
                    "children": []
                }]
            },
            {
                "token": "DeclStmt",
                "position": "",
                "val": "",
                "type": "",
                "children": [
                {
                    "token": "VarDecl",
                    "position": " <16:9>",
                    "val": "b",
                    "type": "int",
                    "children": [
                    {
                        "token": "Literal",
                        "position": " <16:13>",
                        "val": "5",
                        "type": "int",
                        "children": []
                    }]
                }]
            },
            {
                "token": "DeclStmt",
                "position": "",
                "val": "",
                "type": "",
                "children": [
                {
                    "token": "VarDecl",
                    "position": " <17:9>",
                    "val": "c",
                    "type": "int",
                    "children": []
                }]
            },
            {
                "token": "BinaryOperator",
                "position": " <18:7>",
                "val": "=",
                "type": "int",
                "children": [
                {
                    "token": "DeclRefExpr",
                    "position": " <18:5>",
                    "val": "a",
                    "type": "int",
                    "children": []
                },
                {
                    "token": "Literal",
                    "position": " <18:9>",
                    "val": "3",
                    "type": "int",
                    "children": []
                }]
            },
            {
                "token": "BinaryOperator",
                "position": " <19:7>",
                "val": "=",
                "type": "int",
                "children": [
                {
                    "token": "DeclRefExpr",
                    "position": " <19:5>",
                    "val": "c",
                    "type": "int",
                    "children": []
                },
                {
                    "token": "BinaryOperator",
                    "position": " <19:11>",
                    "val": "+",
                    "type": "int",
                    "children": [
                    {
                        "token": "DeclRefExpr",
                        "position": " <19:9>",
                        "val": "a",
                        "type": "int",
                        "children": []
                    },
                    {
                        "token": "DeclRefExpr",
                        "position": " <19:13>",
                        "val": "b",
                        "type": "int",
                        "children": []
                    }]
                }]
            },
            {
                "token": "BinaryOperator",
                "position": " <20:7>",
                "val": "=",
                "type": "int",
                "children": [
                {
                    "token": "DeclRefExpr",
                    "position": " <20:5>",
                    "val": "b",
                    "type": "int",
                    "children": []
                },
                {
                    "token": "CallExpr",
                    "position": "",
                    "val": "",
                    "type": "int",
                    "children": [
                    {
                        "token": "DeclRefExpr",
                        "position": " <20:9>",
                        "val": "positive",
                        "type": "int",
                        "children": []
                    },
                    {
                        "token": "DeclRefExpr",
                        "position": " <20:18>",
                        "val": "c",
                        "type": "int",
                        "children": []
                    }]
                }]
            }]
        }]
    }]
}
```