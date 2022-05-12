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

class ArrayExprAST : public ExprAST {
    int type;
    std::string name;
    int size;


public:
    std::vector<std::unique_ptr<ExprAST>> init;

    const std::string &getName() const { return name; }

    const int getSize() const { return size; }

    const int getType() const { return type; }

    ArrayExprAST(int type, const std::string &name, int size, std::vector<std::unique_ptr<ExprAST>> init) : type(type),
                                                                                                            name(name),
                                                                                                            size(size),
                                                                                                            init(std::move(
                                                                                                                    init)) {}

    Value *codegen(bool wantPtr = false) override;
};

class GlobalArrayExprAST : public ExprAST {
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
