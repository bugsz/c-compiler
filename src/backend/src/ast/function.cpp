#include <bits/stdc++.h>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include "llvm_utils.h"
#include "type_utils.h"
#include "ast/function.h"


Function *PrototypeAST::codegen(bool wantPtr) {
    auto func = getFunction(name);
    if(!func){
        std::vector<Type *> llvmArgs;
        for(auto iter=args.begin(); iter!=args.end(); iter++){        
            llvmArgs.push_back(getVarType((*iter).second));
        }
        FunctionType *functionType = FunctionType::get(getVarType(retVal), llvmArgs, va);
        func = Function::Create(functionType, Function::ExternalLinkage, name, llvmModule.get());
        func->setCallingConv(CallingConv::C);
    }
    auto iter = this->args.begin();
    for (auto &arg: func->args()) {
        std::cout << iter->first << std::endl;
        arg.setName(iter->first);
        iter++;
    }
    return func;
}

Function *FunctionDeclAST::codegen(bool wantPtr) {
    std::cout << "FunctionDeclAST: " << prototype->getName() << std::endl;
    retPtr = retBlock = nullptr;
    auto &p = *prototype;
    Function *currFunction = getFunction(p.getName());
    if (!currFunction) {
        return logErrorF(("Unknown function referenced" + p.getName()).c_str());
    }
    if(!currFunction->empty()){
        return logErrorF(("redefinition of " + p.getName()).c_str());
    }
    BasicBlock* entryBlock = BasicBlock::Create(*llvmContext, "entry", currFunction);
    llvmBuilder->SetInsertPoint(entryBlock);
    // if not void, ret value should be a variable!
    Type *retType = getVarType(p.retVal);
    if(!retType->isVoidTy()){
        retPtr = CreateEntryBlockAllocaWithTypeSize("ret_val", retType);
        llvmBuilder->CreateStore(getInitVal(retType), retPtr);
    }
    resetBlockForControl();
    NamedValues.empty();
    std::map<std::string, AllocaInst *> local_vals;
    NamedValues.push(local_vals);
    for (auto &arg : currFunction->args()) {
        AllocaInst *alloca = CreateEntryBlockAllocaWithTypeSize(arg.getName(), arg.getType());
        llvmBuilder->CreateStore(&arg, alloca);
        NamedValues.top()[std::string(arg.getName())] = alloca;
    }

    llvmBuilder->SetInsertPoint(entryBlock);
    retBlock = BasicBlock::Create(*llvmContext, "ret", currFunction);

    llvmBuilder->SetInsertPoint(retBlock);
    if(retPtr){
        llvmBuilder->CreateRet(llvmBuilder->CreateLoad(retType, retPtr, "final_ret"));
    }else{
        llvmBuilder->CreateRetVoid();
    }
    
    // Create Function Body
    llvmBuilder->SetInsertPoint(entryBlock);
    auto compoundStmt = static_cast<CompoundStmtExprAST *>(body.get());
    auto val = compoundStmt->codegen();
    if(!val) {
        currFunction->eraseFromParent();
        return logErrorF("Error reading body");
    }
    print("Build Successfully");
    // Create default terminator if not(add a default return for all empty labels)
    auto iter = currFunction->getBasicBlockList().begin();
    auto end = currFunction->getBasicBlockList().end();
    for(;iter != end ;++iter){
        if(!iter->getTerminator()){
            llvmBuilder->SetInsertPoint(dyn_cast<BasicBlock>(iter));
            llvmBuilder->CreateBr(retBlock);
        }
    }
    verifyFunction(*currFunction, &errs());
    resetBlockForControl();
    llvmFPM->run(*currFunction);
    return currFunction;
}

Value *CallExprAST::codegen(bool wantPtr) {
    std::cout << "Call to: " + callee << std::endl;
    // 如果是builtin function
    if(callee.find("__builtin_") == 0){
        auto builtin_ret = getBuiltinFunction(callee, args);
        if(builtin_ret) return builtin_ret;
    }

    Function *calleeFunction = llvmModule->getFunction(callee);
    if (!calleeFunction) return logErrorV("Unknown function");
    if (calleeFunction -> arg_size() != args.size() && !calleeFunction->isVarArg()) 
        logErrorV("Incorrect args");

    std::vector<Value *> argsValue;
    auto iter = calleeFunction->arg_begin();
    for (auto &arg: args) {
        Value *argValue = arg->codegen();
        if(iter != calleeFunction->arg_end()){
            argValue = createCast(argValue, iter->getType());
            if (!argValue) return logErrorV("Incorrect type function arg");
            iter++;
        }
        std::cout << "Arg value: " + getLLVMTypeStr(argValue) << std::endl;
        argsValue.push_back(argValue);
    }
    return llvmBuilder->CreateCall(calleeFunction, argsValue);
}

Value *ReturnStmtExprAST::codegen(bool wantPtr) {
    print("Find return stmt");
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    if(!retPtr){
        return llvmBuilder->CreateBr(retBlock);
    }

    if (!body) return logErrorV("No ret val");
    Value *retVal = body->codegen();
    retVal = createCast(retVal, currFunction->getReturnType());
    if (!retVal) return logErrorV("No ret val");
    std::cout << "Get return value: " << getLLVMTypeStr(retVal) << std::endl;
    llvmBuilder->CreateStore(retVal, retPtr);
    
    llvmBuilder->CreateBr(retBlock);
    return retVal;
}
