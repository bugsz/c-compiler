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
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include "ast.h"

using namespace llvm;

static std::unique_ptr<LLVMContext> llvmContext;
static std::unique_ptr<Module> llvmModule;
static std::map<std::string, Value *> NamedValues;
static std::unique_ptr<IRBuilder<>> llvmBuilder;

static void initializeModule() {
    llvmContext = std::make_unique<LLVMContext>();
    llvmModule = std::make_unique<Module>("JIT", *llvmContext);

}

std::unique_ptr<ExprAST> logError(const char *str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

Value *logErrorV(const char *str) {
    logError(str);
}

Value *DoubleExprAST::codegen() {
  return ConstantFP::get(*llvmContext, APFloat(val));
}

Value *VariableExprAST::codegen() {
    Value *V = NamedValues[Name];
    if (!V)
        return logErrorV("Unknown variable name");
    return V;
}

Value *BinaryExprAST::codegen() {
    Value *left = lhs->codegen();
    Value *right = rhs->codegen();

    if (!left || !right) {
        return nullptr;
    }

    switch(op) {
        case '+':
            return llvmBuilder->CreateFAdd(left, right, "add");
        case '-':
            return llvmBuilder->CreateFSub(left, right, "sub");
        case '*':
            return llvmBuilder->CreateFMul(left, right, "mul");
        case '<':
            left = llvmBuilder->CreateFCmpULT(left, right, "lt");
            return llvmBuilder->CreateUIToFP(left, Type::getDoubleTy(*llvmContext), "booltmp");
        default:
            return logErrorV("Invalid binary operator");
    }
}

Value *IfExprAST::codegen() {
    Value *condValue = cond->codegen();
    if (!condValue) return nullptr;

    condValue = llvmBuilder->CreateFCmpONE(
            condValue, ConstantFP::get(*llvmContext, APFloat(0.0)), "if_cond");

    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    BasicBlock *thenBlock = BasicBlock::Create(*llvmContext, "then_case", currFunction);
    BasicBlock *elseBlock = BasicBlock::Create(*llvmContext, "else_case");
    BasicBlock *finalBlock = BasicBlock::Create(*llvmContext, "final");

    llvmBuilder->CreateCondBr(condValue, thenBlock, elseBlock);
    llvmBuilder->SetInsertPoint(elseBlock);

    Value *thenValue = then_case->codegen();
    if (!thenValue) return nullptr;

    currFunction->getBasicBlockList().push_back(elseBlock);
    llvmBuilder->SetInsertPoint(elseBlock);

    Value *elseValue = else_case->codegen();
    if (!elseValue) return nullptr;

    llvmBuilder->CreateBr(finalBlock);
    elseBlock = llvmBuilder->GetInsertBlock();

    currFunction->getBasicBlockList().push_back(finalBlock);
    llvmBuilder->SetInsertPoint(finalBlock);
    PHINode *PN = llvmBuilder->CreatePHI(Type::getDoubleTy(*llvmContext), 2, "if");

    PN->addIncoming(thenValue, thenBlock);
    PN->addIncoming(elseValue, elseBlock);
    return PN;
}

Value *ForExprAST::codegen() {
    /*
     * phi node: i {label: value}
     * entry, loop, after_loop
     */
    Value *startVal = start->codegen();
    if (!startVal) return nullptr;

    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    BasicBlock *headerBlock = llvmBuilder->GetInsertBlock();
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "for_loop", currFunction);
    llvmBuilder->CreateBr(loopBlock);

    // setup header block
    llvmBuilder->SetInsertPoint(loopBlock);
    PHINode *currNode = llvmBuilder->CreatePHI(Type::getDoubleTy(*llvmContext), 2, var_name.c_str());
    currNode->addIncoming(startVal, headerBlock);

    Value *oldVal = NamedValues[var_name];
    NamedValues[var_name] = currNode;


    Value *stepVal = step->codegen();
    if (!stepVal) return nullptr;
    Value *nextVal = llvmBuilder->CreateFAdd(currNode, stepVal, "next_val");

    Value *endVal = end->codegen();
    if (!endVal) return nullptr;
    endVal = llvmBuilder->CreateFCmpONE(endVal, ConstantFP::get(*llvmContext, APFloat(0.0)), "loop_end");

    BasicBlock *loopEndBlock = llvmBuilder->GetInsertBlock();
    BasicBlock *afterBlock = BasicBlock::Create(*llvmContext, "after_loop", currFunction);

    llvmBuilder->CreateCondBr(endVal, loopBlock, afterBlock);
    llvmBuilder->SetInsertPoint(afterBlock);

    currNode->addIncoming(nextVal, loopEndBlock);

    // restore values
    if (oldVal) NamedValues[var_name] = oldVal;
    else NamedValues.erase(var_name);

    return Constant::getNullValue(Type::getDoubleTy(*llvmContext));

}

int main() {
    return 0;
}