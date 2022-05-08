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

#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"


#include "llvm_ast.h"
// #include "type_utils.h"
#include "frontend.h"
#include <bits/stdc++.h>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/StringRef.h>
#include <memory>
#include <string>
#include <unistd.h>
#include <fcntl.h>
using namespace llvm;

static std::unique_ptr<LLVMContext> llvmContext;
static std::unique_ptr<Module> llvmModule;
static bool initializing;
static Value* retPtr;
static BasicBlock* retBlock;
static std::stack<std::map<std::string, AllocaInst *>> NamedValues;
static std::unique_ptr<IRBuilder<>> llvmBuilder;
static std::unique_ptr<legacy::FunctionPassManager> llvmFPM;
static std::map<std::string, std::unique_ptr<PrototypeAST>> functionProtos;

// static BasicBlock *BlockForBreak;
// static BasicBlock *BlockForContinue;
static std::stack<BasicBlock *> BlockForBreak;
static std::stack<BasicBlock *> BlockForContinue;

inline void print(std::string a) { std::cout << a << std::endl; }

int getBinaryOpType(std::string binaryOp) {
    if(binaryOp == "+" ) return ADD;
    if(binaryOp == "-" ) return SUB;
    if(binaryOp == "*" ) return MUL;
    if(binaryOp == "/") return DIV;
    if(binaryOp == "<") return LT;
    if(binaryOp == ">") return GT;
    if(binaryOp == "<=") return LE;
    if(binaryOp == ">=") return GE;    
    if(binaryOp == "==") return EQ;
    if(binaryOp == "!=") return NE;
    if(binaryOp == "=" ) return ASSIGN;
    if(binaryOp =="/=" || binaryOp =="*="|| binaryOp =="-="|| binaryOp =="+=") return ASSIGNPLUS;
    fprintf(stderr, "Unsupported binary operation: %s\n", binaryOp.c_str());
    exit(-1);
}

int getUnaryOpType(std::string unaryOp) {
    if(unaryOp == "+") return POS;
    if(unaryOp == "-") return NEG;
    if(unaryOp == "&") return REF;
    if(unaryOp == "*") return DEREF;
    if(unaryOp == "()") return CAST;
    return POS;
}

ASTNodeType getNodeType(std::string token) {
    if(token == "TranslationUnitDecl") return TRANSLATIONUNIT;
    if(token == "FunctionDecl") return FUNCTIONDECL;
    if(token == "VarDecl") return VARDECL;
    if(token == "BinaryOperator") return BINARYOPERATOR;
    if(token == "Literal") return LITERAL;
    if(token == "CompoundStmt") return COMPOUNDSTMT;
    if(token == "NullStmt") return NULLSTMT;
    if(token == "DeclRefExpr") return DECLREFEXPR;
    if(token == "ReturnStmt") return RETURNSTMT;
    if(token == "ForStmt") return FORSTMT;
    if(token == "ForDelimiter") return FORDELIMITER;
    if(token == "CallExpr") return CALLEXPR;
    if(token == "WhileStmt") return WHILESTMT;
    if(token == "DoStmt") return DOSTMT;
    if(token == "IfStmt") return IFSTMT;
    if(token == "ExplicitCastExpr") return CASTEXPR;
    if(token == "UnaryOperator") return UNARYOPERATOR;
    if(token == "ArrayDecl") return ARRAYDECL;
    if(token == "ArraySubscriptExpr") return ARRAYSUBSCRIPTEXPR;
    if(token == "BreakStmt") return BREAKSTMT;
    if(token == "ContinueStmt") return CONTINUESTMT;
    return UNKNOWN;
}

template<typename T>
std::unique_ptr<T> logError(const char *str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

template<typename T> static std::string getLLVMTypeStr(T* value_or_type) {
    std::string str;
    raw_string_ostream stream(str);
    value_or_type->print(stream);
    return str;
}


Value *logErrorV(const char *str) {
    logError<ExprAST>(str);
    return nullptr;
}

Function *logErrorF(const char *str) {
    logError<ExprAST>(str);
    return nullptr;
}

bool isEqual(char *a, std::string b) {
    std::string bb(b), aa(a);
    return aa == bb;
}

std::string filterString(std::string str){
    std::string buffer(str.size()+1, 0);
    std::string cmd = "python3 -c 'print("+str+", end=\"\")'";
    FILE * f = popen(cmd.c_str(), "r"); assert(f);
    buffer.resize(fread(&buffer[0], 1, buffer.size()-1, f));
    pclose(f);
    return buffer;
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
        // others are all pointers
        return ConstantExpr::getIntToPtr(llvmBuilder->getInt64(0), type);
    }
}



GlobalVariable *createGlob(Type *type, std::string name) {
    llvmModule->getOrInsertGlobal(name, type);
    GlobalVariable *gv = llvmModule->getNamedGlobal(name);
    gv->setConstant(false);
    return gv;
}


void print_node(ast_node_ptr node) {
    printf("Node type: %s(%d), val: %s, n_child: %d type_id: %d\n", node->token, getNodeType(node->token), node->val, node->n_child, node->type_id);
}

bool isValidBinaryOperand(Value *value) {
    return (value->getType()->isFloatingPointTy() || value->getType()->isIntegerTy());
}

void popBlockForControl() {
    BlockForBreak.pop();
    BlockForContinue.pop();
}

void resetBlockForControl() {
    BlockForBreak.empty();
    BlockForContinue.empty();
}

int ptr2raw(int ptr) {
    switch(ptr) {
        case TYPEID_VOID_PTR:
            return TYPEID_VOID;
        case TYPEID_INT_PTR:
            return TYPEID_INT;
        case TYPEID_FLOAT_PTR:
            return TYPEID_FLOAT;
        case TYPEID_DOUBLE_PTR:
            return TYPEID_DOUBLE;
        case TYPEID_CHAR_PTR:
            return TYPEID_CHAR;
        case TYPEID_LONG_PTR:  
            return TYPEID_LONG;
        case TYPEID_SHORT_PTR:
            return TYPEID_SHORT;
        default:
            return ptr;
    }
}

