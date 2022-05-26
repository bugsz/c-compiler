#include "ast/base.h"
#include "frontend.h"


std::unique_ptr<ExprAST> generateBackendASTNode(ast_node_ptr root);
std::unique_ptr<ExprAST> generateBackendAST(struct lib_frontend_ret frontend_ret);