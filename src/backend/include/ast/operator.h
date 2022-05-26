#pragma once

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


#include "ast/base.h"
#include "ast/varref.h"
#include "llvm_utils.h"
#include "type_utils.h"

using namespace llvm;

class ArraySubExprAST : public ExprAST {
    int type;
public:
    std::unique_ptr<VarRefExprAST> var;
    std::unique_ptr<ExprAST> sub;

    int getType() { return type; }

    ArraySubExprAST(std::unique_ptr<VarRefExprAST> var, std::unique_ptr<ExprAST> sub, int type_id) :
            var(std::move(var)), sub(std::move(sub)), type(type_id) {}

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

    UnaryExprAST(const std::string &op, std::unique_ptr<ExprAST> rhs) : op(op), rhs(std::move(rhs)) {};

    Value *codegen(bool wantPtr = false) override;
};