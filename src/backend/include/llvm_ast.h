#include<bits/stdc++.h>


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
    IFSTMT,
    LITERAL,
    DECLSTMT,
    VARDECL,
    COMPOUNDSTMT,
    DECLREFEXPR,
    RETURNSTMT,
    FORSTMT,
    FORDELIMITER,
    CALLEXPR,
    WHILESTMT,
    DOSTMT,
    UNKNOWN
};

enum BinaryOpType {
    ADD = 0,
    SUB,
    MUL, 
    DIV,
    EQ,
    LT,
};

ASTNodeType getNodeType(std::string token);
int getBinaryOpType(std::string binaryOp);

class ExprAST {
public:
    // int type = UNKNOWN;
    virtual ~ExprAST() = default;
    virtual Value *codegen() = 0;
};

class TranslationUnitExprAST: public ExprAST {
    std::vector<std::unique_ptr<ExprAST>> globalVarList;
    std::vector<std::unique_ptr<ExprAST>> exprList;

public:
    TranslationUnitExprAST(std::vector<std::unique_ptr<ExprAST>> globalVarList, std::vector<std::unique_ptr<ExprAST>> exprList) 
    : globalVarList(std::move(globalVarList)), exprList(std::move(exprList)) {}
    
    Value *codegen() override;

};

class LiteralExprAST: public ExprAST {
    int type;
    std::string value;

public:
    LiteralExprAST(int type, std::string &value)
    : type(type), value(value) {
        this->type = type;
        if(type == TYPEID_STR) {
            std::cout << "  asdf" << value << std::endl;
            this->value = value.substr(1, value.size() - 2);
        }
    }
    Value *codegen() override;

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
    Value *codegen() override;

    void setType(int type) { this->type = type; }
    int getType() { return this->type; }
};

class VarRefExprAST: public ExprAST{
    
    int type;

public:
    std::string name;
    VarRefExprAST(const std::string &name, int type) : name(name), type(type) {}
    const std::string &getName() const { return name; }
    Value *codegen() override;
};



/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    std::string op;
    
public:
    int type;
    std::unique_ptr<ExprAST> lhs, rhs;
    BinaryExprAST(const std::string &op, std::unique_ptr<ExprAST> lhs,
                  std::unique_ptr<ExprAST> rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    Value *codegen() override;
};

class ForExprAST : public ExprAST {
    std::unique_ptr<ExprAST> start, end, step, body;
    std::string varName;

public :
    ForExprAST(
            // const std::string &var_name,
            std::unique_ptr<ExprAST> start,
            std::unique_ptr<ExprAST> end,
            std::unique_ptr<ExprAST> step,
            std::unique_ptr<ExprAST> body): 
            // var_name(var_name), 
            start(std::move(start)),
            end(std::move(end)), 
            step(std::move(step)), 
            body(std::move(body)) { this->varName = getVarName(); }

    Value *codegen() override;

    std::string getVarName() {
        auto ss = static_cast<BinaryExprAST *>(start.get());
        auto lhs = static_cast<VarRefExprAST *>(ss->lhs.get());
        return lhs->name;
    }
};


class PrototypeAST : public ExprAST {
    std::string name;
    
    std::map<std::string, int> args;

public:
    int retVal;
    PrototypeAST(std::string &name, int retVal, std::map<std::string, int> args)
    : name(name), retVal(retVal), args(std::move(args)) {}
    Function *codegen();

    std::string &getName() { return name; }
    int getRetVal() { return retVal; }
};

class FunctionDeclAST: public ExprAST {
    std::unique_ptr<PrototypeAST> prototype;
    std::unique_ptr<ExprAST> body;
    std::unique_ptr<ExprAST> returnStmt;
    Function *codegen() override;

public:
    FunctionDeclAST(std::unique_ptr<PrototypeAST> prototype, std::unique_ptr<ExprAST> body, std::unique_ptr<ExprAST> returnStmt)
    : prototype(std::move(prototype)), body(std::move(body)) {}
};

class CompoundStmtExprAST: public ExprAST {
    std::vector<std::unique_ptr<ExprAST>> exprList;
public:
    CompoundStmtExprAST(std::vector<std::unique_ptr<ExprAST>> exprList): exprList(std::move(exprList)) {}
    Value *codegen() override;
};

class ReturnStmtExprAST: public ExprAST {
    std::unique_ptr<ExprAST> body;
public:
    ReturnStmtExprAST(std::unique_ptr<ExprAST> body)
    : body(std::move(body)) {}
    Value *codegen() override;
};

class CallExprAST : public ExprAST {
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;

public:
    CallExprAST(const std::string &callee, std::vector<std::unique_ptr<ExprAST>> args)
    : callee(callee), args(std::move(args)) {}

    Value *codegen() override;
};

class IfExprAST : public ExprAST {
    std::unique_ptr<ExprAST> cond, then_case, else_case;

public:
    IfExprAST(std::unique_ptr<ExprAST> cond,
              std::unique_ptr<ExprAST> then_case,
              std::unique_ptr<ExprAST> else_case)
            : cond(std::move(cond)), then_case(std::move(then_case)), else_case(std::move(else_case)) {}

    Value *codegen() override;
};

class WhileExprAST : public ExprAST {
    std::unique_ptr<ExprAST> cond, body;

public:
    WhileExprAST(std::unique_ptr<ExprAST>cond, std::unique_ptr<ExprAST>body)
    : cond(std::move(cond)), body(std::move(body)) {}
    Value *codegen() override;
};

class DoExprAST : public ExprAST {
    std::unique_ptr<ExprAST> cond, body;

public:
    DoExprAST(std::unique_ptr<ExprAST>cond, std::unique_ptr<ExprAST>body)
    : cond(std::move(cond)), body(std::move(body)) {}
    Value *codegen() override;
};



class LLVMAST {
    ast_node_ptr node_ptr;
};

