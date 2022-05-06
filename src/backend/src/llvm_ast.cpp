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
static std::map<std::string, std::unique_ptr<PrototypeAST>> functionProtos;

inline void print(std::string a) { std::cout << a << std::endl; }

int getBinaryOpType(std::string binaryOp) {
    if(binaryOp == "+") return ADD;
    if(binaryOp == "-") return SUB;
    if(binaryOp == "*") return MUL;
    if(binaryOp == "/") return DIV;
    if(binaryOp == "<") return LT;
    if(binaryOp == ">") return GT;
    if(binaryOp == "<=") return LE;
    if(binaryOp == ">=") return GE;    
    if(binaryOp == "==") return EQ;
    if(binaryOp == "!=") return NE;
    if(binaryOp == "=") return ASSIGN;
    return ADD;
}

int getUnaryOpType(std::string unaryOp) {
    if(unaryOp == "+") return POS;
    if(unaryOp == "-") return NEG;
    if(unaryOp == "&") return REF;
    if(unaryOp == "*") return DEREF;
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
            // auto var = static_cast<VarExprAST *>(node.get());
            auto var = static_unique_pointer_cast<VarExprAST>(std::move(node));
            auto array = std::make_unique<ArrayExprAST>(ptr2raw(var->getType()), var->getName(), arraySize);
            return array;
        }

        case ARRAYSUBSCRIPTEXPR: {
            auto ref = generateBackendASTNode(root->child[0]);
            auto sub = generateBackendASTNode(root->child[1]);
            auto var = static_unique_pointer_cast<VarRefExprAST>(std::move(ref));
            // auto Var = static_cast<VarRefExprAST *>(ref.get());
            // auto var = std::unique_ptr<VarRefExprAST>(Var);
            var->setType(ptr2raw(var->getType()));
            std::cout << "Subscription: " << var->getName() << " " << var->getType() << std::endl;
            auto array = std::make_unique<ArraySubExprAST>(std::move(var), std::move(sub));
            return array;
        }

        case LITERAL: {
            std::string literal(val);
            auto literalExpr = std::make_unique<LiteralExprAST>(root->type_id, literal);
            return literalExpr;
        }

        case BINARYOPERATOR: {
            auto LHS = generateBackendASTNode(root->child[0]);
            auto RHS = generateBackendASTNode(root->child[1]);

            std::string op(val);
            auto binaryExpr = std::make_unique<BinaryExprAST>(op, std::move(LHS), std::move(RHS));
            if (isEqual(root->child[0]->token, "UnaryOperator"))
                binaryExpr->type = UNARYOP;
            else if (isEqual(root->child[0]->token, "ArraySubscriptExpr"))
                binaryExpr->type = ARRAYSUB;
            else
                binaryExpr->type = root->type_id;
            return binaryExpr;
        }

        case UNARYOPERATOR: {
            auto rhs = generateBackendASTNode(root->child[0]);
            std::string op(val);
            auto unaryExpr = std::make_unique<UnaryExprAST>(op, std::move(rhs));
            unaryExpr->type = root->type_id;
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


Type *getVarType(LLVMContext &C, int type_id) {
    switch(type_id) {
        case TYPEID_VOID:
            return Type::getVoidTy(C);
        case TYPEID_CHAR:
            return Type::getInt8Ty(C);
        case TYPEID_SHORT:
            return Type::getInt16Ty(C);
        case TYPEID_INT:
            if(INTEGER_BITWIDTH == 32) 
                return Type::getInt32Ty(C);
            else 
                return Type::getInt64Ty(C);
        case TYPEID_LONG:
            return Type::getInt64Ty(C);
        case TYPEID_FLOAT:
            return Type::getFloatTy(C);
        case TYPEID_DOUBLE:
            return Type::getDoubleTy(C);
        case TYPEID_STR:
            return Type::getInt8PtrTy(C);
        case TYPEID_VOID_PTR:
            return nullptr;
        case TYPEID_CHAR_PTR:
            return Type::getInt8PtrTy(C);
        case TYPEID_SHORT_PTR:
            return Type::getInt16PtrTy(C);        
        case TYPEID_INT_PTR:
            if(INTEGER_BITWIDTH == 32) 
                return Type::getInt32PtrTy(C);
            else 
                return Type::getInt64PtrTy(C);
        case TYPEID_LONG_PTR:
            return Type::getInt64PtrTy(C);        
        case TYPEID_FLOAT_PTR:
            return Type::getFloatPtrTy(C);        
        case TYPEID_DOUBLE_PTR:
            return Type::getDoublePtrTy(C);        
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

Value *getVariable(std::string name, int &isGlobal) {
    auto V = NamedValues.top()[name];
    if(V) {
        std::cout << "Find local variable\n";
        return V;
    }

    auto key = llvmModule->getGlobalVariable(name);
    if (key) {
        isGlobal = 1;
        std::cout << "Find global variable\n";
        return key;
    }
    std::string msg = "Unknown variable name: " + name;
    return logErrorV(msg.c_str());
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAllocaWithTypeSize(StringRef VarName, Type* type, Value* size = nullptr, BasicBlock* Scope = llvmBuilder->GetInsertBlock(), BasicBlock::iterator Point =llvmBuilder->GetInsertPoint()) {
    IRBuilder<> allocator(Scope, Point);
    return allocator.CreateAlloca(type, size, VarName);
}

static AllocaInst *CreateEntryBlockAllocaWithTypeSize(StringRef VarName, int type_id, Value* size = nullptr, BasicBlock* Scope = llvmBuilder->GetInsertBlock(), BasicBlock::iterator Point =llvmBuilder->GetInsertPoint()) {
    IRBuilder<> allocator(Scope, Point);
    return allocator.CreateAlloca(getVarType(*llvmContext, type_id), size, VarName);
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
            return value;
        }
    }
    print("Unknown type to be cast");
    return nullptr;
}

Value *TranslationUnitExprAST::codegen() {
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

Value *VarExprAST::codegen() {
    print("VarExpr, typeid: " + std::to_string(type));
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    auto varType = getVarType(*llvmContext, this->getType());

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
    if(type == TYPEID_CHAR_PTR){
        auto ArraySize = llvmBuilder->getInt32(1000);
        AllocaInst * arrayspace = CreateEntryBlockAllocaWithTypeSize(name, llvmBuilder->getInt8Ty(), ArraySize);
        llvmBuilder->CreateStore(arrayspace, alloca);
    }else{
        llvmBuilder->CreateStore(castedVal, alloca);
    }
    NamedValues.top()[name] = alloca;
    return castedVal;
}

Value *ArrayExprAST::codegen() {
    int isGlobal = 0;
    auto val = getVariable(name, isGlobal);
    if (val) {
        std::string errorMsg = "Variable " + name + " is already defined";
        return logErrorV(errorMsg.c_str());
    }

    auto varType = getVarType(*llvmContext, this->getType());
    print("ArrayExpr, typeid: " + std::to_string(type));

    auto alloca = CreateEntryBlockAllocaWithTypeSize(name, varType);
    auto arrayType = ArrayType::get(varType, size);
    // auto arrayPtr = CreateEntryBlockAllocaWithTypeSize(name, arrayType);
    // llvmBuilder->CreateStore(arrayPtr, alloca);
    auto arrayPtr = llvmBuilder->CreateAlloca(arrayType);
    NamedValues[name] = arrayPtr;
    // auto defaultValue = getInitVal(varType);
    // for (int i=0; i < size; i++) {
    //     auto index = llvmBuilder->getInt32(i);
    //     auto arrayElem = llvmBuilder->CreateGEP(arrayPtr, index);
    //     llvmBuilder->CreateStore(defaultValue, arrayElem);
    // }
    return arrayPtr;
}

Value *VarRefExprAST::codegen() {
    print("VarRefExpr");
    int isGlobal = 0;
    auto V = getVariable(name, isGlobal);
    if (!V) return V;
    if(isGlobal) {
        std::cout << "Find global var: " << name << std::endl;
        GlobalVariable *gV = static_cast<GlobalVariable *>(V);
        if(gV->isConstant() || initializing)
            return gV->getInitializer();
        else
            return llvmBuilder->CreateLoad(V);
    }

    std::cout << "Value of varRef: " + getLLVMTypeStr(V) << std::endl;

    auto type = getVarType(*llvmContext, this->type);
    std::cout << "Type: " << getLLVMTypeStr(type) << " Type-id: " << this->type << " Name: " << this->name << std::endl;
    auto retVal = llvmBuilder->CreateLoad(type, V, name.c_str());

    std::cout << "Value of load: " + getLLVMTypeStr(retVal) << std::endl;
    return retVal;
}

Value *GlobalVarExprAST::codegen() {
    auto varType = getVarType(*llvmContext, this->init->getType());
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

Value *ArraySubExprAST::codegen() {
    print("ArraySubExpr, name: " + name);
    auto rhs = sub->codegen();
    if (!rhs) return nullptr;

    int isGlobal = 0;
    auto alloca = getVariable(name, isGlobal);
    if (!alloca) return nullptr;

    
    auto index = dyn_cast<ConstantInt>(rhs);
    
    if (!index) return logErrorV("Array index must be an integer");
    std::cout << getLLVMTypeStr(index) << std::endl;

    auto zero = llvmBuilder->getInt64(0);
    auto castIndex = createCast(index, Type::getInt64Ty(*llvmContext));
    auto elementPtr = llvmBuilder->CreateGEP(alloca, {zero, castIndex} );
    return llvmBuilder->CreateLoad(elementPtr);
};


Value *LiteralExprAST::codegen() {
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
            
            // auto strCode = ConstantDataArray::getString(*llvmContext, value, true);
            // // auto globalVar = GlobalVariable(*llvmModule, strCode->getType(), true, GlobalValue::PrivateLinkage, strCode);


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

            // return strCode;
        }

        default:
            return logErrorV("Invalid type!");
    }
}

UnaryExprAST::UnaryExprAST(const std::string &op, std::unique_ptr<ExprAST> rhs) {
    this->op = op;
    this->rhs = std::move(rhs);

    try {
        auto r = static_cast<VarRefExprAST *>(this->rhs.get());
        auto name = r->getName();
        this->name = name;
    } catch (std::exception &e) {
        logErrorV("Invalid reference declaration");
        logErrorV(e.what());
    }
}

Value *UnaryExprAST::codegen() {
    int opType = getUnaryOpType(op);
    print("Unary op: " + op + " " + std::to_string(opType));
    Value *right = rhs->codegen();
    if (!right) return nullptr;
    std::cout << "Right type: " << getLLVMTypeStr(right) << std::endl;

    switch (opType) {
        case POS: {
            return right;
        }
        case NEG: {
            if (right->getType()->isFloatingPointTy())
                return llvmBuilder->CreateFNeg(right);
            else 
                return llvmBuilder->CreateNeg(right);
        }
        case REF: {            
            // if (right->getType()->isPtrOrPtrVectorTy()) 
            //     return logErrorV("Unsupported reference operation for pointer type");
            int isGlobal = 0;
            auto V = getVariable(name, isGlobal);
            
            // V = llvmBuilder->CreateBitCast(V, V->getType()->getPointerTo());
            return V;
            
            // return nullptr;
        }
        case DEREF: {
            // if (!right->getType()->isPtrOrPtrVectorTy()) 
            //     return logErrorV("Unsupported reference operation for non-pointer type");

            auto V = llvmBuilder->CreateLoad(right);
            std::cout << "Unary ptr type: " << getLLVMTypeStr(V) << std::endl;
            return V;
        }
        default: {
            auto errorMsg = "Invalid unary op" + op;
            return logErrorV(errorMsg.c_str());
        }
    }

}

Value *BinaryExprAST::codegen() {
    print("Binary op: " + op);
    int opType = getBinaryOpType(op);

    
    if (opType == ASSIGN) {
        std::string name;
        Value *variable;
        if(type == UNARYOP) {
            UnaryExprAST *lhse = static_cast<UnaryExprAST *>(lhs.get());
            name = lhse->getName();
            // only for pointer type
            int isGlobal = 0;
            variable = getVariable(name, isGlobal);
            if(!variable) return nullptr;
            variable = llvmBuilder->CreateLoad(variable);
        }
        else if (type == ARRAYSUB) {
            ArraySubExprAST *lhse = static_cast<ArraySubExprAST *>(lhs.get());
            name = lhse->getName();
            
            auto rhs = lhse->sub->codegen();
            if (!rhs) return nullptr;

            int isGlobal = 0;
            auto alloca = getVariable(name, isGlobal);
            if (!alloca) return nullptr;

            auto index = dyn_cast<ConstantInt>(rhs);
    
            if (!index) return logErrorV("Array index must be an integer");
            std::cout << getLLVMTypeStr(index) << std::endl;

            auto zero = llvmBuilder->getInt64(0);
            auto castIndex = createCast(index, Type::getInt64Ty(*llvmContext));


            variable = llvmBuilder->CreateGEP(alloca, {zero, castIndex} );
            std::cout << getLLVMTypeStr(variable) << std::endl;
            if (!variable) return nullptr;
        }
        else {
            VarRefExprAST *lhse = static_cast<VarRefExprAST *>(lhs.get());
            name = lhse->getName();
            int isGlobal = 0;
            variable = getVariable(name, isGlobal);
            if (!variable) return nullptr;
        }
        
        Value *val =rhs->codegen();
        if (!val) {
            return logErrorV("Invaild Reference");
        }
        // Value *variable = NamedValues[lhse->getName()];

        // int isGlobal = 0;
        // auto variable = getVariable(lhse->getName(), isGlobal);
        // if (!variable) return variable;

        auto castedVal = createCast(val, variable->getType()->getPointerElementType());
        if (!castedVal){
            return logErrorV((std::string("Unsupported assign from "+ getLLVMTypeStr(val->getType()) +" to " + getLLVMTypeStr(variable->getType()->getPointerElementType())).c_str()));
        }
        val = castedVal;
        std::cout << getLLVMTypeStr(val) << std::endl;
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

    if(left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
        // always cast int to FP
        auto FPleft = createCast(left, Type::getDoubleTy(*llvmContext));
        auto FPright = createCast(right, Type::getDoubleTy(*llvmContext));
        if(!FPleft || !FPright){
            return logErrorV(std::string("Unsupported operand between "+ getLLVMTypeStr(left->getType()) +" and " + getLLVMTypeStr(right->getType())).c_str());
        }else{
            left = FPleft;
            right = FPright;
        }
        switch(opType) {
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
        switch(opType) {
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

Function *PrototypeAST::codegen() {
    std::vector<Type *> llvmArgs;
    for (auto arg: args) 
        llvmArgs.push_back(getVarType(*llvmContext, arg.second));
    
    FunctionType *functionType = FunctionType::get(getVarType(*llvmContext, retVal), llvmArgs, false);

    Function *F = Function::Create(functionType, Function::ExternalLinkage, name, llvmModule.get());

    auto it = this->args.begin();
    for (auto &arg: F->args()) {
        print(it->first);
        arg.setName(it->first);
        it++;
    }

    return F;
}

Function *FunctionDeclAST::codegen() {
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
    Type *retType = getVarType(*llvmContext, p.retVal);
    std::cout << "return type: " << getLLVMTypeStr(retType) << std::endl;
    if(!retType->isVoidTy()){
        retPtr = CreateEntryBlockAllocaWithTypeSize("ret_val", retType);
        llvmBuilder->CreateStore(getInitVal(retType), retPtr);
    }

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
    return currFunction;
}

Value *CompoundStmtExprAST::codegen() {
    print("New scope declared!");
    std::map<std::string, AllocaInst *> Scope = NamedValues.top();
    NamedValues.push(Scope);
    // Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    // BasicBlock *BB = BasicBlock::Create(*llvmContext, "compoundBB", currFunction, retBlock);
    // llvmBuilder->SetInsertPoint(BB);

    Value *retVal = llvmBuilder->getTrue();
    for (auto &expr: exprList){
        retVal = expr->codegen();
        if(!retVal) return nullptr;
    }
    print("Back to previous scope!");
    NamedValues.pop();
    return retVal;
}

Value *ReturnStmtExprAST::codegen() {
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

Value *ForExprAST::codegen() {
    /*
     * phi node: i {label: value}
     * entry, loop, after_loop
     */

    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    Value *startVal = start->codegen();
    if (!startVal) return nullptr;
    auto valType = startVal->getType();
    AllocaInst *alloca = CreateEntryBlockAllocaWithTypeSize(varName.c_str(), valType); 
    llvmBuilder->CreateStore(startVal, alloca);

    // BasicBlock *headerBlock = llvmBuilder->GetInsertBlock();
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "for_loop", currFunction);
    BasicBlock *afterBlock = BasicBlock::Create(*llvmContext, "after_for_loop", currFunction);
    BasicBlock *bodyBlock = BasicBlock::Create(*llvmContext, "for_body", currFunction);

    llvmBuilder->CreateBr(loopBlock);
    llvmBuilder->SetInsertPoint(loopBlock);

    NamedValues.top()[varName] = alloca;

    Value *endVal = end->codegen();
    if (!endVal) return nullptr;
    if(endVal->getType()->isFloatingPointTy())
        endVal = llvmBuilder->CreateFCmpONE(endVal, getInitVal(endVal->getType()), "loop_end");

    else if(endVal->getType()->isIntegerTy(INTEGER_BITWIDTH))
        endVal = llvmBuilder->CreateICmpNE(endVal, getInitVal(endVal->getType()), "loop_end");

    llvmBuilder->CreateCondBr(endVal, bodyBlock, afterBlock);
    
    llvmBuilder->SetInsertPoint(bodyBlock);
    if (!body->codegen()) return nullptr;
    

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

    return Constant::getNullValue(valType);
}

Value *CallExprAST::codegen() {
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

Value *IfExprAST::codegen() {
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

Value *WhileExprAST::codegen() {
    // TODO
    print("Generate for while expr");
    

    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    BasicBlock *entryBlock = BasicBlock::Create(*llvmContext, "entry", currFunction);
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "while_loop_body", currFunction);
    BasicBlock *endBlock = BasicBlock::Create(*llvmContext, "while_loop_end", currFunction);

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
    // std::cout << condVal->getType() << std::endl;
    // condVal = getInitVal(Type::getDoubleTy(*llvmContext));
    // endVal = llvmBuilder->CreateFCmpONE(condVal, getInitVal(Type::getDoubleTy(*llvmContext)), "while_comp");
    // TODO 这个应该用什么类型


    if(condType->isFloatingPointTy())
        endVal = llvmBuilder->CreateFCmpONE(condVal, getInitVal(condVal->getType()), "while_comp");
    else
        endVal = llvmBuilder->CreateICmpNE(condVal, getInitVal(condVal->getType()), "while_comp");

    llvmBuilder->CreateCondBr(endVal, loopBlock, endBlock);
    llvmBuilder->SetInsertPoint(loopBlock);
    if(!body->codegen()) return nullptr;

    llvmBuilder->CreateBr(entryBlock);
    llvmBuilder->SetInsertPoint(endBlock);

    // TODO 应该返回什么值
    return Constant::getNullValue(condType);
}

Value *DoExprAST::codegen() {
    print("Generate for while expr");
    

    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    // BasicBlock *entryBlock = BasicBlock::Create(*llvmContext, "entry", currFunction);
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "do_while_loop_body", currFunction);
    BasicBlock *endBlock = BasicBlock::Create(*llvmContext, "do_while_loop_end", currFunction);

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


    // TODO 应该返回什么值
    return Constant::getNullValue(Type::getDoubleTy(*llvmContext));
}

Value * NullStmtAST::codegen(){
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
