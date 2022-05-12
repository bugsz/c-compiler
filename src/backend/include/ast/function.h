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
#include "llvm_utils.h"
#include "type_utils.h"

using namespace llvm;

extern std::unique_ptr<LLVMContext> llvmContext;
extern std::unique_ptr<Module> llvmModule;
extern bool initializing;
extern Value *retPtr;
extern BasicBlock *retBlock;
extern std::stack<std::map<std::string, AllocaInst *>> NamedValues;
extern std::unique_ptr<IRBuilder<>> llvmBuilder;
extern std::unique_ptr<legacy::FunctionPassManager> llvmFPM;

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

class FunctionDeclAST : public ExprAST {
    std::unique_ptr<PrototypeAST> prototype;
    std::unique_ptr<ExprAST> body;

    Function *codegen(bool wantPtr = false) override;

public:
    FunctionDeclAST(std::unique_ptr<PrototypeAST> prototype, std::unique_ptr<ExprAST> body)
            : prototype(std::move(prototype)), body(std::move(body)) {}
};

class CallExprAST : public ExprAST {
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;

public:
    CallExprAST(const std::string &callee, std::vector<std::unique_ptr<ExprAST>> args)
            : callee(callee), args(std::move(args)) {}

    Value *codegen(bool wantPtr = false) override;
};

class ReturnStmtExprAST : public ExprAST {
    std::unique_ptr<ExprAST> body;
public:
    ReturnStmtExprAST(std::unique_ptr<ExprAST> body)
            : body(std::move(body)) {}

    Value *codegen(bool wantPtr = false) override;
};
