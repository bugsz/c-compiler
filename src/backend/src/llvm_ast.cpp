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
#include <iostream>
#include <llvm-12/llvm/ADT/APInt.h>
#include <llvm-12/llvm/ADT/StringRef.h>
#include <memory>

using namespace llvm;

static std::unique_ptr<LLVMContext> llvmContext;
static std::unique_ptr<Module> llvmModule;
static std::map<std::string, AllocaInst *> NamedValues;
static std::unique_ptr<IRBuilder<>> llvmBuilder;
static std::map<std::string, std::unique_ptr<PrototypeAST>> functionProtos;


inline void print(std::string a) { std::cout << a << std::endl; }

int getBinaryOpType(std::string binaryOp) {
    if(binaryOp == "+") return ADD;
    if(binaryOp == "-") return SUB;
    if(binaryOp == "*") return MUL;
    if(binaryOp == "/") return DIV;
    if(binaryOp == "<") return LT;
    if(binaryOp == "=") return EQ;
    return ADD;
}

ASTNodeType getNodeType(std::string token) {
    if(token == "TranslationUnitDecl") return TRANSLATIONUNIT;
    if(token == "FunctionDecl") return FUNCTIONDECL;
    if(token == "VarDecl") return VARDECL;
    if(token == "BinaryOperator") return BINARYOPERATOR;
    if(token == "Literal") return LITERAL;
    if(token == "CompoundStmt") return COMPOUNDSTMT;
    if(token == "DeclRefExpr") return DECLREFEXPR;
    if(token == "DeclStmt") return DECLSTMT;
    if(token == "ReturnStmt") return RETURNSTMT;
    if(token == "ForStmt") return FORSTMT;
    if(token == "ForDelimiter") return FORDELIMITER;
    if(token == "CallExpr") return CALLEXPR;
    if(token == "WhileStmt") return WHILESTMT;
    if(token == "DoStmt") return DOSTMT;
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
    int pos;
    str = std::regex_replace(str, std::regex(R"(\\n)"), "\n");
    str = std::regex_replace(str, std::regex(R"(\\t)"), "\t");
    return str;
}

Constant *getInitVal(Type *type) {
    if(type->isDoubleTy()) return ConstantFP::get(llvmBuilder->getDoubleTy(), 0);
    else if(type->isIntegerTy(INTEGER_BITWIDTH)) {
        if (INTEGER_BITWIDTH == 32) return llvmBuilder->getInt32(0);
        else return llvmBuilder->getInt64(0);
    }
    
    std::string error_msg("In func `getInitVal`: unknown type");
    fprintf(stderr, "Error: %s\n", error_msg.c_str());
    return nullptr;
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
    return (value->getType()->isDoubleTy() || value->getType()->isIntegerTy(INTEGER_BITWIDTH));
}

Function *getPrintfFunction(Module *mod) {
    const char *func_name = "printf";
    Function *func = mod->getFunction(func_name);
    // if(func) logErrorF("Cannot use `printf`, as it is a built-in function");
    if (func) return func;

    
    FunctionType *funcType = FunctionType::get(
        Type::getInt32Ty(mod->getContext()), 
        {Type::getInt8PtrTy(mod->getContext())},
         true); 
    func = Function::Create(funcType, Function::ExternalLinkage, func_name, mod);
    func->setCallingConv(CallingConv::C);
    
    return func;
    // return nullptr;
}

Value *getBuiltinFunction(std::string callee, std::vector<std::unique_ptr<ExprAST>> &args) {
    if(callee == "__builtin_printf") {
        // return nullptr;
        print("Using builtin function `printf`");

        std::vector<Value *> varArgs;
        auto formatArgs = llvmBuilder->CreateGlobalStringPtr(
            (static_cast<LiteralExprAST *>(args[0].get()))->getValue()
        );

        std::cout << static_cast<LiteralExprAST *>(args[0].get())->getValue() << std::endl;
        varArgs.push_back(formatArgs);
        for (int i=1;i<args.size();i++) {
            auto arg = args[i]->codegen();
            varArgs.push_back(arg);
        }
        
        auto func_printf = getPrintfFunction(llvmModule.get());
        return llvmBuilder->CreateCall(func_printf, varArgs);
    }
    return nullptr;
}


