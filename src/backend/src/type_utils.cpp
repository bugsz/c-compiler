#include "type_utils.h"
#include<bits/stdc++.h>

int getBinaryOpType(std::string binaryOp) {
    if(binaryOp == "+" ) return ADD;
    if(binaryOp == "-" ) return SUB;
    if(binaryOp == "*" ) return MUL;
    if(binaryOp == "/") return DIV;
    if(binaryOp == "<") return LT;
    if(binaryOp == ">") return GT;
    if(binaryOp == "%") return REM;
    if(binaryOp == "=" ) return ASSIGN;
    if(binaryOp == "<=") return LE;
    if(binaryOp == ">=") return GE;    
    if(binaryOp == "==") return EQ;
    if(binaryOp == "!=") return NE;
    if(binaryOp == "/="|| binaryOp == "*="|| binaryOp == "-="|| binaryOp == "+=" || \
       binaryOp == "&="|| binaryOp == "|="|| binaryOp == "^="|| binaryOp == "<<="|| binaryOp == ">>=") \
       return ASSIGNPLUS;
    if(binaryOp == "++") return INC;
    if(binaryOp == "--") return DEC;
    if(binaryOp == "&&") return LAND;
    if(binaryOp == "||") return LOR;
    if(binaryOp == "<<") return SHL;
    if(binaryOp == ">>") return SHR;
    if(binaryOp == "&") return AND;
    if(binaryOp == "|") return OR;
    if(binaryOp == "^") return XOR;
    fprintf(stderr, "Unsupported binary operation: %s\n", binaryOp.c_str());
    exit(-1);
}

int getUnaryOpType(std::string unaryOp) {
    if(unaryOp == "+") return POS;
    if(unaryOp == "-") return NEG;
    if(unaryOp == "&") return REF;
    if(unaryOp == "*") return DEREF;
    if(unaryOp == "()") return CAST;
    if(unaryOp == "~") return NOT;
    if(unaryOp == "!") return LNOT;
    return POS;
}

bool isEqual(char *a, std::string b) {
    std::string bb(b), aa(a);
    return aa == bb;
}

ASTNodeType getNodeType(std::string token) {
    if(token == "TranslationUnitDecl") return TRANSLATIONUNIT;
    if(token == "FunctionDecl") return FUNCTIONDECL;
    if(token == "VarDecl") return VARDECL;
    if(token == "BinaryOperator") return BINARYOPERATOR;
    if(token == "Literal") return LITERAL;
    if(token == "CompoundStmt") return COMPOUNDSTMT;
    if(token == "NullStmt") return NULLSTMT;
    if(token == "DeclRefExpr") return DECLREFEXPR;
    if(token == "ReturnStmt") return RETURNSTMT;
    if(token == "ForStmt") return FORSTMT;
    if(token == "ForDelimiter") return FORDELIMITER;
    if(token == "CallExpr") return CALLEXPR;
    if(token == "WhileStmt") return WHILESTMT;
    if(token == "DoStmt") return DOSTMT;
    if(token == "IfStmt") return IFSTMT;
    if(token == "ExplicitCastExpr") return CASTEXPR;
    if(token == "UnaryOperator") return UNARYOPERATOR;
    if(token == "ArrayDecl") return ARRAYDECL;
    if(token == "ArraySubscriptExpr") return ARRAYSUBSCRIPTEXPR;
    if(token == "BreakStmt") return BREAKSTMT;
    if(token == "ContinueStmt") return CONTINUESTMT;
    return UNKNOWN;
}

int ptr2raw(int ptr) {
    switch(ptr) {
        case TYPEID_VOID_PTR:
            return TYPEID_VOID;
        case TYPEID_INT_PTR:
            return TYPEID_INT;
        case TYPEID_FLOAT_PTR:
            return TYPEID_FLOAT;
        case TYPEID_DOUBLE_PTR:
            return TYPEID_DOUBLE;
        case TYPEID_CHAR_PTR:
            return TYPEID_CHAR;
        case TYPEID_LONG_PTR:  
            return TYPEID_LONG;
        case TYPEID_SHORT_PTR:
            return TYPEID_SHORT;
        default:
            return ptr;
    }
}

std::string filterString(std::string str){
    std::string buffer(str.size()+1, 0);
    std::string cmd = "python3 -c 'print("+str+", end=\"\")'";
    FILE * f = popen(cmd.c_str(), "r"); assert(f);
    buffer.resize(fread(&buffer[0], 1, buffer.size()-1, f));
    pclose(f);
    return buffer;
}

void print(std::string a) { std::cout << a << std::endl; }

void print_node(ast_node_ptr node) {
    printf("Node type: %s(%d), val: %s, n_child: %d type_id: %d\n", node->token, getNodeType(node->token), node->val, node->n_child, node->type_id);
}