template<typename TO, typename FROM>
std::unique_ptr<TO> static_unique_pointer_cast (std::unique_ptr<FROM>&& old){
    return std::unique_ptr<TO>{static_cast<TO*>(old.release())};
    //conversion: unique_ptr<FROM>->FROM*->TO*->unique_ptr<TO>
}

Value *getBuiltinFunction(std::string callee, std::vector<std::unique_ptr<ExprAST>> &args) {
    if(llvmModule->getFunction(callee)){
        print("Builtin function over written by user!");
        return nullptr;
    }
    Module* mod = llvmModule.get();
    std::vector<Value *> varArgs;
    FunctionType *funcType;
    std::string func_name = callee.substr(strlen("__builtin_"), callee.length());
    print("Real name of " + callee + " is " + func_name);
    auto external_func = llvmModule->getFunction(func_name);
    if(callee == "__builtin_printf"){
        if(args.size() < 1){
            logErrorV("too few arguments to function call");
        }
        auto formatArgs = llvmBuilder->CreateGlobalStringPtr(
            (static_cast<LiteralExprAST *>(args[0].get()))->getValue()
        );
        varArgs.push_back(formatArgs);
        for (int i = 1; i < args.size(); i++) {
            auto arg = args[i]->codegen();
            varArgs.push_back(arg);
        }
        funcType = FunctionType::get(
            Type::getInt32Ty(mod->getContext()), 
            {Type::getInt8PtrTy(mod->getContext())},
            true
        );
    }else if(callee == "__builtin_sprintf"){
        if(args.size() < 2){
            logErrorV("too few arguments to function call");
            exit(1);
        }
        varArgs.push_back(args[0]->codegen());
        auto formatArgs = llvmBuilder->CreateGlobalStringPtr(
            (static_cast<LiteralExprAST *>(args[1].get()))->getValue()
        );
        varArgs.push_back(formatArgs);
        for (int i=2; i < args.size(); i++) {
            auto arg = args[i]->codegen();
            varArgs.push_back(arg);
        }
        funcType = FunctionType::get(
            Type::getInt32Ty(mod->getContext()), 
            {Type::getInt8PtrTy(mod->getContext()), Type::getInt8PtrTy(mod->getContext())},
            true
        );
    }else if(callee == "__builtin_scanf") {
        if(args.size() < 2){
            logErrorV("too few arguments to function call");
        }
        auto formatArgs = llvmBuilder->CreateGlobalStringPtr(
            (static_cast<LiteralExprAST *>(args[0].get()))->getValue()
        );
        varArgs.push_back(formatArgs);
        for (int i = 1; i < args.size(); i++) {
            auto arg = args[i]->codegen();
            varArgs.push_back(arg);
        }
        funcType = FunctionType::get(
            Type::getInt32Ty(mod->getContext()), 
            {Type::getInt8PtrTy(mod->getContext())},
            true
        );
    }
    
    if(external_func){
        return llvmBuilder->CreateCall(external_func, varArgs);
    }
    auto func = Function::Create(funcType, Function::ExternalLinkage, func_name, mod);
    func->setCallingConv(CallingConv::C);
    return llvmBuilder->CreateCall(func, varArgs);
}