std::unique_ptr<ExprAST> generateBackendASTNode(ast_node_ptr root) {
    // if (!root->n_child) return nullptr;
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
            std::unique_ptr<ExprAST> returnStmt, compoundStmt;

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
                auto funcExpr = std::make_unique<FunctionDeclAST>(std::move(prototype), std::move(compoundStmt), std::move(returnStmt));
                return funcExpr;
            }
        }

        case DECLSTMT: {
            // 全局变量声明的AST就是 VarDecl, 无DeclStmt这一层
            return generateBackendASTNode(root->child[0]);
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
            binaryExpr->type = root->type_id;
            return binaryExpr;
        }

        case COMPOUNDSTMT: {
            std::vector<std::unique_ptr<ExprAST>> exprList;
            for(int i=0;i<root->n_child;i++) exprList.push_back(generateBackendASTNode(root->child[i]));
            auto compound = std::make_unique<CompoundStmtExprAST>(std::move(exprList));
            return compound;
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
    std::cout << "Get var type: " << type_id << std::endl;
    switch(type_id) {
        case TYPEID_INT:
            // std::cout << "int\n";
            if(INTEGER_BITWIDTH == 32) return Type::getInt32Ty(C);
            else return Type::getInt64Ty(C);
        case TYPEID_DOUBLE:
            return Type::getDoubleTy(C);
        case TYPEID_VOID:
            return Type::getVoidTy(C);
        case TYPEID_STR:
            return Type::getInt8PtrTy(C);
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
    if (auto *V = NamedValues[name]) {
        return V;
    }

    auto key = llvmModule->getGlobalVariable(name);
    // auto key = llvmModule->getNamedGlobal(name);
    // auto key = llvmBuilder->CreateLoad(name);
    if (key) {
        isGlobal = 1;
        // auto load = llvmBuilder->CreateLoad(key);
        return key;
    }
    std::string msg = "Unknown variable name: " + name;
    return logErrorV(msg.c_str());
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          StringRef VarName) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                     TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Type::getDoubleTy(*llvmContext), nullptr, VarName);
}

static AllocaInst *CreateEntryBlockAllocaWithType(Function *TheFunction,
                                          StringRef VarName, int type_id) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                     TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(getVarType(*llvmContext, type_id), nullptr, VarName);
}

static AllocaInst *CreateEntryBlockAllocaWithType(Function *TheFunction,
                                          StringRef VarName, Type *type) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                     TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(type, nullptr, VarName);
}


std::string getTypeString(Type *type) {
    if (type->isDoubleTy()) return std::string("double");
    if (type->isIntegerTy(INTEGER_BITWIDTH)) 
        return std::string("int_") + std::to_string(INTEGER_BITWIDTH);
    return std::string("Unknown");
}

Value *createCast(Value *value, Type *type) {
    Type *valType = value->getType();
    if(valType == type) return value;
    else if(type->isDoubleTy() && valType->isDoubleTy()) return value;
    else if(type->isIntegerTy(INTEGER_BITWIDTH) && valType->isIntegerTy(INTEGER_BITWIDTH)) return value;
    else if(type->isDoubleTy() && valType->isIntegerTy(INTEGER_BITWIDTH)) 
        return llvmBuilder->CreateSIToFP(value, type);
    else if(type->isIntegerTy(INTEGER_BITWIDTH) && valType->isDoubleTy())
        return llvmBuilder->CreateFPToSI(value, type);
    
    
    std::cout << "Not finish creating cast" << std::endl;
    return value;
    // return logErrorV("Invalid type cast!\n");
}

Value *TranslationUnitExprAST::codegen() {
    print("TranslationUnit");

    FunctionType *initFuncType = FunctionType::get(llvmBuilder->getVoidTy(), false);
    Function *initFunc = Function::Create(initFuncType, Function::ExternalLinkage, "__global_var_init", llvmModule.get());
    llvmBuilder->SetInsertPoint(BasicBlock::Create(*llvmContext, "entry", initFunc));

    // auto theBB = BasicBlock::Create(*llvmContext, "entry", llvmBuilder->GetInsertBlock()->getParent());
    // llvmBuilder->SetInsertPoint(theBB);

    int globalVar_len = globalVarList.size();
    int expr_len = exprList.size();
    print(std::to_string(globalVar_len) + " " + std::to_string(expr_len));

    for (int i=0;i<globalVar_len;i++) globalVarList[i]->codegen();
    llvmBuilder->CreateRetVoid();
    verifyFunction(*initFunc, &llvm::errs());

    // llvmBuilder->CreateCall(getFunction("__global_var_init"));

    for (int i=0;i<expr_len;i++) exprList[i]->codegen();

    

    return nullptr;
}

Value *VarExprAST::codegen() {
    print("VarExpr, typeid: " + std::to_string(type));
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();

    auto varType = getVarType(*llvmContext, this->getType());

    Value *initVal;


    if(init) {
        initVal = init->codegen();
        std::cout << getTypeString(initVal->getType()) << " " << getTypeString(varType) << std::endl;
        initVal = createCast(initVal, varType);

        if(!initVal) return nullptr;
    }
    else initVal = getInitVal(varType);
    
    
    AllocaInst *alloca = CreateEntryBlockAllocaWithType(currFunction, name, type);
    llvmBuilder->CreateStore(initVal, alloca);
    NamedValues[name] = alloca;
    return initVal;
}

