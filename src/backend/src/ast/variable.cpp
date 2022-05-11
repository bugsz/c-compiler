#include <bits/stdc++.h>

#include "llvm/ADT/Optional.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"

#include "llvm_utils.h"
#include "type_utils.h"
#include "ast/variable.h"

using namespace llvm;

Value *VarExprAST::codegen(bool wantPtr) {
    print("VarExpr, typeid: " + std::to_string(type));
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    auto varType = getVarType(this->getType());
    std::cout << getLLVMTypeStr(varType) << std::endl;
    Value *initVal;
    if(init) {
        initVal = init->codegen();
    } else {
        initVal = getInitVal(varType);
    }
    auto castedVal = createCast(initVal, varType);
    if(!castedVal){
        return logErrorV((std::string("Unsupported initializatin from "+ getLLVMTypeStr(initVal->getType()) +" to " + getLLVMTypeStr(varType)).c_str()));
    }
    AllocaInst *alloca = CreateEntryBlockAllocaWithTypeSize(name, varType);
    llvmBuilder->CreateStore(castedVal, alloca);
    NamedValues.top()[name] = alloca;
    return castedVal;
}

Value *GlobalVarExprAST::codegen(bool wantPtr) {
    auto varType = getVarType(this->init->getType());
    print("GlobalVarExpr, Name: " + init->name + " Type: " + getLLVMTypeStr(varType));

    Value *initVal;

    if(init->init) {
        initVal = init->init->codegen();
        initVal = createCast(initVal, varType);
        if(!initVal) 
            return nullptr;
    }else{
        initVal = getInitVal(varType);
    }

    auto globalVar = createGlob(varType, init->name);
    globalVar->setInitializer(dyn_cast<Constant>(initVal));
    return initVal;
}