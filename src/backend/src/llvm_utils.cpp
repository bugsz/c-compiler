#include <bits/stdc++.h>
#include "llvm_utils.h"
#include "type_utils.h"
#include "ast/base.h"
#include "ast/variable.h"
#include "ast/varref.h"

using namespace llvm;

std::unique_ptr<LLVMContext> llvmContext;
std::unique_ptr<Module> llvmModule;
bool initializing;
Value* retPtr;
BasicBlock* retBlock;
std::stack<std::map<std::string, AllocaInst *>> NamedValues;
std::unique_ptr<IRBuilder<>> llvmBuilder;
std::unique_ptr<legacy::FunctionPassManager> llvmFPM;

std::stack<BasicBlock *> BlockForEndif;
std::stack<BasicBlock *> BlockForBreak;
std::stack<BasicBlock *> BlockForContinue;

Value *logErrorV(const char *str) {
    logError<ExprAST>(str);
    return nullptr;
}

Function *logErrorF(const char *str) {
    logError<ExprAST>(str);
    return nullptr;
}

Constant *getInitVal(Type *type) {
    if(type->isFloatTy())
        return ConstantFP::get(llvmBuilder->getFloatTy(), 0);
    else if(type->isDoubleTy()) 
        return ConstantFP::get(llvmBuilder->getDoubleTy(), 0);
    else if(type->isIntegerTy(1)) 
        return llvmBuilder->getInt1(false);
    else if(type->isIntegerTy(8))
        return llvmBuilder->getInt8(0);
    else if(type->isIntegerTy(16))
        return llvmBuilder->getInt16(0);
    else if(type->isIntegerTy(32))
        return llvmBuilder->getInt32(0);
    else if(type->isIntegerTy(64))
        return llvmBuilder->getInt64(0);
    else{
        //others are all pointers(including void *)
        return ConstantExpr::getIntToPtr(llvmBuilder->getInt64(0), type);
    }
}

GlobalVariable *createGlob(Type *type, std::string name) {
    llvmModule->getOrInsertGlobal(name, type);
    GlobalVariable *gv = llvmModule->getNamedGlobal(name);
    gv->setConstant(false);
    return gv;
}

Function *getFunction(std::string name) {
    if (auto *F = llvmModule->getFunction(name)) {
        return F;
    }
    return nullptr;
}

Value *getBuiltinFunction(std::string callee, std::vector<std::unique_ptr<ExprAST>> &args) {
    std::vector<Value *> varArgs;
    std::string func_name = callee.substr(strlen("__builtin_"), callee.length());
    auto func = getFunction(func_name);
    if(!func){
        fprintf(stderr, "error: use of unknown builtin '%s'\n", callee.c_str());
        exit(-1);
    }
    return llvmBuilder->CreateCall(func, varArgs);
}

bool isValidBinaryOperand(Value *value) {
    return (value->getType()->isFloatingPointTy() || value->getType()->isIntegerTy());
}

Value *getVariable(std::string name) {
    auto V = NamedValues.top()[name];
    if(V) {
        return V;
    }
    auto key = llvmModule->getGlobalVariable(name);
    if (key) {
        return key;
    }
    return nullptr;
}

Value *getVariable(std::string name, int &isGlobal) {
    auto V = NamedValues.top()[name];
    if(V) {
        std::cout << "Find local variable " << name << std::endl;
        return V;
    }
    auto key = llvmModule->getGlobalVariable(name);
    if (key) {
        std::cout << "Find global variable: " << name << std::endl;
        isGlobal = 1;
        return key;
    }
    return nullptr;
}