Value *VarRefExprAST::codegen() {
    print("VarRefExpr");
    int isGlobal = 0;
    auto V = getVariable(name, isGlobal);
    if (!V) return V;
    if(isGlobal) {
        std::cout << "Find global var: " << name << std::endl;
        return llvmBuilder->CreateLoad(V);
        // return V;
    }

    
    return llvmBuilder->CreateLoad(getVarType(*llvmContext, this->type), V, name.c_str());
}

Value *GlobalVarExprAST::codegen() {
    print("GlobalVarExpr, typeid: " + std::to_string(type));
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();

    auto varType = getVarType(*llvmContext, this->type);

    Value *initVal;

    if(init->init) {
        initVal = init->init->codegen();
        std::cout << getTypeString(initVal->getType()) << " " << getTypeString(varType) << " " << getLLVMTypeStr(initVal) << std::endl;
        initVal = createCast(initVal, varType);

        if(!initVal) return nullptr;
    }
    else {
        std::cout << "No init val, using default" << std::endl;
        initVal = getInitVal(varType);
    }

    auto globalVar = createGlob(getVarType(*llvmContext, type), name);
    auto iVal = getInitVal(varType);
    globalVar->setInitializer(iVal);
    // llvmBuilder->CreateStore(initVal, globalVar);
    // TODO 目前不支持初始化成某个数

    return iVal;

    // return initVal;
}


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



