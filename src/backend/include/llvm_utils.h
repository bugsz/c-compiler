#pragma once
#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"

#include "ast/base.h"
#include "ast/array.h"
#include "ast/compound.h"
#include "ast/control_flow.h"
#include "ast/function.h"
#include "ast/literal.h"
#include "ast/null.h"
#include "ast/operator.h"
#include "ast/translationUnit.h"
#include "ast/variable.h"
#include "ast/varref.h"

#define INTEGER_BITWIDTH 32

using namespace llvm;



template<typename T>
std::unique_ptr<T> logError(const char *str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}


template<typename T> 
static std::string getLLVMTypeStr(T* value_or_type) {
    std::string str;
    raw_string_ostream stream(str);
    value_or_type->print(stream);
    return str;
}

Value *logErrorV(const char *str);
Function *logErrorF(const char *str);

Constant *getInitVal(Type *type);
GlobalVariable *createGlob(Type *type, std::string name);
Function *getFunction(std::string name);
Value *getBuiltinFunction(std::string callee, std::vector<std::unique_ptr<ExprAST>> &args);

bool isValidBinaryOperand(Value *value);

Value *getVariable(std::string name);
Value *getVariable(std::string name, int &isGlobal);

Type *getVarType(int type_id);

Value *createCast(Value *value, Type *type);



void popBlockForControl();
void resetBlockForControl();

AllocaInst *CreateEntryBlockAllocaWithTypeSize(StringRef VarName, Type* type, Value* size = nullptr, BasicBlock* Scope = llvmBuilder->GetInsertBlock(), BasicBlock::iterator Point =llvmBuilder->GetInsertPoint());

AllocaInst *CreateEntryBlockAllocaWithTypeSize(StringRef VarName, int type_id, Value* size = nullptr, BasicBlock* Scope = llvmBuilder->GetInsertBlock(), BasicBlock::iterator Point =llvmBuilder->GetInsertPoint());