Type *getVarType(int type_id) {
    switch(type_id) {
        case TYPEID_VOID:
            return Type::getVoidTy(*llvmContext);
        case TYPEID_CHAR:
            return Type::getInt8Ty(*llvmContext);
        case TYPEID_SHORT:
            return Type::getInt16Ty(*llvmContext);
        case TYPEID_INT:
            if(INTEGER_BITWIDTH == 32) 
                return Type::getInt32Ty(*llvmContext);
            else 
                return Type::getInt64Ty(*llvmContext);
        case TYPEID_LONG:
            return Type::getInt64Ty(*llvmContext);
        case TYPEID_FLOAT:
            return Type::getFloatTy(*llvmContext);
        case TYPEID_DOUBLE:
            return Type::getDoubleTy(*llvmContext);
        case TYPEID_STR:
            return Type::getInt8PtrTy(*llvmContext);
        case TYPEID_VOID_PTR:
            return Type::getInt8PtrTy(*llvmContext);
        case TYPEID_CHAR_PTR:
            return Type::getInt8PtrTy(*llvmContext);
        case TYPEID_SHORT_PTR:
            return Type::getInt16PtrTy(*llvmContext);        
        case TYPEID_INT_PTR:
            if(INTEGER_BITWIDTH == 32) 
                return Type::getInt32PtrTy(*llvmContext);
            else 
                return Type::getInt64PtrTy(*llvmContext);
        case TYPEID_LONG_PTR:
            return Type::getInt64PtrTy(*llvmContext);        
        case TYPEID_FLOAT_PTR:
            return Type::getFloatPtrTy(*llvmContext);        
        case TYPEID_DOUBLE_PTR:
            return Type::getDoublePtrTy(*llvmContext);        
        default:
            return nullptr;
    }
}

// 将value转成想要的type类型
Value *createCast(Value *value, Type *type) {
    if(!value) return nullptr;
    std::cout << "val-type:" << getLLVMTypeStr(value) << " want-type: " << getLLVMTypeStr(type) << std::endl;
    if(value->getType() == type){
        print("No need to cast");
        return value;
    }
    auto val_type = value->getType();
    if(val_type->isFloatingPointTy()){
        print("Val belongs to FloatingPoint");
        if(type->isFloatingPointTy()){
            return llvmBuilder->CreateFPCast(value, type);
        }else if(type->isIntegerTy()){
            return llvmBuilder->CreateCast(Instruction::FPToSI, value, type);
        }else if(type->isPtrOrPtrVectorTy()){
            return nullptr;
        }
    }
    if(val_type->isIntegerTy()){
        print("Val belongs to Integer");
        if(type->isFloatingPointTy()){
            return llvmBuilder->CreateCast(Instruction::SIToFP, value, type);
        }else if(type->isIntegerTy()){
            return llvmBuilder->CreateSExtOrTrunc(value, type);
        }else if(type->isPtrOrPtrVectorTy()){
            return llvmBuilder->CreateCast(Instruction::IntToPtr, value, type);
        }
    }
    if(val_type->isPtrOrPtrVectorTy()){
        print("Val belongs to Pointer");
        if(type->isFloatingPointTy()){
            return nullptr;
        }else if(type->isIntegerTy()){
            return llvmBuilder->CreateCast(Instruction::PtrToInt, value, type);
        }else if(type->isPtrOrPtrVectorTy()){
            return llvmBuilder->CreatePointerCast(value, type);
        }
    }
    print("Unknown type to be cast");
    return nullptr;
}

void popBlockForControl() {
    BlockForBreak.pop();
    BlockForContinue.pop();
}

void resetBlockForControl() {
    BlockForBreak.empty();
    BlockForContinue.empty();
}


AllocaInst *CreateEntryBlockAllocaWithTypeSize(StringRef VarName, Type* type, Value* size, BasicBlock* Scope, BasicBlock::iterator Point) {
    IRBuilder<> allocator(Scope, Point);
    return allocator.CreateAlloca(type, size, VarName);
}

AllocaInst *CreateEntryBlockAllocaWithTypeSize(StringRef VarName, int type_id, Value* size, BasicBlock* Scope, BasicBlock::iterator Point ) {
    IRBuilder<> allocator(Scope, Point);
    return allocator.CreateAlloca(getVarType(type_id), size, VarName);
}