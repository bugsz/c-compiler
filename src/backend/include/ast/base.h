#pragma once

#include "llvm/IR/Value.h"

using namespace llvm;

class ExprAST {
public:
    virtual ~ExprAST() = default;

    virtual Value *codegen(bool wantPtr = false) = 0;
};