std::unique_ptr<ExprAST> generateBackendASTNode(ast_node_ptr root) {
    if (!root) return nullptr;
    ASTNodeType nodeType = getNodeType(std::string(root->token));
    print_node(root);

    char val[MAX_TOKEN_LEN], token[MAX_TOKEN_LEN];
    strcpy(val, root->val);
    strcpy(token, root->token);

    switch (nodeType) {
        case TRANSLATIONUNIT: {
            std::vector<std::unique_ptr<ExprAST>> exprList;
            std::vector<std::unique_ptr<ExprAST>> globalVarList;
            for (int i = 0; i < root->n_child; i++) {
                if(isEqual(root->child[i]->token, "VarDecl")) {
                    auto varDecl = generateBackendASTNode(root->child[i]);
                    std::unique_ptr<VarExprAST> var(static_cast<VarExprAST *>(varDecl.release()));

                    auto globalVarDecl = std::make_unique<GlobalVarExprAST>(std::move(var));
                    globalVarList.push_back(std::move(globalVarDecl));
                }
                else{
                    exprList.push_back(generateBackendASTNode(root->child[i]));
                }
            }
                
            auto trUnit = std::make_unique<TranslationUnitExprAST>(std::move(globalVarList), std::move(exprList));
            return trUnit;
        }

        case FUNCTIONDECL: {
            // 函数ast节点仅保存返回值类型
            std::map<std::string, int> args;
            std::string name(val);    
            
            int param_end = root->n_child;
            std::unique_ptr<ExprAST> compoundStmt;

            if(param_end && isEqual(root->child[param_end-1]->token, "CompoundStmt")) {
                compoundStmt = generateBackendASTNode(root->child[param_end - 1]);
                param_end -= 1;
            } 
            else compoundStmt = nullptr;


            for (int i = 0; i < param_end; i++) {
                auto child = root->child[i];
                if (isEqual(child->token, "ParmVarDecl")) args[std::string(child->val)] = child->type_id;
                print(child->val);
            }

            auto prototype = std::make_unique<PrototypeAST>(name, root->type_id, args);
            prototype->codegen();

            if(!compoundStmt) {
                print("This function def is a prototype");
                return prototype;
            }
            else {
                auto funcExpr = std::make_unique<FunctionDeclAST>(std::move(prototype), std::move(compoundStmt));
                return funcExpr;
            }
        }

        case DECLREFEXPR: {
            std::string varName(val);
            int type = root->type_id;
            auto var = std::make_unique<VarRefExprAST>(varName, type);
            return var;
        }
        
        case VARDECL: {
            std::string varName(val);
            int type = root->type_id;

            auto rhs = 
                root->n_child ? generateBackendASTNode(root->child[0])
                              : nullptr;
            auto var = std::make_unique<VarExprAST>(varName, type, rhs ? std::move(rhs) : nullptr);

            return var;
        }

        case ARRAYDECL: {
            int arraySize = atoi(root->val);
            auto node = generateBackendASTNode(root->child[0]);
            auto var = static_unique_pointer_cast<VarExprAST>(std::move(node));
            auto array = std::make_unique<ArrayExprAST>(ptr2raw(var->getType()), var->getName(), arraySize);
            return array;
        }

        case ARRAYSUBSCRIPTEXPR: {
            auto ref = generateBackendASTNode(root->child[0]);
            auto sub = generateBackendASTNode(root->child[1]);
            auto var = static_unique_pointer_cast<VarRefExprAST>(std::move(ref));
            auto array = std::make_unique<ArraySubExprAST>(std::move(var), std::move(sub), root->type_id);
            return array;
        }

        case LITERAL: {
            std::string literal(val);
            auto literalExpr = std::make_unique<LiteralExprAST>(root->type_id, literal);
            return literalExpr;
        }

        case BINARYOPERATOR: {
            int op_type = getBinaryOpType(std::string(val));
            if( strcmp(root->child[0]->token, "UnaryOperator") == 0 && strcmp(root->child[0]->val, "&") == 0
                && (op_type == ASSIGN || op_type == ASSIGNPLUS)
            ){
                fprintf(stderr, "lvalue can not be a reference\n");
                exit(-1);
            }
            auto LHS = generateBackendASTNode(root->child[0]);
            auto RHS = generateBackendASTNode(root->child[1]);
            if(op_type == ASSIGNPLUS){
                auto left = generateBackendASTNode(root->child[0]);
                auto right = std::make_unique<BinaryExprAST>(getBinaryOpType(std::string(1, val[0])), std::move(LHS), std::move(RHS));
                return std::make_unique<BinaryExprAST>(ASSIGN, std::move(left), std::move(right));
            }else{
                return std::make_unique<BinaryExprAST>(op_type, std::move(LHS), std::move(RHS));
            }
        }

        case CASTEXPR: {
            auto rhs = generateBackendASTNode(root->child[0]);
            auto unaryExpr = std::make_unique<UnaryExprAST>("()", std::move(rhs));
            unaryExpr->setType(root->type_id);
            return unaryExpr;
        }

        case UNARYOPERATOR: {
            auto rhs = generateBackendASTNode(root->child[0]);
            std::string op(val);
            auto unaryExpr = std::make_unique<UnaryExprAST>(op, std::move(rhs));
            unaryExpr->setType(root->type_id);
            return unaryExpr;
        }

        case COMPOUNDSTMT: {
            std::vector<std::unique_ptr<ExprAST>> exprList;
            for(int i=0;i<root->n_child;i++) exprList.push_back(generateBackendASTNode(root->child[i]));
            auto compound = std::make_unique<CompoundStmtExprAST>(std::move(exprList));
            return compound;
        }

        case NULLSTMT: {
            return std::make_unique<NullStmtAST>();
        }

        case FORSTMT: {
            auto start = generateBackendASTNode(root->child[0]);
            auto end = generateBackendASTNode(root->child[1]);
            auto step = generateBackendASTNode(root->child[2]);
            auto body = generateBackendASTNode(root->child[4]);
            auto forStmt = std::make_unique<ForExprAST>(std::move(start), std::move(end), std::move(step), std::move(body));
            return forStmt;
        }

        case BREAKSTMT: {
            return std::make_unique<BreakExprAST>();
        }

        case CONTINUESTMT: {
            return std::make_unique<ContinueExprAST>();
        }

        case RETURNSTMT: {
            auto body = generateBackendASTNode(root->child[0]);
            auto returnStmt = std::make_unique<ReturnStmtExprAST>(std::move(body));
            return returnStmt;
        }

        case CALLEXPR: {
            auto name = std::string(root->child[0]->val);
            std::vector<std::unique_ptr<ExprAST>> args;
            for(int i=1;i<root->n_child;i++) args.push_back(
                generateBackendASTNode(root->child[i])
            );
            auto call = std::make_unique<CallExprAST>(name, std::move(args));
            return call;
        }

        case IFSTMT: {
            auto cond = generateBackendASTNode(root->child[0]);
            auto then_case = generateBackendASTNode(root->child[1]);
            auto else_case = (root->n_child >= 3) ? generateBackendASTNode(root->child[2]) : nullptr;
            auto ifExpr = std::make_unique<IfExprAST>(std::move(cond), std::move(then_case), std::move(else_case));
            return ifExpr;
        }

        case WHILESTMT: {
            auto cond = generateBackendASTNode(root->child[0]);
            auto body = generateBackendASTNode(root->child[1]);
            auto whileExpr = std::make_unique<WhileExprAST>(std::move(cond), std::move(body));
            return whileExpr;
        }

        case DOSTMT: {
            auto cond = generateBackendASTNode(root->child[1]);
            auto body = generateBackendASTNode(root->child[0]);
            auto doExpr = std::make_unique<DoExprAST>(std::move(cond), std::move(body));
            return doExpr;
        }

        default:
            return nullptr;
    }
}


std::unique_ptr<ExprAST> generateBackendAST(struct lib_frontend_ret frontend_ret) {
    ast_node_ptr root = frontend_ret.root;
    std::unique_ptr<ExprAST> newRoot = generateBackendASTNode(root);
    return newRoot;
}


static void initializeModule() {
    llvmContext = std::make_unique<LLVMContext>();
    llvmModule = std::make_unique<Module>("JIT", *llvmContext);
    llvmBuilder = std::make_unique<IRBuilder<>>(*llvmContext);
    llvmFPM = std::make_unique<legacy::FunctionPassManager>(llvmModule.get());
    llvmFPM->add(createInstructionCombiningPass());
    llvmFPM->add(createReassociatePass());
    llvmFPM->add(createGVNPass());
    llvmFPM->add(createCFGSimplificationPass());
    llvmFPM->add(llvm::createPromoteMemoryToRegisterPass());
    llvmFPM->add(llvm::createInstructionCombiningPass());
    llvmFPM->add(llvm::createReassociatePass());
    llvmFPM->doInitialization();
}