Value *BinaryExprAST::codegen() {
    print("Binary op: " + op);
    int opType = getBinaryOpType(op);

    
    if (opType == EQ) {
        VarRefExprAST *lhse = static_cast<VarRefExprAST *>(lhs.get());
        Value *val = rhs->codegen();
        if (!val) return nullptr;
        // Value *variable = NamedValues[lhse->getName()];

        int isGlobal = 0;
        auto variable = getVariable(lhse->getName(), isGlobal);
        if (!variable) return variable;


        std::cout << getLLVMTypeStr(val) << std::endl;
        llvmBuilder->CreateStore(val, variable);
        return val;
    }

    // cast value: only support (int, double), (double, double), (double, int)
    

    Value *left = lhs->codegen();
    Value *right = rhs->codegen();

    if (!left || !right) {
        return nullptr;
    }

    if (!isValidBinaryOperand(left) || !isValidBinaryOperand(right)) return logErrorV("Invalid binary op!");

    if(left->getType()->isDoubleTy())
        right = createCast(right, left->getType());
    else left = createCast(left, right->getType());

    if(left->getType()->isDoubleTy()) {
        switch(opType) {
            case ADD:
                return llvmBuilder->CreateFAdd(left, right, "fadd");
            case SUB:
                return llvmBuilder->CreateFSub(left, right, "fsub");
            case MUL:
                return llvmBuilder->CreateFMul(left, right, "fmul");
            case LT:
                left = llvmBuilder->CreateFCmpULT(left, right, "flt");
                return llvmBuilder->CreateUIToFP(left, Type::getDoubleTy(*llvmContext), "booltmp");
            case DIV:
                return llvmBuilder->CreateFDiv(left, right);

            default:
                return logErrorV("Invalid binary operator");
        }
    }
    else {
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
                left = llvmBuilder->CreateICmpSLT(left, right, "ilt");
                left = llvmBuilder->CreateSIToFP(left, getVarType(*llvmContext, TYPEID_DOUBLE));
                left = llvmBuilder->CreateFPToSI(left, getVarType(*llvmContext, TYPEID_INT));
                return left;
                
            default:
                // llvmBuilder->CreateI
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
    print("Codegen for Functiondecl");
    // std::cout << prototype.get() << std::endl;
    auto &p = *prototype;
    functionProtos[p.getName()] = std::move(prototype);
    Function *currFunction = getFunction(p.getName());
    if (!currFunction) return logErrorF(("Unknown function referenced" + p.getName()).c_str());

    BasicBlock *BB = BasicBlock::Create(*llvmContext, "entry", currFunction);
    llvmBuilder->SetInsertPoint(BB);

    NamedValues.clear();
    for (auto &arg : currFunction->args()) {
        AllocaInst *alloca = CreateEntryBlockAllocaWithType(currFunction, arg.getName(), arg.getType());
        llvmBuilder->CreateStore(&arg, alloca);
        NamedValues[std::string(arg.getName())] = alloca;
    }

    Type *retType = getVarType(*llvmContext, p.retVal);
    // print(getTypeString(retType));
    Value *bodyVal = body->codegen();
    print(getTypeString(bodyVal->getType()) + " " + std::to_string(p.retVal));
    if(bodyVal) {
        if(p.getRetVal() == TYPEID_VOID) llvmBuilder->CreateRetVoid();
        else llvmBuilder->CreateRet(
            createCast(bodyVal, retType)
            );
        verifyFunction(*currFunction, &llvm::errs());
        return currFunction;
    }
    // TODO 没有明确指定的时候返回默认值

    // error reading body
    currFunction->eraseFromParent();
    return logErrorF("Error reading body");
}

Value *CompoundStmtExprAST::codegen() {
    Function *currFunction = llvmBuilder->GetInsertBlock()->getParent();
    // BasicBlock *BB = BasicBlock::Create(*llvmContext, "compoundBB", currFunction);
    // // currFunction->getBasicBlockList().push_back(BB);
    // llvmBuilder->SetInsertPoint(BB);

    Value *retVal;
    for (auto &expr: exprList) retVal = expr->codegen();
    
    return retVal;
}

Value *ReturnStmtExprAST::codegen() {
    print("Find return stmt");
    Value *retVal = body->codegen();
    if (!retVal) return logErrorV("No ret val");
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
    AllocaInst *alloca = CreateEntryBlockAllocaWithType(currFunction, varName.c_str(), valType); // TODO

    // print(varName);
    
    llvmBuilder->CreateStore(startVal, alloca);

    // BasicBlock *headerBlock = llvmBuilder->GetInsertBlock();
    BasicBlock *loopBlock = BasicBlock::Create(*llvmContext, "for_loop", currFunction);
    BasicBlock *afterBlock = BasicBlock::Create(*llvmContext, "after_for_loop", currFunction);
    BasicBlock *bodyBlock = BasicBlock::Create(*llvmContext, "for_body", currFunction);

    llvmBuilder->CreateBr(loopBlock);
    llvmBuilder->SetInsertPoint(loopBlock);

    NamedValues[varName] = alloca;

    Value *endVal = end->codegen();
    if (!endVal) return nullptr;
    if(endVal->getType()->isDoubleTy())
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
    std::cout   << "Type in for: "
                << getTypeString(startVal->getType()) 
                << " " 
                << getTypeString(endVal->getType())
                << " "
                << getTypeString(stepVar->getType()) 
                << std::endl;

    Value *nextVar = llvmBuilder->CreateLoad(valType, alloca, varName.c_str());
    llvmBuilder->CreateBr(loopBlock);
   
    llvmBuilder->SetInsertPoint(afterBlock);

    return Constant::getNullValue(valType);
}

Value *CallExprAST::codegen() {
    std::cout << callee << std::endl;

    auto builtin_ret = getBuiltinFunction(callee, args);
    if(builtin_ret) return builtin_ret;
    

    Function *calleeFunction = llvmModule->getFunction(callee);
    if (!calleeFunction) return logErrorV("Unknown function");
    if (calleeFunction->arg_size() != args.size()) logErrorV("Incorrect args");

    std::vector<Value *> argsValue;
    for (auto &arg: args) {
        Value *argValue = arg->codegen();
        if (!argValue) return nullptr;
        argsValue.push_back(argValue);
    }

    return llvmBuilder->CreateCall(calleeFunction, argsValue, "func_call");
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
            << getTypeString(condVal->getType())
            << std::endl;
    // std::cout << condVal->getType() << std::endl;
    // condVal = getInitVal(Type::getDoubleTy(*llvmContext));
    // endVal = llvmBuilder->CreateFCmpONE(condVal, getInitVal(Type::getDoubleTy(*llvmContext)), "while_comp");
    // TODO 这个应该用什么类型


    if(condType->isDoubleTy())
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
            << getTypeString(condVal->getType())
            << std::endl;


    if(condVal->getType()->isDoubleTy())
        endVal = llvmBuilder->CreateFCmpONE(condVal, getInitVal(condVal->getType()), "while_comp");
    else
        endVal = llvmBuilder->CreateICmpNE(condVal, getInitVal(condVal->getType()), "while_comp");

    llvmBuilder->CreateCondBr(endVal, loopBlock, endBlock);
    llvmBuilder->SetInsertPoint(endBlock);


    // TODO 应该返回什么值
    return Constant::getNullValue(Type::getDoubleTy(*llvmContext));
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

int main(int argc, const char **argv) {
    initializeModule();

    run_lib_backend(argc, argv);
    llvmModule->print(errs(), nullptr);
    int status_code = compile();
    std::cout << "Compile end with status code: " << status_code << std::endl << std::endl;
    return 0;
}