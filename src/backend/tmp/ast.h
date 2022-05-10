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

#include<bits/stdc++.h>


using namespace llvm;

class ExprAST {
public:
    virtual ~ExprAST() = default;

    virtual Value *codegen() = 0;
};

/// DoubleExprAST - Expression class for numeric literals like "1.0".
class DoubleExprAST : public ExprAST {
    double val;
public:
    DoubleExprAST(double val) : val(val) {}

    Value *codegen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
    std::string name;
public:
    VariableExprAST(const std::string &name) : name(name) {}

    Value *codegen() override;

    const std::string &getName() const { return name; }
};


/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
    char opcode;
    std::unique_ptr<ExprAST> operand;
public:
    UnaryExprAST(char opcode, std::unique_ptr<ExprAST> operand)
            : opcode(opcode), operand(std::move(operand)) {}

    Value *codegen() override;
};


/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    char op;
    std::unique_ptr<ExprAST> lhs, rhs;
public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs,
                  std::unique_ptr<ExprAST> rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

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

class ForExprAST : public ExprAST {
    std::unique_ptr<ExprAST> start, end, step, body;
    std::string var_name;

public :
    ForExprAST(
            const std::string &var_name,
            std::unique_ptr<ExprAST> start,
            std::unique_ptr<ExprAST> end,
            std::unique_ptr<ExprAST> step,
            std::unique_ptr<ExprAST> body): var_name(var_name), start(std::move(start)),
            end(std::move(end)), step(std::move(step)), body(std::move(body)) {}

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

class PrototypeAST {
    // function declaration
    std::string name;
    std::vector<std::string> args;

public :
    PrototypeAST(std::string &name, std::vector<std::string> args)
    : name(name), args(std::move(args)) {}
    std::string &getName() { return name; }

    Function *codegen();

};

class FunctionAST {
    std::unique_ptr<PrototypeAST> proto;
    std::unique_ptr<ExprAST> body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto, std::unique_ptr<ExprAST> body)
    : proto(std::move(proto)), body(std::move(body)) {}

    Function *codegen();
};

class WhileExprAST : public ExprAST {
    std::unique_ptr<ExprAST> cond, body;

public:
    WhileExprAST(std::unique_ptr<ExprAST>cond, std::unique_ptr<ExprAST>body)
    : cond(std::move(cond)), body(std::move(body)) {}
    Value *codegen() override;
};