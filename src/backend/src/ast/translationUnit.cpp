

#include "llvm/ADT/Optional.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"

#include "llvm_utils.h"
#include "type_utils.h"
#include "ast/translationUnit.h"

using namespace llvm;

extern std::unique_ptr<LLVMContext> llvmContext;
extern std::unique_ptr<Module> llvmModule;
extern bool initializing;
extern Value* retPtr;
extern BasicBlock* retBlock;
extern std::stack<std::map<std::string, AllocaInst *>> NamedValues;
extern std::unique_ptr<IRBuilder<>> llvmBuilder;
extern std::unique_ptr<legacy::FunctionPassManager> llvmFPM;

Value *TranslationUnitExprAST::codegen(bool wantPtr) {
    // print("TranslationUnit");
    int globalVar_len = globalVarList.size();
    int expr_len = exprList.size();
    // print("Global vars: "+ std::to_string(globalVar_len) + "\nExprs: " + std::to_string(expr_len));
    // print("Global(s) Initializing Pass......");
    initializing = true;
    for (int i=0; i < globalVar_len; i++) {
        globalVarList[i]->codegen();
    }
    initializing = false;
    // print("Global(s) Initializing End......");

    for (int i=0; i < expr_len; i++) {
        if(exprList[i]->codegen() == nullptr) exit(-1);
    }
    return nullptr;
}