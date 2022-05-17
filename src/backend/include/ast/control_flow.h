#pragma once

#include <bits/stdc++.h>

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
#include "ast/operator.h"
#include "llvm_utils.h"
#include "type_utils.h"

extern std::unique_ptr<LLVMContext> llvmContext;
extern std::unique_ptr<Module> llvmModule;
extern bool initializing;
extern Value *retPtr;
extern BasicBlock *retBlock;
extern std::stack<std::map<std::string, AllocaInst *>> NamedValues;
extern std::unique_ptr<IRBuilder<>> llvmBuilder;
extern std::unique_ptr<legacy::FunctionPassManager> llvmFPM;

extern std::stack<BasicBlock *> BlockForBreak;
extern std::stack<BasicBlock *> BlockForContinue;

using namespace llvm;

extern std::unique_ptr<LLVMContext> llvmContext;
extern std::unique_ptr<Module> llvmModule;
extern bool initializing;
extern Value *retPtr;
extern BasicBlock *retBlock;
extern std::stack<std::map<std::string, AllocaInst *>> NamedValues;
extern std::unique_ptr<IRBuilder<>> llvmBuilder;
extern std::unique_ptr<legacy::FunctionPassManager> llvmFPM;

class ForExprAST : public ExprAST {
    std::unique_ptr<ExprAST> start, end, step, body;

public :
    ForExprAST(
            std::unique_ptr<ExprAST> start,
            std::unique_ptr<ExprAST> end,
            std::unique_ptr<ExprAST> step,
            std::unique_ptr<ExprAST> body) :
            start(std::move(start)),
            end(std::move(end)),
            step(std::move(step)),
            body(std::move(body)) {}

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
    WhileExprAST(std::unique_ptr<ExprAST> cond, std::unique_ptr<ExprAST> body)
            : cond(std::move(cond)), body(std::move(body)) {}

    Value *codegen(bool wantPtr = false) override;
};

class DoExprAST : public ExprAST {
    std::unique_ptr<ExprAST> cond, body;

public:
    DoExprAST(std::unique_ptr<ExprAST> cond, std::unique_ptr<ExprAST> body)
            : cond(std::move(cond)), body(std::move(body)) {}

    Value *codegen(bool wantPtr = false) override;
};