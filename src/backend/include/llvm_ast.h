#include<bits/stdc++.h>
#include <memory>


#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"


#include "frontend.h"
#include "type_utils.h"

using namespace llvm;

#define INTEGER_BITWIDTH 32

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

ASTNodeType getNodeType(std::string token);
int getBinaryOpType(std::string binaryOp);

std::string filterString(std::string str);

class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual Value *codegen(bool wantPtr = false) = 0;
};

class NullStmtAST: public ExprAST{
    Value *codegen(bool wantPtr = false) override;
};

class TranslationUnitExprAST: public ExprAST {
    std::vector<std::unique_ptr<ExprAST>> globalVarList;
    std::vector<std::unique_ptr<ExprAST>> exprList;

public:
    TranslationUnitExprAST(std::vector<std::unique_ptr<ExprAST>> globalVarList, std::vector<std::unique_ptr<ExprAST>> exprList) 
    : globalVarList(std::move(globalVarList)), exprList(std::move(exprList)) {}
    
    Value *codegen(bool wantPtr = false) override;
};

class LiteralExprAST: public ExprAST {
    int type;
    std::string value;

public:
    LiteralExprAST(int type, std::string &value)
    : type(type), value(value) {
        this->type = type;
        if(type == TYPEID_STR) {
            this->value = filterString(value);
        }
    }
    Value *codegen(bool wantPtr = false) override;

    void setType(int type) { this->type = type; }
    int getType() { return this->type; }
    std::string &getValue() { return this->value; }
};

class VarExprAST: public ExprAST {
    int type;
public:
    std::string name;
    std::unique_ptr<ExprAST> init;
    VarExprAST(const std::string &name, int type, std::unique_ptr<ExprAST> init)
    : name(name), type(type), init(std::move(init)) {}
    Value *codegen(bool wantPtr = false) override;

    void setType(int type) { this->type = type; }
    int getType() { return this->type; }
    std::string &getName() { return this->name; }
};

class GlobalVarExprAST: public ExprAST {
    int type;
    std::string name;
    std::unique_ptr<VarExprAST> init;
public:
    GlobalVarExprAST(std::unique_ptr<VarExprAST> init)
    : init(std::move(init)) { 
        std::cout << "global var" << std::endl;
        type = this->init->getType(); 
        name = this->init->getName();
        std::cout << type << " " << name << std::endl; 
    }
    Value *codegen(bool wantPtr = false) override;
};

class VarRefExprAST: public ExprAST{
    int type;
    std::string name;
public:
    int getType() { return this->type; }
    void setType(int type) { this->type = type; }
    VarRefExprAST(const std::string &name, int type) : name(name), type(type) {}
    const std::string &getName() const { return name; }
    void setName(const std::string &name) { this->name = name; }
    Value *codegen(bool wantPtr = false) override;
};

class ArrayExprAST: public ExprAST {
    int type;
    std::string name;
    int size;
    
    
public:
    std::vector<std::unique_ptr<ExprAST>> init;
    const std::string &getName() const { return name; }
    const int getSize() const { return size; }
    const int getType() const { return type; }
    ArrayExprAST(int type, const std::string &name, int size, std::vector<std::unique_ptr<ExprAST>> init) : type(type), name(name), size(size), init(std::move(init)) {}
    Value *codegen(bool wantPtr = false) override;
};

class ArraySubExprAST: public ExprAST {
    int type;
public:
    std::unique_ptr<VarRefExprAST> var;
    std::unique_ptr<ExprAST> sub;
    int getType() { return type; }
    ArraySubExprAST(std::unique_ptr<VarRefExprAST> var, std::unique_ptr<ExprAST> sub, int type_id):
    var(std::move(var)), sub(std::move(sub)), type(type_id) {}
    Value *codegen(bool wantPtr = false) override;
};

class GlobalArrayExprAST: public ExprAST {
    int type;
    std::string name;
    std::unique_ptr<ArrayExprAST> init;
public:
    GlobalArrayExprAST(std::unique_ptr<ArrayExprAST> init)
    : init(std::move(init)) { 
        std::cout << "global var" << std::endl;
        type = this->init->getType(); 
        name = this->init->getName();
        std::cout << type << " " << name << std::endl; 
    }
    Value *codegen(bool wantPtr = false) override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    int op_type;
    
public:
    std::unique_ptr<ExprAST> lhs, rhs;
    BinaryExprAST(int op_type, std::unique_ptr<ExprAST> lhs,
                  std::unique_ptr<ExprAST> rhs)
            : op_type(op_type), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    Value *codegen(bool wantPtr = false) override;
};