static void initializeBuiltinFunction() {
    // 初始化内置函数
    // 内置函数的定义在builtin.h中

    // for (auto &func : builtin_function) {
    //     auto func_ptr = (void (*)(void))func.second;
    //     auto func_type = FunctionType::get(Type::getVoidTy(*llvmContext), {}, false);
    //     auto func_value = Function::Create(func_type, Function::ExternalLinkage, func.first, *llvmModule);
    //     func_value->setCallingConv(CallingConv::C);
    //     func_value->addFnAttr(Attribute::NoUnwind);
    //     func_value->addFnAttr(Attribute::UWTable);
    //     func_value->setDoesNotThrow();
    //     func_value->setAlignment(4);
    //     auto func_ptr_value = ConstantExpr::getIntToPtr(ConstantInt::get(Type::getInt64Ty(*llvmContext), (uint64_t)func_ptr), func_type->getPointerTo());
    //     func_value->setLinkage(GlobalValue::ExternalLinkage);
    //     func_value->setExternallyInitialized(true);
    //     func_value->setInitializer(func_ptr_value);
    // }
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
            return nullptr;
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

Function *getFunction(std::string name) {
    if (auto *F = llvmModule->getFunction(name)) return F;
    auto F = functionProtos.find(name);
    if(F != functionProtos.end()) return F->second->codegen(); 
    return nullptr;
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
        return V;
    }
    auto key = llvmModule->getGlobalVariable(name);
    if (key) {
        isGlobal = 1;
        return key;
    }
    return nullptr;
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAllocaWithTypeSize(StringRef VarName, Type* type, Value* size = nullptr, BasicBlock* Scope = llvmBuilder->GetInsertBlock(), BasicBlock::iterator Point =llvmBuilder->GetInsertPoint()) {
    IRBuilder<> allocator(Scope, Point);
    return allocator.CreateAlloca(type, size, VarName);
}

static AllocaInst *CreateEntryBlockAllocaWithTypeSize(StringRef VarName, int type_id, Value* size = nullptr, BasicBlock* Scope = llvmBuilder->GetInsertBlock(), BasicBlock::iterator Point =llvmBuilder->GetInsertPoint()) {
    IRBuilder<> allocator(Scope, Point);
    return allocator.CreateAlloca(getVarType(type_id), size, VarName);
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

Value *TranslationUnitExprAST::codegen(bool wantPtr) {
    print("TranslationUnit");
    int globalVar_len = globalVarList.size();
    int expr_len = exprList.size();
    print("Global vars: "+ std::to_string(globalVar_len) + "\nExprs: " + std::to_string(expr_len));
    print("Global(s) Initializing Pass......");
    initializing = true;
    for (int i=0; i < globalVar_len; i++) {
        globalVarList[i]->codegen();
    }
    initializing = false;
    print("Global(s) Initializing End......");

    for (int i=0; i < expr_len; i++) {
        exprList[i]->codegen();
    }
    return nullptr;
}

Value *VarExprAST::codegen(bool wantPtr) {
    print("VarExpr, typeid: " + std::to_string(type));
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    auto varType = getVarType(this->getType());

    Value *initVal;
    if(init) {
        initVal = init->codegen();
    } else {
        initVal = getInitVal(varType);
    }
    auto castedVal = createCast(initVal, varType);
    if(!castedVal){
        logErrorV((std::string("Unsupported initializatin from "+ getLLVMTypeStr(initVal->getType()) +" to " + getLLVMTypeStr(varType)).c_str()));
    }
    AllocaInst *alloca = CreateEntryBlockAllocaWithTypeSize(name, varType);
    llvmBuilder->CreateStore(castedVal, alloca);
    NamedValues.top()[name] = alloca;
    return castedVal;
}

Value *ArrayExprAST::codegen(bool wantPtr) {
    auto val = getVariable(name);
    if (val) {
        std::string errorMsg = "Variable " + name + " is already defined";
        return logErrorV(errorMsg.c_str());
    }

    auto varType = getVarType(this->getType());
    auto arrayType = ArrayType::get(varType, size);
    auto arrayPtr = llvmBuilder->CreateAlloca(arrayType, nullptr, name);
    NamedValues.top()[name] = arrayPtr;
    // auto defaultValue = getInitVal(varType);
    // for (int i=0; i < size; i++) {
    //     auto index = llvmBuilder->getInt32(i);
    //     auto arrayElem = llvmBuilder->CreateGEP(arrayPtr, index);
    //     llvmBuilder->CreateStore(defaultValue, arrayElem);
    // }
    return arrayPtr;
}

Value *VarRefExprAST::codegen(bool wantPtr) {
    print("VarRefExpr");
    int isGlobal = 0;
    auto V = getVariable(name, isGlobal);
    if (!V) {
        return nullptr;
    }
    if(wantPtr) {
        return V;
    }
    if(isGlobal) {
        std::cout << "Find global var: " << name << std::endl;
        GlobalVariable *gV = static_cast<GlobalVariable *>(V);
        if(gV->isConstant() || initializing)
            return gV->getInitializer();
        else
            return llvmBuilder->CreateLoad(V->getType()->getPointerElementType(), V);
    }
    if(V->getType()->getPointerElementType()->isArrayTy()){
        return llvmBuilder->CreateGEP(V->getType()->getPointerElementType(), V, {llvmBuilder->getInt64(0), llvmBuilder->getInt64(0)});
    }else{
        return llvmBuilder->CreateLoad(getVarType(this->type), V, name.c_str());
    }
}

Value *GlobalVarExprAST::codegen(bool wantPtr) {
    auto varType = getVarType(this->init->getType());
    print("GlobalVarExpr, Name: " + init->name + " Type: " + getLLVMTypeStr(varType));

    Value *initVal;

    if(init->init) {
        initVal = init->init->codegen();
        initVal = createCast(initVal, varType);
        if(!initVal) 
            return nullptr;
    }else{
        std::cout << "No init val, using default" << std::endl;
        initVal = getInitVal(varType);
    }

    auto globalVar = createGlob(varType, init->name);
    globalVar->setInitializer(dyn_cast<Constant>(initVal));
    return initVal;
}

Value *ArraySubExprAST::codegen(bool wantPtr) {
    static Value* zero = llvmBuilder->getInt64(0);
    auto index = sub->codegen();
    if (!index) return nullptr;
    auto varPtr = var->codegen(true);
    if (!varPtr) return nullptr;
    Value* elementPtr;
    auto castIndex = createCast(index, Type::getInt64Ty(*llvmContext));
    Type * elementType = getVarType(type);
    if(varPtr->getType()->getPointerElementType()->isPointerTy()){
        // unsafe way
        Value * arrayPtr = llvmBuilder->CreateLoad(elementType->getPointerTo(), varPtr);
        elementPtr = llvmBuilder->CreateGEP(
            elementType,
            arrayPtr, 
            castIndex
        );
    }else{
        // safeway
        elementPtr = llvmBuilder->CreateGEP(
            ArrayType::get(elementType, varPtr->getType()->getPointerElementType()->getArrayNumElements()), 
            varPtr, 
            {zero, castIndex}
        );
    }
    if(wantPtr){
        return elementPtr;
    }
    return llvmBuilder->CreateLoad(elementType, elementPtr);
};


Value *LiteralExprAST::codegen(bool wantPtr) {
    print("LiteralExpr, typeid: " + std::to_string(this->getType()));
    switch(this->getType()) {
        case TYPEID_INT: {
            auto intCode = ConstantInt::get(*llvmContext, APInt(32, atoi(value.c_str())));
            return intCode;
        }
        case TYPEID_DOUBLE: {
            auto doubleCode = ConstantFP::get(*llvmContext, APFloat(atof(value.c_str())));
            return doubleCode;
        }
        case TYPEID_STR: {
            std::cout << "Creating string literal: " << value << std::endl;

            auto str = this->value;
            auto charType = llvm::IntegerType::get(*llvmContext, 8);

            //1. Initialize chars vector
            std::vector<llvm::Constant *> chars(str.length());
            for(unsigned int i = 0; i < str.size(); i++) {
            chars[i] = llvm::ConstantInt::get(charType, str[i]);
            }

            //1b. add a zero terminator too
            chars.push_back(llvm::ConstantInt::get(charType, 0));


            //2. Initialize the string from the characters
            auto stringType = llvm::ArrayType::get(charType, chars.size());

            //3. Create the declaration statement
            auto globalDeclaration = (llvm::GlobalVariable*) llvmModule->getOrInsertGlobal(".str", stringType);
            globalDeclaration->setInitializer(llvm::ConstantArray::get(stringType, chars));
            globalDeclaration->setConstant(true);
            globalDeclaration->setLinkage(llvm::GlobalValue::LinkageTypes::PrivateLinkage);
            globalDeclaration->setUnnamedAddr (llvm::GlobalValue::UnnamedAddr::Global);

            //4. Return a cast to an i8*
            return llvm::ConstantExpr::getBitCast(globalDeclaration, charType->getPointerTo());
       }

        default:
            return logErrorV("Invalid type!");
    }
}

Value *UnaryExprAST::codegen(bool wantPtr) {
    int opType = getUnaryOpType(op);
    print("Unary op: " + op);
    auto varType = getVarType(type);
    Value *right;    

    switch (opType) {
        case POS: {
            Value *right = rhs->codegen();
            return right;
        }
        case NEG: {
            if (right->getType()->isFloatingPointTy())
                return llvmBuilder->CreateFNeg(right);
            else 
                return llvmBuilder->CreateNeg(right);
        }
        
        case REF: {
            Value * ptr = rhs->codegen(true);          
            if (!ptr) return logErrorV("Referring to a undefined variable");
            return ptr;
        }

        case DEREF: {
            Value *right = rhs->codegen(wantPtr);
            if (!right) return logErrorV("Unable to do dereferring");
            if(wantPtr && right->getType()->getPointerElementType() == varType){
                /* 
                    this indicates right is already a ptr to char, int, double, etc...
                    use to handle the special case that *(ptr + offset) as left part
                    of an assign statement, in this case, we know that since
                    ptr + offset don't actually refer to a variable in context, it is
                    meaningless to create load from it.
                */
                return right;
            }
            auto V = llvmBuilder->CreateLoad(wantPtr ? varType->getPointerTo() : varType, right);
            if (!V) return logErrorV("Unable to do dereferring");
            return V;
        }

        case CAST: {
            Value * right = rhs->codegen();
            Value * casted = createCast(right, getVarType(type));
            if(!casted) return logErrorV("Unsupported Cast");
            return casted;
        }

        default: {
            auto errorMsg = "Invalid unary op" + op;
            return logErrorV(errorMsg.c_str());
        }
    }
}

Value *BinaryExprAST::codegen(bool wantPtr) {

    if (op_type == ASSIGN) {
        std::string name;
        Value * variable = lhs->codegen(true);
        Value * val = rhs->codegen();
        if (!variable || !val) {
            return logErrorV("Invaild Reference");
        }
        auto castedVal = createCast(val, variable->getType()->getPointerElementType());
        if (!castedVal){
            return logErrorV((std::string("Unsupported assign from "+ getLLVMTypeStr(val->getType()) +" to " + getLLVMTypeStr(variable->getType()->getPointerElementType())).c_str()));
        }
        val = castedVal;
        llvmBuilder->CreateStore(val, variable);
        return val;
    }


    Value *left = lhs->codegen();
    std::cout << "Value on lhs: " + getLLVMTypeStr(left) << std::endl;
    Value *right = rhs->codegen();
    std::cout << "Value on rhs: " + getLLVMTypeStr(right) << std::endl;


    if (!left || !right) {
        return logErrorV("lhs / rhs is not valid");
    }

    auto leftType = left->getType();
    auto rightType = right->getType();

    if (leftType->isPointerTy() && rightType->isPointerTy()) {
        return logErrorV("Unsupported operation between pointers");
    }

    if(leftType->isPointerTy() || rightType->isPointerTy()) {
        print("Pointer operation");
        auto newLeft = leftType->isPointerTy() ? left : right;
        auto newRight = leftType->isPointerTy() ? right : left;
        left = newLeft;
        right = newRight;

        switch(op_type) {
            case ADD:
                return llvmBuilder->CreateGEP(leftType->getPointerElementType(), left, right);
            case SUB:
                right = llvmBuilder->CreateNeg(right);
                return llvmBuilder->CreateGEP(leftType->getPointerElementType(), left, right);
            default:
                return logErrorV("Unsupported binary operation for pointer");
        }
    }


    if(leftType->isFloatingPointTy() || rightType->isFloatingPointTy()) {
        // always cast int to FP
        auto FPleft = createCast(left, Type::getDoubleTy(*llvmContext));
        auto FPright = createCast(right, Type::getDoubleTy(*llvmContext));
        if(!FPleft || !FPright){
            return logErrorV(std::string("Unsupported operand between "+ getLLVMTypeStr(left->getType()) +" and " + getLLVMTypeStr(right->getType())).c_str());
        }else{
            left = FPleft;
            right = FPright;
        }
        switch(op_type) {
            case ADD:
                return llvmBuilder->CreateFAdd(left, right, "fadd");
            case SUB:
                return llvmBuilder->CreateFSub(left, right, "fsub");
            case MUL:
                return llvmBuilder->CreateFMul(left, right, "fmul");
            case DIV:
                return llvmBuilder->CreateFDiv(left, right, "fdiv");
            case LT:
                return llvmBuilder->CreateFCmpOLT(left, right, "flt");
            case GT:
                return llvmBuilder->CreateFCmpOGT(left, right, "fgt");
            case EQ:
                return llvmBuilder->CreateFCmpOEQ(left, right, "feq");
            case NE:
                return llvmBuilder->CreateFCmpUNE(left, right, "fne");
            case LE:
                return llvmBuilder->CreateFCmpOLE(left, right, "fle");
            case GE:
                return llvmBuilder->CreateFCmpOGE(left, right, "fge");
            default:
                return logErrorV("Invalid binary operator");
        }
    }else{
        // Otherwise, always try to convert right to left!
        right = createCast(right, left->getType());
        if(!right){
            return logErrorV((std::string("Unsupported operand between "+ getLLVMTypeStr(left->getType()) +" : " + getLLVMTypeStr(right->getType())).c_str()));
        }
        switch(op_type) {
            case ADD:
                return llvmBuilder->CreateAdd(left, right, "iadd");
            case SUB:
                return llvmBuilder->CreateSub(left, right, "isub");
            case MUL:
                return llvmBuilder->CreateMul(left, right, "imul");
            case DIV:
                return llvmBuilder->CreateSDiv(left, right, "idiv");
            case LT:
                return llvmBuilder->CreateICmpSLT(left, right, "ilt");
            case GT:
                return llvmBuilder->CreateICmpSGT(left, right, "igt");
            case LE:
                return llvmBuilder->CreateICmpSLE(left, right, "ile");
            case GE:
                return llvmBuilder->CreateICmpSGE(left, right, "ige");
            case NE:
                return llvmBuilder->CreateICmpNE(left, right, "ine");
            case EQ:
                return llvmBuilder->CreateICmpEQ(left, right, "ieq");
            default:
                return logErrorV("Invalid binary operator");
        }
    }
}

Function *PrototypeAST::codegen(bool wantPtr) {
    std::vector<Type *> llvmArgs;
    for (auto arg: args) 
        llvmArgs.push_back(getVarType(arg.second));
    
    FunctionType *functionType = FunctionType::get(getVarType(retVal), llvmArgs, false);

    Function *F = Function::Create(functionType, Function::ExternalLinkage, name, llvmModule.get());

    auto it = this->args.begin();
    for (auto &arg: F->args()) {
        print(it->first);
        arg.setName(it->first);
        it++;
    }

    return F;
}

Function *FunctionDeclAST::codegen(bool wantPtr) {
    std::cout << "FunctionDeclAST: " << prototype->getName() << std::endl;
    retPtr = retBlock = nullptr;
    auto &p = *prototype;
    functionProtos[p.getName()] = std::move(prototype);
    Function *currFunction = getFunction(p.getName());
    if (!currFunction) {
        return logErrorF(("Unknown function referenced" + p.getName()).c_str());
    }
    BasicBlock* entryBlock = BasicBlock::Create(*llvmContext, "entry", currFunction);
    llvmBuilder->SetInsertPoint(entryBlock);
    // if not void, ret value should be a variable!
    Type *retType = getVarType(p.retVal);
    std::cout << "return type: " << getLLVMTypeStr(retType) << std::endl;
    if(!retType->isVoidTy()){
        retPtr = CreateEntryBlockAllocaWithTypeSize("ret_val", retType);
        llvmBuilder->CreateStore(getInitVal(retType), retPtr);
    }
    resetBlockForControl();
    NamedValues.empty();
    std::map<std::string, AllocaInst *> local_vals;
    NamedValues.push(local_vals);
    for (auto &arg : currFunction->args()) {
        AllocaInst *alloca = CreateEntryBlockAllocaWithTypeSize(arg.getName(), arg.getType());
        llvmBuilder->CreateStore(&arg, alloca);
        NamedValues.top()[std::string(arg.getName())] = alloca;
    }

    llvmBuilder->SetInsertPoint(entryBlock);
    retBlock = BasicBlock::Create(*llvmContext, "ret", currFunction);

    llvmBuilder->SetInsertPoint(retBlock);
    if(retPtr){
        llvmBuilder->CreateRet(llvmBuilder->CreateLoad(retType, retPtr, "final_ret"));
    }else{
        llvmBuilder->CreateRetVoid();
    }
    
    // Create Function Body
    llvmBuilder->SetInsertPoint(entryBlock);
    auto compoundStmt = static_cast<CompoundStmtExprAST *>(body.get());
    auto val = compoundStmt->codegen();
    if(!val) {
        currFunction->eraseFromParent();
        return logErrorF("Error reading body");
    }
    print("Build Successfully");
    // Create default terminator if not(add a default return for all empty labels)
    auto iter = currFunction->getBasicBlockList().begin();
    auto end = currFunction->getBasicBlockList().end();
    print("Check If Terminated");
    for(;iter != end ;++iter){
        std::cout << iter->getName().data() << std::endl;
        if(!iter->getTerminator()){
            llvmBuilder->SetInsertPoint(dyn_cast<BasicBlock>(iter));
            llvmBuilder->CreateBr(retBlock);
        }
    }
    verifyFunction(*currFunction, &errs());
    resetBlockForControl();
    llvmFPM->run(*currFunction);
    return currFunction;
}

Value *CompoundStmtExprAST::codegen(bool wantPtr) {
    print("New scope declared!");
    std::map<std::string, AllocaInst *> Scope = NamedValues.top();
    NamedValues.push(Scope);
    Value *retVal = llvmBuilder->getTrue();
    for (auto &expr: exprList){
        retVal = expr->codegen();
        if(!retVal) return nullptr;
    }
    print("Back to previous scope!");
    NamedValues.pop();
    return retVal;
}

Value *ReturnStmtExprAST::codegen(bool wantPtr) {
    print("Find return stmt");
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    if(!retPtr){
        return llvmBuilder->CreateBr(retBlock);
    }

    if (!body) return logErrorV("No ret val");
    Value *retVal = body->codegen();
    retVal = createCast(retVal, currFunction->getReturnType());
    if (!retVal) return logErrorV("No ret val");
    std::cout << "Get return value: " << getLLVMTypeStr(retVal) << std::endl;
    llvmBuilder->CreateStore(retVal, retPtr);
    
    llvmBuilder->CreateBr(retBlock);
    return retVal;
}

Value *ForExprAST::codegen(bool wantPtr) {
    /*
     * phi node: i {label: value}
     * entry, loop, after_loop
     */

    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    Value *startVal = start->codegen();
    if (!startVal) return nullptr;
    auto valType = startVal->getType();

    auto alloca = getVariable(varName);

    if (!alloca) {
        return logErrorV("Unknown variable referenced in for loop");
    }
    

    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "for_loop", currFunction);
    BasicBlock *bodyBlock = BasicBlock::Create(*llvmContext, "for_body", currFunction);
    BasicBlock *stepBlock = BasicBlock::Create(*llvmContext, "for_step", currFunction);
    BasicBlock *afterBlock = BasicBlock::Create(*llvmContext, "after_for_loop", currFunction);

    BlockForBreak.push(afterBlock);
    BlockForContinue.push(stepBlock);

    llvmBuilder->CreateBr(loopBlock);
    llvmBuilder->SetInsertPoint(loopBlock);

    Value *endVal = end->codegen();
    if (!endVal) return nullptr;
    if(endVal->getType()->isFloatingPointTy())
        endVal = llvmBuilder->CreateFCmpONE(endVal, getInitVal(endVal->getType()), "loop_end");

    else if(endVal->getType()->isIntegerTy(INTEGER_BITWIDTH))
        endVal = llvmBuilder->CreateICmpNE(endVal, getInitVal(endVal->getType()), "loop_end");

    llvmBuilder->CreateCondBr(endVal, bodyBlock, afterBlock);
    
    llvmBuilder->SetInsertPoint(bodyBlock);
    if (!body->codegen()) return nullptr;
    
    llvmBuilder->CreateBr(stepBlock);
    llvmBuilder->SetInsertPoint(stepBlock);
    Value *stepVar = step->codegen();
    if (!stepVar) return nullptr;

    std::cout   << "Type in for: "
                << getLLVMTypeStr(startVal) 
                << " " 
                << getLLVMTypeStr(endVal)
                << " "
                << getLLVMTypeStr(stepVar) 
                << std::endl;

    Value *nextVar = llvmBuilder->CreateLoad(valType, alloca, varName.c_str());
    llvmBuilder->CreateBr(loopBlock);
    llvmBuilder->SetInsertPoint(afterBlock);
    popBlockForControl();
    return Constant::getNullValue(valType);
}

Value *CallExprAST::codegen(bool wantPtr) {
    std::cout << "Call to: " + callee << std::endl;
    // 如果是builtin function
    if(callee.find("__builtin_") == 0){
        auto builtin_ret = getBuiltinFunction(callee, args);
        if(builtin_ret) return builtin_ret;
    }

    Function *calleeFunction = llvmModule->getFunction(callee);
    if (!calleeFunction) return logErrorV("Unknown function");
    if (calleeFunction->arg_size() != args.size()) logErrorV("Incorrect args");

    std::vector<Value *> argsValue;
    for (auto &arg: args) {
        Value *argValue = arg->codegen();
        if (!argValue) return nullptr;
        std::cout << "Arg value: " + getLLVMTypeStr(argValue) << std::endl;
        argsValue.push_back(argValue);
    }

    return llvmBuilder->CreateCall(calleeFunction, argsValue);
}

Value *IfExprAST::codegen(bool wantPtr) {
    Value *condValue = cond->codegen();
    if (!condValue) return nullptr;
    auto condType = condValue->getType();

    std::cout << "Cond type: " << getLLVMTypeStr(condType) << std::endl;
    Value * condVal;
    if(condType->isFloatingPointTy())
        condVal = llvmBuilder->CreateFCmpONE(condValue, getInitVal(condType), "if_comp");
    else
        condVal = llvmBuilder->CreateICmpNE(condValue, getInitVal(condType), "if_comp");

    std::cout << "type of condval: " << getLLVMTypeStr(condVal) << std::endl;
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    BasicBlock *endifBlock = BasicBlock::Create(*llvmContext, "endif", currFunction, retBlock);
    BasicBlock *thenBlock = BasicBlock::Create(*llvmContext, "then_case", currFunction, endifBlock);
    if(else_case){
        BasicBlock *elseBlock = BasicBlock::Create(*llvmContext, "else_case", currFunction, endifBlock);
        llvmBuilder->CreateCondBr(condVal, thenBlock, elseBlock);

        llvmBuilder->SetInsertPoint(thenBlock);
        Value *thenValue = then_case->codegen();
        if (!thenValue) return nullptr;
        if(!thenBlock->getTerminator()){
            llvmBuilder->CreateBr(endifBlock);
        }

        llvmBuilder->SetInsertPoint(elseBlock);
        Value *elseValue = else_case->codegen();
        if (!elseValue) return nullptr;
        if(!elseBlock->getTerminator()){
            llvmBuilder->CreateBr(endifBlock);
        }

        llvmBuilder->SetInsertPoint(endifBlock);
        return condValue;
    }else{
        llvmBuilder->CreateCondBr(condVal, thenBlock, endifBlock);
        llvmBuilder->SetInsertPoint(thenBlock);

        Value *thenValue = then_case->codegen();
        if (!thenValue) return nullptr;
        std::cout << "Return value of then value: " << getLLVMTypeStr(thenValue) << std::endl;
        if(!thenBlock->getTerminator()){
            llvmBuilder->CreateBr(endifBlock);
        }
        llvmBuilder->SetInsertPoint(endifBlock);
        return condValue;
    }
}

Value *WhileExprAST::codegen(bool wantPtr) {
    print("Generate for while expr");

    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    BasicBlock *entryBlock = BasicBlock::Create(*llvmContext, "entry", currFunction);
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "while_loop_body", currFunction);
    BasicBlock *endBlock = BasicBlock::Create(*llvmContext, "while_loop_end", currFunction);

    BlockForBreak.push(endBlock);
    BlockForContinue.push(loopBlock);

    llvmBuilder->CreateBr(entryBlock);
    llvmBuilder->SetInsertPoint(entryBlock);
    Value *endVal;

    Value *condVal = cond->codegen();
    if (!condVal) return nullptr;
    auto condType = condVal->getType();

    std::cout << "Type in while: " 
            << getLLVMTypeStr(condVal) 
            << " "
            << getLLVMTypeStr(condVal->getType())
            << std::endl;


    if(condType->isFloatingPointTy())
        endVal = llvmBuilder->CreateFCmpONE(condVal, getInitVal(condVal->getType()), "while_comp");
    else
        endVal = llvmBuilder->CreateICmpNE(condVal, getInitVal(condVal->getType()), "while_comp");

    llvmBuilder->CreateCondBr(endVal, loopBlock, endBlock);
    llvmBuilder->SetInsertPoint(loopBlock);
    if(!body->codegen()) return nullptr;

    llvmBuilder->CreateBr(entryBlock);
    llvmBuilder->SetInsertPoint(endBlock);

    popBlockForControl();
    return Constant::getNullValue(condType);
}

