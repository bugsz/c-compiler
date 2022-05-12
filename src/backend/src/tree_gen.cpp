#include "ast/base.h"
#include "ast/array.h"
#include "ast/compound.h"
#include "ast/control_flow.h"
#include "ast/function.h"
#include "ast/literal.h"
#include "ast/null.h"
#include "ast/operator.h"
#include "ast/translationUnit.h"
#include "ast/variable.h"
#include "ast/varref.h"
#include "frontend.h"
#include <bits/stdc++.h>

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
                else if(isEqual(root->child[i]->token, "ArrayDecl")) {
                    auto arrayDecl = generateBackendASTNode(root->child[i]);
                    auto array = static_unique_pointer_cast<ArrayExprAST>(std::move(arrayDecl));

                    auto globalArrayDecl = std::make_unique<GlobalArrayExprAST>(std::move(array));
                    globalVarList.push_back(std::move(globalArrayDecl));
                }
                else{
                    exprList.push_back(generateBackendASTNode(root->child[i]));
                }
            }
                
            auto trUnit = std::make_unique<TranslationUnitExprAST>(std::move(globalVarList), std::move(exprList));
            return trUnit;
        }

        case FUNCTIONDECL: {
            std::vector<std::pair<std::string, int>> args;
            std::string name(val);    
            name = getFunctionName(name);

            int param_end = root->n_child;
            std::unique_ptr<ExprAST> compoundStmt;

            if(param_end && isEqual(root->child[param_end-1]->token, "CompoundStmt")) {
                compoundStmt = generateBackendASTNode(root->child[param_end - 1]);
                param_end -= 1;
            } else compoundStmt = nullptr;

            bool isVarArg = false;
            for (int i = 0; i < param_end; i++) {
                auto child = root->child[i];
                if (isEqual(child->token, "ParmVarDecl")) args.push_back(std::pair<std::string, int>((child->val), child->type_id));
                if (isEqual(child->token, "VariadicParms")) isVarArg = true;
            }

            auto prototype = std::make_unique<PrototypeAST>(name, root->type_id, args, isVarArg);
            prototype->codegen();
            if(!compoundStmt) {
                return prototype;
            } else {
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
            std::vector<std::unique_ptr<ExprAST>> init;
            if (root->n_child > 1) {
                auto child = root->child[1];
                for(int i=0;i<child->n_child;i++){
                    init.push_back(generateBackendASTNode(child->child[i]));
                }
            }

            auto array = std::make_unique<ArrayExprAST>(ptr2raw(var->getType()), var->getName(), arraySize, std::move(init));
            
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
                && (op_type == ASSIGN || op_type == ASSIGNPLUS || op_type == DEC || op_type == INC)
            ){
                fprintf(stderr, "lvalue can not be a reference\n");
                exit(-1);
            }
            auto LHS = generateBackendASTNode(root->child[0]);
            auto RHS = generateBackendASTNode(root->child[1]);
            if(op_type == ASSIGNPLUS || op_type == INC || op_type == DEC){
                auto left = generateBackendASTNode(root->child[0]);
                auto right = std::make_unique<BinaryExprAST>(getBinaryOpType(std::string(1, val[0])), std::move(LHS), std::move(RHS));
                return std::make_unique<BinaryExprAST>(op_type == ASSIGNPLUS ? ASSIGN:op_type, std::move(left), std::move(right));
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
            if(!root->n_child) {
                auto null = std::make_unique<NullStmtAST>();
                exprList.push_back(std::move(null));
            }
            for(int i=0;i<root->n_child;i++) exprList.push_back(generateBackendASTNode(root->child[i]));
            // std::cout << "CompoundStmt has: " << exprList.size() << std::endl;
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
            name = getFunctionName(name);
            std::vector<std::unique_ptr<ExprAST>> args;
            for(int i=1;i<root->n_child;i++) 
                args.push_back(generateBackendASTNode(root->child[i]));
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