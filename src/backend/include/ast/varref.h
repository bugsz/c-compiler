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


class VarRefExprAST : public ExprAST {
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