Value *DoExprAST::codegen(bool wantPtr) {
    print("Generate for while expr");
    

    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    // BasicBlock *entryBlock = BasicBlock::Create(*llvmContext, "entry", currFunction);
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "do_while_loop_body", currFunction);
    BasicBlock *endBlock = BasicBlock::Create(*llvmContext, "do_while_loop_end", currFunction);

    BlockForBreak.push(endBlock);
    BlockForContinue.push(loopBlock);

    llvmBuilder->CreateBr(loopBlock);
    llvmBuilder->SetInsertPoint(loopBlock);

    if(!body->codegen()) return nullptr;

    Value *endVal;

    Value *condVal = cond->codegen();
    if (!condVal) return nullptr;
    auto condType = condVal->getType();

    std::cout << "Type in do_while: " 
            << getLLVMTypeStr(condVal) 
            << " "
            << getLLVMTypeStr(condVal->getType())
            << std::endl;


    if(condVal->getType()->isFloatingPointTy())
        endVal = llvmBuilder->CreateFCmpONE(condVal, getInitVal(condVal->getType()), "while_comp");
    else
        endVal = llvmBuilder->CreateICmpNE(condVal, getInitVal(condVal->getType()), "while_comp");

    llvmBuilder->CreateCondBr(endVal, loopBlock, endBlock);
    llvmBuilder->SetInsertPoint(endBlock);

    popBlockForControl();
    return Constant::getNullValue(Type::getDoubleTy(*llvmContext));
}

