#include <bits/stdc++.h>


#include "llvm/ADT/Optional.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm_utils.h"
#include "ast/array.h"

using namespace llvm;

Value *ArrayExprAST::codegen(bool wantPtr) {
    auto varType = getVarType(this->getType());
    auto arrayType = ArrayType::get(varType, size);
    auto arrayPtr = llvmBuilder->CreateAlloca(arrayType, nullptr, name);
    NamedValues.top()[name] = arrayPtr;

    int initSize = 0;
    if(init.size() == 1) initSize = size;
    if(init.size() > 1) initSize = init.size();

    auto zeroInit = llvmBuilder->getInt64(0);
    for (int i=0; i < initSize; i++) {
        auto defaultValue = (init.size() == 1) ? init[0]->codegen() : init[i]->codegen();
        defaultValue = createCast(defaultValue, varType);
        if(!defaultValue) return logErrorV("Initializer type don't match");
        auto index = llvmBuilder->getInt64(i);
        auto arrayElem = llvmBuilder->CreateGEP(arrayType, arrayPtr, {zeroInit, index});
        llvmBuilder->CreateStore(defaultValue, arrayElem);
    }
    return arrayPtr;
}

Value *GlobalArrayExprAST::codegen(bool wantPtr) {
    // std::cout << "GlobalArrayExpr" << std::endl;
    auto varType = getVarType(init->getType());
    auto arrayType = ArrayType::get(varType, init->getSize());

    llvmModule->getOrInsertGlobal(init->getName(), arrayType);
    GlobalVariable *gv = llvmModule->getNamedGlobal(name);
    gv->setConstant(false);
    std::vector<Constant *> initVals;
    
    int totalSize = init->getSize();
    int initSize = init->init.size() == 1 ? totalSize : init->init.size();
    for (int i = 0; i < initSize; i++) {
        auto initVal = init->init[init->init.size() == 1 ? 0: i]->codegen();
        initVal = createCast(initVal, varType);
        if(!initVal){
            return logErrorV((std::string("Unsupported initializatin from "+ getLLVMTypeStr(initVal->getType()) +" to " + getLLVMTypeStr(varType)).c_str()));
        }
        initVals.push_back(dyn_cast<Constant>(initVal));
    }

    for(int i = initSize; i < totalSize; i++) {
        auto initVal = getInitVal(varType);
        initVals.push_back(initVal);
    }

    auto initArray = ConstantArray::get(arrayType, initVals);
    gv->setInitializer(initArray);
    return getInitVal(varType);
}