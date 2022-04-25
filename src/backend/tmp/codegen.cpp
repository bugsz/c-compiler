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
#include "llvm/Support/TargetRegistry.h"
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
    llvmBuilder = std::make_unique<IRBuilder<>>(*llvmContext);
}

std::unique_ptr<ExprAST> logError(const char *str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

Value *logErrorV(const char *str) {
    logError(str);
    return nullptr;
}

Value *DoubleExprAST::codegen() {
    // std::cout << "codegen for double" << std::endl;
    return ConstantFP::get(*llvmContext, APFloat(val));
}

Value *VariableExprAST::codegen() {
    Value *V = NamedValues[name];
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
    BasicBlock *afterBlock = BasicBlock::Create(*llvmContext, "after_for_loop", currFunction);

    llvmBuilder->CreateCondBr(endVal, loopBlock, afterBlock);
    llvmBuilder->SetInsertPoint(afterBlock);

    currNode->addIncoming(nextVal, loopEndBlock);

    // restore values
    if (oldVal) NamedValues[var_name] = oldVal;
    else NamedValues.erase(var_name);

    return Constant::getNullValue(Type::getDoubleTy(*llvmContext));
}

Value *WhileExprAST::codegen() {
    // TODO
    Value *condVal = cond->codegen();
    if (!condVal) return nullptr;

    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    BasicBlock *entryBlock = BasicBlock::Create(*llvmContext, "entry", currFunction);
    llvmBuilder->SetInsertPoint(entryBlock);

    Value *endVal = llvmBuilder->CreateFCmpONE(condVal, ConstantFP::get(*llvmContext, APFloat(0.0)));
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "while_loop_body", currFunction);
    BasicBlock *endBlock = BasicBlock::Create(*llvmContext, "while_loop_end", currFunction);

    llvmBuilder->CreateCondBr(endVal, loopBlock, endBlock);
    llvmBuilder->SetInsertPoint(endBlock);

    return Constant::getNullValue(Type::getDoubleTy(*llvmContext));
}

Value *CallExprAST::codegen() {
    std::cout << callee << std::endl;
    Function *calleeFunction = llvmModule->getFunction(callee);
    if (!calleeFunction) return logErrorV("Unknown function");
    if (calleeFunction->arg_size() != args.size()) logErrorV("Incorrect args");

    std::vector<Value *> argsValue;
    for (auto &arg: args) {
        Value *argValue = arg->codegen();
        if (!argValue) return nullptr;
        argsValue.push_back(argValue);
    }

    return llvmBuilder->CreateCall(calleeFunction, argsValue, "calltmp");
}

Function *PrototypeAST::codegen() {
    std::vector<Type *> data_type(args.size(), Type::getDoubleTy(*llvmContext));
    FunctionType *ty = FunctionType::get(Type::getDoubleTy(*llvmContext), data_type, false);

    Function *F = Function::Create(ty, Function::ExternalLinkage, name, llvmModule.get());

//    unsigned idx = 0;
//    for (auto &arg: F->args())
//        arg.setName(args[idx++]);

    return F;
}

Function *FunctionAST::codegen() {
    Function *currFunction = llvmModule->getFunction(proto->getName());
    if (!currFunction) currFunction = proto->codegen();
    if (!currFunction) return nullptr;
    if (!currFunction->empty()) return (Function *)logErrorV("Function cannot be redefined");

    BasicBlock *BB = BasicBlock::Create(*llvmContext, "entry", currFunction);
    llvmBuilder->SetInsertPoint(BB);

    NamedValues.clear();
    for (auto &arg: currFunction->args())
        NamedValues[std::string(arg.getName())] = &arg;

    Value *retVal = body->codegen();
    if (retVal) {
        llvmBuilder->CreateRet(retVal);
        verifyFunction(*currFunction);

        return currFunction;
    }


    // TODO: support explicit return value
    currFunction->eraseFromParent();
    return nullptr;
}

void compile() {
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    auto targetTriple = sys::getDefaultTargetTriple();

    auto cpu = "generic";
    auto features = "";

    std::string error;
    auto target = TargetRegistry::lookupTarget(targetTriple, error);

    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto theTargetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, RM);

    llvmModule->setDataLayout(theTargetMachine->createDataLayout());

    auto filename = "output.o";
    std::error_code EC;
    raw_fd_ostream dest(filename, EC, sys::fs::OF_None);

    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;

    pass.run(*llvmModule);
    dest.flush();
}

void testAst() {
//    DoubleExprAST doubleExprAst(0.5);
//    doubleExprAst.codegen();

    auto astD1 = std::make_unique<DoubleExprAST>(0.5);
    auto astD2 = std::make_unique<DoubleExprAST>(0.5);

    std::string name("test"), arg1("x"), arg2("y");
    std::vector<std::string> args({arg1, arg2});

    auto astVariable1 = std::make_unique<VariableExprAST>(arg1);
    auto astVariable2 = std::make_unique<VariableExprAST>(arg2);
    auto astV2 = std::make_unique<BinaryExprAST>('*', std::move(astVariable1), std::move(astVariable2));
    auto astProto1 = std::make_unique<PrototypeAST>(name, std::move(args));
    astProto1->codegen();
    auto astFunc1 = std::make_unique<FunctionAST>(std::move(astProto1), std::move(astV2));

//    fprintf(stderr, "read function definition: \n");
//    astFunc1->codegen()->print(errs());
//    fprintf(stderr, "\n");


    std::vector<std::unique_ptr<ExprAST>> doubleArgs;
    doubleArgs.push_back(std::move(astD1));
    doubleArgs.push_back(std::move(astD2));
    auto astCall1 = std::make_unique<CallExprAST>(name, std::move(doubleArgs));

    // fprintf(stderr, "read function definition");
    astCall1->codegen()->print(errs());

}

int main() {
    initializeModule();
    testAst();
    llvmModule->print(errs(), nullptr);
    return 0;
}