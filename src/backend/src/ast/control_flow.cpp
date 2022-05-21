#include <bits/stdc++.h>

#include "llvm/ADT/Optional.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm_utils.h"
#include "type_utils.h"
#include "ast/control_flow.h"

using namespace llvm;

Value *ForExprAST::codegen(bool wantPtr) {
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    Value *startExpr = start->codegen();
    if (!startExpr) return nullptr;    
    BasicBlock *afterBlock = BasicBlock::Create(*llvmContext, "after_for_loop", currFunction, retBlock);
    BasicBlock *stepBlock = BasicBlock::Create(*llvmContext, "for_step", currFunction, afterBlock);
    BasicBlock *bodyBlock = BasicBlock::Create(*llvmContext, "for_body", currFunction, stepBlock);
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "for_loop", currFunction, bodyBlock);

    BlockForBreak.push(afterBlock);
    BlockForContinue.push(stepBlock);

    llvmBuilder->CreateBr(loopBlock);
    llvmBuilder->SetInsertPoint(loopBlock);
    // Check first
    Value *endVal = end->codegen();
    if (!endVal) return nullptr;
    if(endVal->getType()->isFloatingPointTy())
        endVal = llvmBuilder->CreateFCmpONE(endVal, getInitVal(endVal->getType()), "loop_end");
    else if(endVal->getType()->isIntegerTy(INTEGER_BITWIDTH))
        endVal = llvmBuilder->CreateICmpNE(endVal, getInitVal(endVal->getType()), "loop_end");

    llvmBuilder->CreateCondBr(endVal, bodyBlock, afterBlock);
    llvmBuilder->SetInsertPoint(bodyBlock);
    if (!body->codegen()) return nullptr;
    if(!llvmBuilder->GetInsertBlock()->getTerminator()){
        llvmBuilder->CreateBr(stepBlock);
    }
    llvmBuilder->SetInsertPoint(stepBlock);
    Value *stepVar = step->codegen();
    if (!stepVar) return nullptr;
    if(!llvmBuilder->GetInsertBlock()->getTerminator()){
        llvmBuilder->CreateBr(loopBlock);
    }
    llvmBuilder->SetInsertPoint(afterBlock);
    popBlockForControl();
    return llvmBuilder->getTrue();
}

Value *IfExprAST::codegen(bool wantPtr) {
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    BasicBlock *endifBlock = BasicBlock::Create(*llvmContext, "endif", currFunction, retBlock);
    BasicBlock *thenBlock = BasicBlock::Create(*llvmContext, "then_case", currFunction, endifBlock);
    BasicBlock *ifBlock = BasicBlock::Create(*llvmContext, "if", currFunction, thenBlock);
    llvmBuilder->CreateBr(ifBlock);
    llvmBuilder->SetInsertPoint(ifBlock);
    Value *condValue = cond->codegen();
    if (!condValue) return nullptr;
    auto condType = condValue->getType();
    Value * condVal;
    if(condType->isFloatingPointTy())
        condVal = llvmBuilder->CreateFCmpONE(condValue, getInitVal(condType), "if_comp");
    else
        condVal = llvmBuilder->CreateICmpNE(condValue, getInitVal(condType), "if_comp");

    if(else_case){
        BasicBlock *elseBlock = BasicBlock::Create(*llvmContext, "else_case", currFunction, endifBlock);
        llvmBuilder->CreateCondBr(condVal, thenBlock, elseBlock);

        llvmBuilder->SetInsertPoint(thenBlock);
        Value *thenValue = then_case->codegen();
        if (!thenValue) return nullptr;
        if(!llvmBuilder->GetInsertBlock()->getTerminator()){
            llvmBuilder->CreateBr(endifBlock);
        }

        llvmBuilder->SetInsertPoint(elseBlock);
        Value *elseValue = else_case->codegen();
        if (!elseValue) return nullptr;
        if(!llvmBuilder->GetInsertBlock()->getTerminator()){
            llvmBuilder->CreateBr(endifBlock);
        }
        llvmBuilder->SetInsertPoint(endifBlock);
        return condValue;
    }else{
        llvmBuilder->CreateCondBr(condVal, thenBlock, endifBlock);
        llvmBuilder->SetInsertPoint(thenBlock);

        Value *thenValue = then_case->codegen();
        if (!thenValue) return nullptr;
        // std::cout << "Return value of then value: " << getLLVMTypeStr(thenValue) << std::endl;
        if(!llvmBuilder->GetInsertBlock()->getTerminator()){
            llvmBuilder->CreateBr(endifBlock);
        }
        llvmBuilder->SetInsertPoint(endifBlock);
        return condValue;
    }
}