class UnaryExprAST : public ExprAST {
    int type;
    std::string op;
public:
    std::unique_ptr<ExprAST> rhs;
    void setType(int type_id) { this->type = type_id; }
    int getType() { return type; }
    UnaryExprAST(const std::string &op, std::unique_ptr<ExprAST> rhs):op(op), rhs(std::move(rhs)){};
    Value *codegen(bool wantPtr = false) override;
};

class ForExprAST : public ExprAST {
    std::unique_ptr<ExprAST> start, end, step, body;

public :
    ForExprAST(
            std::unique_ptr<ExprAST> start,
            std::unique_ptr<ExprAST> end,
            std::unique_ptr<ExprAST> step,
            std::unique_ptr<ExprAST> body): 
            start(std::move(start)),
            end(std::move(end)), 
            step(std::move(step)), 
            body(std::move(body)){}

    Value *codegen(bool wantPtr = false) override;

    std::string getVarName() {
        auto ss = static_cast<BinaryExprAST *>(start.get());
        auto lhs = static_cast<VarRefExprAST *>(ss->lhs.get());
        return lhs->getName();
    }
};

class BreakExprAST : public ExprAST {
public:
    BreakExprAST() {}
    Value *codegen(bool wantPtr = false) override;
};

class ContinueExprAST : public ExprAST {
public:
    ContinueExprAST() {}
    Value *codegen(bool wantPtr = false) override;
};



class PrototypeAST : public ExprAST {
    std::string name;
    std::vector<std::pair<std::string, int>> args;
    bool va;
public:
    int retVal;
    PrototypeAST(std::string &name, int retVal, std::vector<std::pair<std::string, int>> args, bool va)
    : name(name), retVal(retVal), args(std::move(args)), va(va) {}
    Function *codegen(bool wantPtr = false);
    std::string &getName() { return name; }
    int getRetVal() { return retVal; }
};

class FunctionDeclAST: public ExprAST {
    std::unique_ptr<PrototypeAST> prototype;
    std::unique_ptr<ExprAST> body;
    Function *codegen(bool wantPtr = false) override;

public:
    FunctionDeclAST(std::unique_ptr<PrototypeAST> prototype, std::unique_ptr<ExprAST> body)
    : prototype(std::move(prototype)), body(std::move(body)) {}
};

class CompoundStmtExprAST: public ExprAST {
    
public:
    std::vector<std::unique_ptr<ExprAST>> exprList;
    CompoundStmtExprAST(std::vector<std::unique_ptr<ExprAST>> exprList): exprList(std::move(exprList)) {}
    Value *codegen(bool wantPtr = false) override;
};

class ReturnStmtExprAST: public ExprAST {
    std::unique_ptr<ExprAST> body;
public:
    ReturnStmtExprAST(std::unique_ptr<ExprAST> body)
    : body(std::move(body)) {}
    Value *codegen(bool wantPtr = false) override;
};

class CallExprAST : public ExprAST {
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;

public:
    CallExprAST(const std::string &callee, std::vector<std::unique_ptr<ExprAST>> args)
    : callee(callee), args(std::move(args)) {}

    Value *codegen(bool wantPtr = false) override;
};

class IfExprAST : public ExprAST {
    std::unique_ptr<ExprAST> cond, then_case, else_case;

public:
    IfExprAST(std::unique_ptr<ExprAST> cond,
              std::unique_ptr<ExprAST> then_case,
              std::unique_ptr<ExprAST> else_case)
            : cond(std::move(cond)), then_case(std::move(then_case)), else_case(std::move(else_case)) {}

    Value *codegen(bool wantPtr = false) override;
};

class WhileExprAST : public ExprAST {
    std::unique_ptr<ExprAST> cond, body;

public:
    WhileExprAST(std::unique_ptr<ExprAST>cond, std::unique_ptr<ExprAST>body)
    : cond(std::move(cond)), body(std::move(body)) {}
    Value *codegen(bool wantPtr = false) override;
};

class DoExprAST : public ExprAST {
    std::unique_ptr<ExprAST> cond, body;

public:
    DoExprAST(std::unique_ptr<ExprAST>cond, std::unique_ptr<ExprAST>body)
    : cond(std::move(cond)), body(std::move(body)) {}
    Value *codegen(bool wantPtr = false) override;
};

class LLVMAST {
    ast_node_ptr node_ptr;
};

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
    DEC
    // todo: 位运算
};

enum UnaryOpType {
    POS = 0,
    NEG,
    DEREF,
    REF,
    CAST,
    LNOT
};
