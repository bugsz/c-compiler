#include "ast/base.h"
#include "ast/null.h"

using namespace llvm;

Value * NullStmtAST::codegen(bool wantPtr){
    return llvmBuilder->getTrue();
}