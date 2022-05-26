

#include "llvm/ADT/Optional.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm_utils.h"
#include "ast/varref.h"

using namespace llvm;

Value *VarRefExprAST::codegen(bool wantPtr) {
    // std::cout << "VarRefExpr: " << name << std::endl;
    // std::cout << "WantPtr: " << wantPtr << std::endl;
    int isGlobal = 0;
    auto V = getVariable(name, isGlobal);
    if (!V) {
        return logErrorV("Undefined variable");
    }

    if(wantPtr) {
        return V;
    }
    if(isGlobal) {
        // std::cout << "Find global var: " << name << std::endl;
        GlobalVariable *gV = static_cast<GlobalVariable *>(V);
        if(initializing){
            return gV->getInitializer();
        }else if(gV->getType()->getPointerElementType()->isArrayTy()){
            return llvmBuilder->CreateGEP(gV->getType()->getPointerElementType(), gV, {llvmBuilder->getInt64(0), llvmBuilder->getInt64(0)});
        }else{
            return llvmBuilder->CreateLoad(V->getType()->getPointerElementType(), V);
        }
    }

    if(V->getType()->getPointerElementType()->isArrayTy()){
        return llvmBuilder->CreateGEP(V->getType()->getPointerElementType(), V, {llvmBuilder->getInt64(0), llvmBuilder->getInt64(0)});
    }else{
        return llvmBuilder->CreateLoad(getVarType(this->type), V, name.c_str());
    }
}