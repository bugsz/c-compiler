


#include "llvm/ADT/Optional.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm_utils.h"
#include "ast/array.h"

using namespace llvm;

Value *ArrayExprAST::codegen(bool wantPtr) {
    Type* varType = getVarType(type);
    Type* arrayType;
    AllocaInst* arrayPtr;
    if(size >= 0){
        // use a constant to create array
        arrayType = ArrayType::get(varType, size);
        arrayPtr = llvmBuilder->CreateAlloca(arrayType, nullptr, name);
        NamedValues.top()[name] = arrayPtr;
        int initSize = 0;
        if(init.size() == 1) initSize = size;
        if(init.size() > 1) initSize = init.size();
        auto zeroInit = llvmBuilder->getInt64(0);
        for (int i = 0; i < initSize; i++) {
            auto defaultValue = (init.size() == 1) ? init[0]->codegen() : init[i]->codegen();
            defaultValue = createCast(defaultValue, varType);
            if(!defaultValue) return logErrorV("Initializer type don't match");
            auto index = llvmBuilder->getInt64(i);
            auto arrayElem = llvmBuilder->CreateGEP(arrayType, arrayPtr, {zeroInit, index});
            llvmBuilder->CreateStore(defaultValue, arrayElem);
        }
    }else{
        // use variables to create array (describe dimensions)
        int initSize = init.size();
        assert(initSize == 1 || initSize == 2);
        if(initSize == 1){
            Value * totalsize = init[0]->codegen();
            if(!totalsize->getType()->isIntegerTy()){
                return logErrorV("array size must be an integer");
            }
            auto array = CreateEntryBlockAllocaWithTypeSize("", varType, totalsize);
            arrayPtr = CreateEntryBlockAllocaWithTypeSize(name, varType->getPointerTo());
            llvmBuilder->CreateStore(array, arrayPtr);
            NamedValues.top()[name] = arrayPtr;
        }else{
            assert(varType->isPointerTy());
            Value * rows = init[0]->codegen();
            Value * cols = init[1]->codegen();
            if(!rows->getType()->isIntegerTy() || !cols->getType()->isIntegerTy())
                return logErrorV("array size must be an integer");     
            rows = createCast(rows, llvmBuilder->getInt64Ty());
            cols = createCast(cols, llvmBuilder->getInt64Ty());
            Value * totalsize = llvmBuilder->CreateNUWMul(rows, cols);
            auto array = CreateEntryBlockAllocaWithTypeSize("", varType->getPointerElementType(), totalsize);
            auto arrayPtrs = CreateEntryBlockAllocaWithTypeSize("", varType, rows);
            // for loop to initialize
            Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
            BasicBlock *endBlock = BasicBlock::Create(*llvmContext, "init_end", currFunction, retBlock);
            BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "init_body", currFunction, endBlock);
            BasicBlock *entryBlock = BasicBlock::Create(*llvmContext, "init_entry", currFunction, loopBlock);
            auto condVar = CreateEntryBlockAllocaWithTypeSize("", llvmBuilder->getInt64Ty());
            llvmBuilder->CreateStore(rows, condVar);
            llvmBuilder->CreateBr(entryBlock);
            llvmBuilder->SetInsertPoint(entryBlock);
            auto endVal = llvmBuilder->CreateICmpNE(
                llvmBuilder->CreateLoad(llvmBuilder->getInt64Ty(), condVar), 
                getInitVal(llvmBuilder->getInt64Ty()), 
                "init_comp");
            llvmBuilder->CreateCondBr(endVal, loopBlock, endBlock);
            llvmBuilder->SetInsertPoint(loopBlock);
            auto val = llvmBuilder->CreateLoad(llvmBuilder->getInt64Ty(), condVar);
            llvmBuilder->CreateStore(llvmBuilder->CreateSub(val, llvmBuilder->getInt64(1)), condVar);
            val = llvmBuilder->CreateLoad(llvmBuilder->getInt64Ty(), condVar);
            auto offset = llvmBuilder->CreateMul(cols, val);
            auto addr = llvmBuilder->CreateGEP(varType->getPointerElementType(), array, offset);
            auto addrptr = llvmBuilder->CreateGEP(varType, arrayPtrs, val);
            llvmBuilder->CreateStore(addr, addrptr);
            llvmBuilder->CreateBr(entryBlock);
            llvmBuilder->SetInsertPoint(endBlock);
            arrayPtr = CreateEntryBlockAllocaWithTypeSize(name, varType->getPointerTo());
            llvmBuilder->CreateStore(arrayPtrs, arrayPtr);
            NamedValues.top()[name] = arrayPtr;
        }
    }
    return arrayPtr;
}

Value *GlobalArrayExprAST::codegen(bool wantPtr) {
    // std::cout << "GlobalArrayExpr" << std::endl;
    Type* varType = getVarType(init->getType());
    ArrayType* arrayType;
    std::vector<Constant *> initVals;
    if(init->getSize() < 0){
        while(varType->isPointerTy()){
            varType = varType->getPointerElementType();
        }
        int initSize = init->init.size()-1;
        arrayType = ArrayType::get(varType, dyn_cast<ConstantInt>(init->init[initSize]->codegen())->getLimitedValue());
        for (int i = initSize-1; i >= 0; i--) {
            arrayType = ArrayType::get(arrayType, dyn_cast<ConstantInt>(init->init[i]->codegen())->getLimitedValue());
        }
    }else{
        arrayType = ArrayType::get(varType, init->getSize());
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
    }
    llvmModule->getOrInsertGlobal(init->getName(), arrayType);
    GlobalVariable *gv = llvmModule->getNamedGlobal(name);
    gv->setConstant(false);
    auto initArray = ConstantArray::get(arrayType, initVals);
    gv->setInitializer(initArray);
    return getInitVal(varType);
}