Value *BreakExprAST::codegen(bool wantPtr) {
    print("Generate for break expr");
    if(BlockForBreak.empty()){
        return logErrorV("Break statement not in loop");
    }
    return llvmBuilder->CreateBr(BlockForBreak.top());
}

Value *ContinueExprAST::codegen(bool wantPtr) {
    print("Generate for continue expr");
    if(BlockForContinue.empty()){
        return logErrorV("Continue statement not in loop");
    }
    return llvmBuilder->CreateBr(BlockForContinue.top());
}

Value * NullStmtAST::codegen(bool wantPtr){
    return llvmBuilder->getTrue();
}

static std::unique_ptr<ExprAST> ast;
void run_lib_backend(int argc, const char **argv) {
    auto frontend_ret = frontend_entry(argc, argv);
    ast = generateBackendAST(frontend_ret);
    print("Finish generating AST for backend");
    ast->codegen();
}

int compile() {
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
    std::cout << "Using output filename: " << filename << std::endl; 
    std::error_code EC;
    raw_fd_ostream dest(filename, EC, sys::fs::OF_None);

    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;

    if (theTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        errs() << "TheTargetMachine can't emit a file of this type";
        return 1;
    }

    pass.run(*llvmModule);
    dest.flush();
    return 0;
}

void save() {
    std::string IRText;
    raw_string_ostream OS(IRText);
    OS << *llvmModule;
    OS.flush();
    #ifdef IRONLY
        std::cout << IRText;
    #else
        std::string fileName("output.ll");
        std::ofstream outFile;
        outFile.open(fileName);
        outFile << IRText;
        std::cout << "IR code saved to " + fileName << std::endl;
        outFile.close();
    #endif
}

int main(int argc, const char **argv) {
    #ifdef IRONLY
        int stdout_fd = dup(STDOUT_FILENO);
        close(STDOUT_FILENO);
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
    #endif
    
    initializeModule();

    run_lib_backend(argc, argv);
    // llvmModule->print(errs(), nullptr);

    #ifndef IRONLY
        save();
        int status_code = compile();
        std::cout << "Compile end with status code: " << status_code << std::endl << std::endl;
    #else
        close(devnull);
    	dup2(stdout_fd, STDOUT_FILENO);
        save();
    #endif
    return 0;
}
