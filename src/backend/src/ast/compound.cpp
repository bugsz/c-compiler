


#include "llvm/ADT/Optional.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"

#include "ast/compound.h"

using namespace llvm;

Value *CompoundStmtExprAST::codegen(bool wantPtr) {
    std::map<std::string, AllocaInst *> Scope = NamedValues.top();
    NamedValues.push(Scope);
    Value *retVal = llvmBuilder->getTrue();
    for (auto &expr: exprList){
        retVal = expr->codegen();
        if(!retVal) return nullptr;
    }
    NamedValues.pop();
    return retVal;
}