/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-11 08:47:17
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/include/ast.h
 * @Description: 
 */

#pragma once
#define FRONTEND_AST

#include "ast_impl.h"

class FrontendAST {
public:
    FrontendAST(ast_node_ptr node) {
        this->root = node;
    }
    ~FrontendAST() = default;
    
private:
    ast_node_ptr root;
};