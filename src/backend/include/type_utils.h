#pragma once

#include "frontend.h"
#include <cassert>
#include <iostream>
#include <stack>
#include <memory>

enum BinaryOpType {
    ADD = 0,
    SUB,
    MUL,
    DIV,
    EQ,
    LT,
    GT,
    LE,
    GE,
    NE,
    ASSIGN,
    ASSIGNPLUS,
    LAND,
    LOR,
    REM,
    INC,
    DEC,
    SHL,
    SHR,
    AND,
    OR,
    XOR
};

enum UnaryOpType {
    POS = 0,
    NEG,
    DEREF,
    REF,
    CAST,
    NOT,
    LNOT
};

enum ASTNodeType {
    TRANSLATIONUNIT = 0,
    FUNCTIONDECL,
    BINARYOPERATOR,
    UNARYOPERATOR,
    IFSTMT,
    LITERAL,
    VARDECL,
    ARRAYDECL,
    COMPOUNDSTMT,
    NULLSTMT,
    DECLREFEXPR,
    RETURNSTMT,
    FORSTMT,
    FORDELIMITER,
    CALLEXPR,
    WHILESTMT,
    DOSTMT,
    CASTEXPR,
    ARRAYSUBSCRIPTEXPR,
    BREAKSTMT,
    CONTINUESTMT,
    UNKNOWN
};

int getBinaryOpType(std::string binaryOp);

int getUnaryOpType(std::string unaryOp);

bool isEqual(char *a, std::string b);

ASTNodeType getNodeType(std::string token);

std::string filterString(std::string str);

void print(std::string a);

void print_node(ast_node_ptr node);

template<typename TO, typename FROM>
std::unique_ptr<TO> static_unique_pointer_cast(std::unique_ptr<FROM> &&old) {
    return std::unique_ptr<TO>{static_cast<TO *>(old.release())};
}

