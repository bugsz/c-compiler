#pragma once

#include <bits/stdc++.h>

#include "llvm/IR/Module.h"

#include "ast/base.h"
#include "llvm_utils.h"
#include "type_utils.h"

using namespace llvm;


extern std::unique_ptr<Module> llvmModule;


class NullStmtAST : public ExprAST {
    Value *codegen(bool wantPtr = false) override;
};