Value *WhileExprAST::codegen(bool wantPtr) {
    // print("Generate for while expr");

    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    BasicBlock *endBlock = BasicBlock::Create(*llvmContext, "while_loop_end", currFunction, retBlock);
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "while_loop_body", currFunction, endBlock);
    BasicBlock *entryBlock = BasicBlock::Create(*llvmContext, "entry", currFunction, loopBlock);

    BlockForBreak.push(endBlock);
    BlockForContinue.push(loopBlock);

    llvmBuilder->CreateBr(entryBlock);
    llvmBuilder->SetInsertPoint(entryBlock);
    Value *endVal;

    Value *condVal = cond->codegen();
    if (!condVal) return nullptr;
    auto condType = condVal->getType();

    if(condType->isFloatingPointTy())
        endVal = llvmBuilder->CreateFCmpONE(condVal, getInitVal(condVal->getType()), "while_comp");
    else
        endVal = llvmBuilder->CreateICmpNE(condVal, getInitVal(condVal->getType()), "while_comp");

    llvmBuilder->CreateCondBr(endVal, loopBlock, endBlock);
    llvmBuilder->SetInsertPoint(loopBlock);
    if(!body->codegen()) return nullptr;
    if(!llvmBuilder->GetInsertBlock()->getTerminator()){
            llvmBuilder->CreateBr(entryBlock);
    }
    llvmBuilder->SetInsertPoint(endBlock);
    popBlockForControl();
    return Constant::getNullValue(condType);
}

Value *DoExprAST::codegen(bool wantPtr) {
    // print("Generate do while expr");
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    BasicBlock *endBlock = BasicBlock::Create(*llvmContext, "do_while_loop_end", currFunction, retBlock);
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "do_while_loop_body", currFunction, endBlock);

    BlockForBreak.push(endBlock);
    BlockForContinue.push(loopBlock);

    llvmBuilder->CreateBr(loopBlock);
    llvmBuilder->SetInsertPoint(loopBlock);

    if(!body->codegen()) return nullptr;
    Value *endVal;
    Value *condVal = cond->codegen();

    if (!condVal) return nullptr;
    auto condType = condVal->getType();

    if(condVal->getType()->isFloatingPointTy())
        endVal = llvmBuilder->CreateFCmpONE(condVal, getInitVal(condVal->getType()), "while_comp");
    else
        endVal = llvmBuilder->CreateICmpNE(condVal, getInitVal(condVal->getType()), "while_comp");

    if(!llvmBuilder->GetInsertBlock()->getTerminator()){
        llvmBuilder->CreateCondBr(endVal, loopBlock, endBlock);
    }
    llvmBuilder->SetInsertPoint(endBlock);
    popBlockForControl();
    return Constant::getNullValue(Type::getDoubleTy(*llvmContext));
}

Value *BreakExprAST::codegen(bool wantPtr) {
    // print("Generate for break expr");
    if(BlockForBreak.empty()){
        return logErrorV("Break statement not in loop");
    }
    return llvmBuilder->CreateBr(BlockForBreak.top());
}

Value *ContinueExprAST::codegen(bool wantPtr) {
    // print("Generate for continue expr");
    if(BlockForContinue.empty()){
        return logErrorV("Continue statement not in loop");
    }
    return llvmBuilder->CreateBr(BlockForContinue.top());
}