/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-13 18:47:02
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/src/semantic.cpp
 * @Description: 
 */

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>

#include "semantic.h"
#include "symtab_impl.h"
#include "builtin.h"
#include "config.h"

using namespace std;

static bool warning_flag = false; // disable warning if true
static void semantic_warning(ast_loc_t loc, const char* fmt, ...);

static SymbolTable sym_tab;
static TypeAliasTable type_alias_tab;

int get_literal_type(const char* literal) {
    string test(literal);
    if (test.find("\"") != string::npos) {
        return TYPEID_STR;
    } else if (test.find("'") != string::npos) {
        return TYPEID_CHAR;
    } else if (test.find(".") != string::npos) {
        return TYPEID_DOUBLE;
    } else {
        try {  
            int t = stoi(test);
            if(to_string(t) != test) throw logic_error("");
            return TYPEID_INT;
        }
        catch (...) {
            try {
                stol(test);
                return TYPEID_LONG;
            }
            catch (...) {
                return TYPEID_OVERFLOW;
            }
        }
    } 
}

void add_symbol(const char* name, int type) {
    sym_tab.add(name, type);
}

int get_symbol_type(const char* name) {
    return sym_tab.get(name);
}

const char* get_function_type(const char* name) {
    auto table = sym_tab.get_global_sym_tab()->sym_tab_impl;
    if (table.find(name) == table.end() || !table.find(name)->second.func()) {
        return nullptr;
    } else {
        return strdup(table.find(name)->second.get_type_name().c_str());
    }
}

int get_type_size(int type_id){
    switch (type_id) {
    case TYPEID_CHAR:
        return sizeof(char);
    case TYPEID_SHORT:
        return sizeof(short);
    case TYPEID_INT:
        return sizeof(int);
    case TYPEID_LONG:
        return sizeof(long);
    case TYPEID_FLOAT:
        return sizeof(float);
    case TYPEID_DOUBLE:
        return sizeof(double);
    case TYPEID_VOID_PTR:
    case TYPEID_CHAR_PTR:
    case TYPEID_SHORT_PTR:
    case TYPEID_INT_PTR:
    case TYPEID_LONG_PTR:
    case TYPEID_FLOAT_PTR:
    case TYPEID_DOUBLE_PTR:
        return sizeof(void*);
    default:
        return -1;
    }
}

void add_type_alias(const char* name, int type_id) {
    type_alias_tab.add_typedef(name, type_id);
}

int get_type_alias(const char* name) {
    return type_alias_tab.get_typeid(name);
}

static int ptr_deref_type(int type_id) {
    assert(type_id >= TYPEID_VOID_PTR && type_id <= TYPEID_DOUBLE_PTR);
    int diff = TYPEID_VOID_PTR - TYPEID_VOID;
    return type_id - diff;
}

static int ptr_ref_type(int type_id) {
    assert(type_id >= TYPEID_VOID && type_id <= TYPEID_DOUBLE);
    int diff = TYPEID_VOID_PTR - TYPEID_VOID;
    return type_id + diff;
}

int is_declared(const char* name) {
    return sym_tab.is_redef(name);
}

void print_sym_tab() {
    sym_tab.print();
}

static int expr_type_check(ast_node_ptr uni, ast_node_ptr op) {
    if (uni->type_id == TYPEID_VOID || uni->type_id == TYPEID_STR)
        return -1;
    if (get_function_type(uni->val) != nullptr) {
        if (string(op->val) == "&") {
            return ptr_ref_type(uni->type_id);
        }
        return -1;
    }
    if (string(op->val) == "*") {
        if (uni->type_id < TYPEID_VOID_PTR || uni->type_id > TYPEID_DOUBLE_PTR
            || string(uni->token) == "Literal")
            return -1;
        return ptr_deref_type(uni->type_id);
    }
    if (string(op->val) == "&") {
        if (uni->type_id <= TYPEID_VOID || uni->type_id > TYPEID_DOUBLE
            || string(uni->token) == "Literal")
            return -1;
        return ptr_ref_type(uni->type_id);
    }
    return uni->type_id;
}

static int expr_type_check(ast_node_ptr left, ast_node_ptr right, ast_node_ptr op) {
    if (left->type_id == TYPEID_VOID || right->type_id == TYPEID_VOID
        || left->type_id == TYPEID_STR || right->type_id == TYPEID_STR) {
        return -1;
    }
    if (left->type_id >= TYPEID_VOID_PTR && right->type_id >= TYPEID_VOID_PTR
        && string(op->val) != "=") {
        return -1;
    }
    if (get_function_type(left->val) != nullptr ||
        get_function_type(right->val) != nullptr) { // check if the identifier is a function
        return -1;
    }
    if (string(op->val) == "=") {
        if (left->type_id < right->type_id) {
            semantic_warning(op->pos, "implicit conversion from '%s' to '%s' might change value",
                typeid_deref[right->type_id], typeid_deref[left->type_id]);
        }
    }
    return max(left->type_id, right->type_id);
}


static fstream& goto_line(fstream& file, unsigned int num){
    file.seekg(ios::beg);
    for(int i=0; i < num - 1; ++i){
        file.ignore(numeric_limits<streamsize>::max(),'\n');
    }
    return file;
}

static void semantic_warning(ast_loc_t loc, const char* fmt, ...) {
    if (warning_flag) return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, COLOR_BOLD "%s:%d:%d:" COLOR_PURPLE " warning: " COLOR_NORMAL COLOR_BOLD,
        loc.filename, loc.last_line, loc.last_column);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n" COLOR_NORMAL);
    va_end(ap);
    fstream file(loc.filename);
    goto_line(file, loc.last_line);
    string err_line;
    getline(file, err_line);
    cerr << err_line << endl;
    file.close();
    for (int i = 0; i < loc.last_column - 1; i++) {
        cerr <<  COLOR_GREEN "~";
    }
    cerr << "^" COLOR_NORMAL << endl;
}

static void semantic_error(int* n_errs, ast_loc_t loc, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, COLOR_BOLD "%s:%d:%d:" COLOR_RED " error: " COLOR_NORMAL COLOR_BOLD,
        loc.filename, loc.last_line, loc.last_column);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n" COLOR_NORMAL);
    va_end(ap);
    fstream file(loc.filename);
    goto_line(file, loc.last_line);
    string err_line;
    getline(file, err_line);
    cerr << err_line << endl;
    file.close();
    for (int i = 0; i < loc.last_column - 1; i++) {
        cerr <<  COLOR_GREEN " ";
    }
    cerr << "^" COLOR_NORMAL << endl;
    if(n_errs) (*n_errs)++;
}

static string check_decl(ast_node_ptr node) {
    if (node->type_id == TYPEID_VOID) {
        return "cannot declare a void type variable";
    }
    if (string(node->token) == "ParmVarDecl" && node->type_id == TYPEID_STR) {
        return "cannot declare a string type parameter";
    }
    if (node->type_id == TYPEID_STR && node->n_child == 0) {
        return "cannot declare a string type variable without initializer";
    }
    if (node->type_id == TYPEID_STR && !(node->n_child == 1
        && (string(node->child[0]->token) == "Literal") ||
        (string(node->child[0]->token).find("BUILTIN_") != string::npos))) {
        return "cannot declare a string type variable with initializer other than a string literal";
    }
    return "";
}

static string const_fold(int* n_errs, ast_node_ptr num, string op) {
    int type = num->type_id;
    stringstream ss;
    assert(num->type_id != TYPEID_VOID);
    if (op == "-") {
        if (num->type_id > TYPEID_LONG) {
            ss << fixed << setprecision(16) << -atof(num->val);
            return ss.str();
        } else {
            return to_string(-atol(num->val));
        }
    } else if (op == "~") {
        if (num->type_id > TYPEID_LONG) {
            semantic_error(n_errs, num->pos, "invalid operand to unary expression");
        } else {
            return to_string(~atol(num->val));
        }
    } else if (op == "!") {
        if (num->type_id > TYPEID_LONG) {
            semantic_error(n_errs, num->pos, "invalid operand to unary expression");
        } else {
            return to_string(!atol(num->val));
        }
    }
    return "";
}

static string const_fold(int* n_errs, ast_node_ptr left, ast_node_ptr right, string op, int res_type) {
    int typel = left->type_id, typer = right->type_id;
    stringstream ss;
    assert(op != "=");
    if (op == "+") {
        if(res_type == TYPEID_LONG) {
            return to_string(atol(left->val) + atol(right->val));;
        } else if(res_type > TYPEID_LONG){
            ss << fixed << setprecision(16) << atof(left->val) + atof(right->val);
            return ss.str();
        } else if(res_type < TYPEID_LONG){
            return to_string(atoi(left->val) + atoi(right->val));
        }
    } else if (op == "-") {
        if(res_type == TYPEID_LONG) {
            return to_string(atol(left->val) - atol(right->val));;
        } else if(res_type > TYPEID_LONG){
            ss << fixed << setprecision(16) << atof(left->val) - atof(right->val);
            return ss.str();
        } else if(res_type < TYPEID_LONG){
            return to_string(atoi(left->val) - atoi(right->val));
        }
    } else if (op == "*") {
        if(res_type == TYPEID_LONG) {
            return to_string(atol(left->val) * atol(right->val));;
        } else if(res_type > TYPEID_LONG){
            ss << fixed << setprecision(16) << atof(left->val) * atof(right->val);
            return ss.str();
        } else if(res_type < TYPEID_LONG){
            return to_string(atoi(left->val) * atoi(right->val));
        }
    } else if (op == "/") {
        if(res_type == TYPEID_LONG) {
            return to_string(atol(left->val) / atol(right->val));;
        } else if(res_type > TYPEID_LONG){
            ss << fixed << setprecision(16) << atof(left->val) / atof(right->val);
            return ss.str();
        } else if(res_type < TYPEID_LONG){
            return to_string(atoi(left->val) / atoi(right->val));
        }
    } else if (op == "%") {
        if (typel > TYPEID_LONG || typer > TYPEID_LONG) {
            semantic_error(n_errs, left->pos, "invalid operands to binary expression");
            return "";
        }
        return to_string(atoi(left->val) % atoi(right->val));
    } else if (op == "<") {
        int res = atof(left->val) < atof(right->val);
        return to_string(res);
    } else if (op == ">") {
        int res = atof(left->val) > atof(right->val);
        return to_string(res);
    } else if (op == "<=") {
        int res = atof(left->val) <= atof(right->val);
        return to_string(res);
    } else if (op == ">=") {
        int res = atof(left->val) >= atof(right->val);
        return to_string(res);
    } else if (op == "&&") {
        if (typel > TYPEID_LONG || typer > TYPEID_LONG) {
            semantic_error(n_errs, left->pos, "invalid operands to binary expression");
            return "";
        }
        int res = atoi(left->val) && atoi(right->val);
        return to_string(res);
    } else if (op == "||") {
        if (typel > TYPEID_LONG || typer > TYPEID_LONG) {
            semantic_error(n_errs, left->pos, "invalid operands to binary expression");
            return "";
        }
        int res = atoi(left->val) || atoi(right->val);
        return to_string(res);
    } else if (op == ">>") {
        if (typel > TYPEID_LONG || typer > TYPEID_LONG) {
            semantic_error(n_errs, left->pos, "invalid operands to binary expression");
            return "";
        }
        long res = atol(left->val) >> atol(right->val);
        return to_string(res);
    } else if (op == "<<") {
        if (typel > TYPEID_LONG || typer > TYPEID_LONG) {
            semantic_error(n_errs, left->pos, "invalid operands to binary expression");
            return "";
        }
        long res = atol(left->val) << atol(right->val);
        return to_string(res);
    } else if (op == "&") {
        if (typel > TYPEID_LONG || typer > TYPEID_LONG) {
            semantic_error(n_errs, left->pos, "invalid operands to binary expression");
            return "";
        }
        long res = atol(left->val) & atol(right->val);
        return to_string(res);
    } else if (op == "|") {
        if (typel > TYPEID_LONG || typer > TYPEID_LONG) {
            semantic_error(n_errs, left->pos, "invalid operands to binary expression");
            return "";
        }
        long res = atol(left->val) && atol(right->val);
        return to_string(res);
    } else if (op == "^") {
        if (typel > TYPEID_LONG || typer > TYPEID_LONG) {
            semantic_error(n_errs, left->pos, "invalid operands to binary expression");
            return "";
        }
        long res = atol(left->val) ^ atol(right->val);
        return to_string(res);
    } 
    return "";
}

static bool lvalue_validation(ast_node_ptr left, ast_node_ptr right, ast_node_ptr op) {
    return true; // debug only
    string ltok(left->token);
    if (ltok != "DeclRefExpr") {
        return false;
    } else {
        auto scope_sym_tab = sym_tab.get_global_sym_tab()->sym_tab_impl;
        auto it = scope_sym_tab.find(left->val);
        if (it != scope_sym_tab.end() && it->second.func())
            return false;
    }
    return true;
}

static void semantic_check_impl(int* n_errs, ast_node_ptr node) {
    if (node == nullptr) {
        return;
    }
    bool already_checked = false;
    string token(node->token);
    if ((token == "VarDecl" || token == "ParmVarDecl")
        && string(node->parent->token) != "ArrayDecl") {
        if (is_declared(node->val)) {
            semantic_error(n_errs, node->pos, "redefinition of '%s'", node->val);
        } else {
            string validation = check_decl(node);
            if (!validation.empty()) {
                semantic_error(n_errs, node->pos, "%s", validation.c_str());
            }
            if (node->n_child == 1 && string(node->child[0]->token) == "Literal") {
                sym_tab.add(node->val, node->type_id, false, node->child[0]->val);
            } else if (node->n_child == 1 && string(node->child[0]->token).find("BUILTIN_") != string::npos) {
                semantic_check_impl(n_errs, node->child[0]);
                sym_tab.add(node->val, node->type_id, false, node->child[0]->val);
            } else {
                sym_tab.add(node->val, node->type_id);
            }
            if (token == "ParmVarDecl") {
                assert(string(node->parent->token) == "FunctionDecl" || string(node->parent->token) == "ProtoDecl");
                assert(sym_tab.get_cur_sym_tab()->parent != nullptr);
                sym_tab.get_global_sym_tab()->sym_tab_impl.find(node->parent->val)->second.add_param(node->type_id);
            }
        }
    } else if (token == "CompoundStmt" || token == "FunctionDecl" || token == "ProtoDecl") {
        if (token == "FunctionDecl" || token == "ProtoDecl") {
            assert(sym_tab.get_cur_sym_tab()->parent == nullptr);
            if(is_declared(node->val)) {
                bool isredef = false;
                bool va_args = false;
                int args = 0;

                SymbolAttr sym_attr = sym_tab.get_global_sym_tab()->sym_tab_impl.at(node->val);
                if(sym_attr.has_body()){
                    semantic_error(n_errs, node->pos, "redefinition of '%s'", node->val);
                }
                if(sym_attr.get_type_id() != node->type_id){
                    semantic_error(n_errs, node->pos, "conflicting types for '%s'", node->val);
                }
                for(int i=0;i<node->n_child;i++) {
                    if(string(node->child[i]->token) == "ParmVarDecl"){
                        args++;
                        if(node->child[i]->type_id != sym_attr.get_arg_type(i)){
                            isredef = true;
                            break;
                        }
                    }
                    if(string(node->child[i]->token) == "VariadicParms"){
                        va_args = true;
                    }
                }
                if(args != sym_attr.param_nums() || va_args != sym_attr.has_va_args())
                    isredef = true;
                if(isredef)
                    semantic_error(n_errs, node->pos, "conflicting types for '%s'", node->val);
                else{
                    sym_tab.remove(node->val);
                }
            }
            sym_tab.add(node->val, node->type_id, true);
        }
        already_checked = true;
        if (token == "CompoundStmt" && string(node->parent->token) == "FunctionDecl") {
            already_checked = false;
            sym_tab.get_global_sym_tab()->sym_tab_impl.at(node->parent->val).set_body(true);
        }
        if (already_checked) {
            sym_tab.enter_scope(node->val);
            for(int i=0;i<node->n_child;i++) {
                semantic_check_impl(n_errs, node->child[i]);
            }
            sym_tab.exit_scope();
        }
        if(token == "CompoundStmt") {
            node->type_id = node->n_child ? node->child[node->n_child - 1]->type_id : TYPEID_VOID;
        }
    } else if (token == "DeclRefExpr") {
        if (get_symbol_type(node->val) < 0 && string(node->val).find("__builtin_") != 0 && string(node->val).find("__llvm_") != 0 ) {
            semantic_error(n_errs, node->pos, "use of undeclared identifier '%s'", node->val);
            assert(node->parent != nullptr);
            if (string(node->parent->token) == "CallExpr" && node == node->parent->child[0]) {
                node->parent->type_id = TYPEID_INT;
            }
        } else {
            node->type_id = get_symbol_type(node->val);
            node->type_id < 0 ? node->type_id = TYPEID_INT : 0;
            assert(node->parent != nullptr);
            if (string(node->parent->token) == "CallExpr" && node == node->parent->child[0]) {
                node->parent->type_id = node->type_id;
                if (string(node->val).find("__builtin_") != 0) {
                    auto sym_attr = sym_tab.get_global_sym_tab()->sym_tab_impl.find(node->val)->second;
                    assert(sym_attr.param_nums() >= 0);
                    if (!sym_attr.has_va_args() && node->parent->n_child - 1 != sym_attr.param_nums()) {
                        semantic_error(n_errs, node->pos,
                            "invalid number of arguments to function '%s', expected %d argument(s)",
                            node->val, sym_attr.param_nums());
                    } else if (sym_attr.has_va_args() && node->parent->n_child - 1 < sym_attr.param_nums()) {
                        semantic_error(n_errs, node->pos,
                            "invalid number of arguments to function '%s', expected at least %d argument(s)",
                            node->val, sym_attr.param_nums());
                    }
                }
            }
        }
    } else if (token == "BinaryOperator") {
        already_checked = true;
        assert(node->n_child == 2);
        for (int i = 0;i < node->n_child;i++) {
            semantic_check_impl(n_errs, node->child[i]);
        }
        if (string(node->val) == "=" && !lvalue_validation(node->child[0], node->child[1], node)) {
            semantic_error(n_errs, node->pos, "lvalue required as left operand of assignment");
        }
        int type = expr_type_check(node->child[0], node->child[1], node);
        if (type < 0) {
            semantic_error(n_errs, node->pos, "incompatible types in binary expression");
        } else {
            node->type_id = type;
            if (string(node->val) != "="
                && string(node->child[0]->token) == "Literal"
                && string(node->child[1]->token) == "Literal") {
                string res = const_fold(n_errs, node->child[0], node->child[1], node->val, type);
                if (!res.empty()) {
                    strcpy(node->val, res.c_str());
                    strcpy(node->token, "Literal");
                    node->n_child = 0;
                }
            }
        }
    } else if (token == "UnaryOperator") {
        already_checked = true;
        assert(node->n_child == 1);
        semantic_check_impl(n_errs, node->child[0]);
        int type = expr_type_check(node->child[0], node);
        if (type < 0) {
            semantic_error(n_errs, node->pos, "incompatible types in unary expression");
        } else {
            node->type_id = type;
            if(string(node->child[0]->token)== "Literal") {
                string res = const_fold(n_errs, node->child[0], node->val);
                if (!res.empty()) {
                    strcpy(node->val, res.c_str());
                    strcpy(node->token, "Literal");
                    node->n_child = 0;
                }
            }
        }
    } else if (token.find("BUILTIN_") == 0) {
        if (token == "BUILTIN_ITOA") {
            assert(node->n_child == 2);
            string temp(node->child[0]->val);
            int n = 0;
            if (string(node->child[0]->token) == "DeclRefExpr" &&
                get_symbol_type(node->child[0]->val) == TYPEID_STR) {
                n = atoi(sym_tab.get_value(node->child[0]->val).c_str());
            } else if (string(node->child[0]->token) == "Literal" && node->child[0]->type_id == TYPEID_STR) {
                if(temp.find("\"") == 0) {
                    temp = temp.substr(1, temp.size() - 2);
                }
                n = atoi(temp.c_str());
            } else {
                semantic_check_impl(n_errs, node->child[0]);
                if(string(node->child[0]->token) == "Literal" && node->child[0]->type_id == TYPEID_STR) {
                    temp = node->child[0]->val;
                    if(temp.find("\"") == 0) {
                        temp = temp.substr(1, temp.size() - 2);
                    }
                    n = atoi(temp.c_str());
                } else {
                    semantic_error(n_errs, node->pos,
                        "invalid argument to builtin function '%s', expected 'string, const int'", token.c_str());
                }
            }
            if (node->child[1]->type_id != TYPEID_INT) {
                semantic_error(n_errs, node->pos,
                    "invalid argument to builtin function '%s', expected 'string, const int'", token.c_str());
            }
            int base = atoi(node->child[1]->val);
            if (base != 2 && base != 8 && base != 10 && base != 16) {
                semantic_error(n_errs, node->pos,
                    "invalid base %d for itoa, expected 2, 8, 10 or 16", base);
            } else {
                strcpy(node->val, builtin_itoa(n, base));
            }
            sprintf(node->token, "Literal");
            node->n_child = 0;
        } else if (token == "BUILTIN_STRCAT") {
            assert(node->n_child == 2);
            string s1, s2, temp;
            if (string(node->child[0]->token) == "DeclRefExpr" &&
                get_symbol_type(node->child[0]->val) == TYPEID_STR) {
                s1 = sym_tab.get_value(node->child[0]->val);
            } else if (string(node->child[0]->token) == "Literal" && node->child[0]->type_id == TYPEID_STR) {
                s1 = node->child[0]->val;
            } else {
                semantic_check_impl(n_errs, node->child[0]);
                if(string(node->child[0]->token) == "Literal" && node->child[0]->type_id == TYPEID_STR) {
                    s1 = node->child[0]->val;
                } else {
                    semantic_error(n_errs, node->pos,
                        "invalid argument to builtin function '%s', expected 'string, string'", token.c_str());
                }
            }
            if (string(node->child[1]->token) == "DeclRefExpr" &&
                get_symbol_type(node->child[1]->val) == TYPEID_STR) {
                s2 = sym_tab.get_value(node->child[1]->val);
            } else if (string(node->child[1]->token) == "Literal" && node->child[1]->type_id == TYPEID_STR) {
                s2 = node->child[1]->val;
            } else {
                semantic_check_impl(n_errs, node->child[1]);
                if(string(node->child[1]->token) == "Literal" && node->child[1]->type_id == TYPEID_STR) {
                    s2 = node->child[1]->val;
                } else {
                    semantic_error(n_errs, node->pos, "invalid argument to builtin function '%s', expected 'string, string'", token.c_str());
                }
            }
            strcpy(node->val, builtin_strcat(s1.c_str(), s2.c_str()));
            sprintf(node->token, "Literal");
            node->n_child = 0;
        } else if (token == "BUILTIN_STRLEN") {
            assert(node->n_child == 1);
            string s;
            if (string(node->child[0]->token) == "DeclRefExpr" &&
                get_symbol_type(node->child[0]->val) == TYPEID_STR) {
                s = sym_tab.get_value(node->child[0]->val);
            } else if (string(node->child[0]->token) == "Literal" && node->child[0]->type_id == TYPEID_STR) {
                s = node->child[0]->val;
            } else {
                semantic_check_impl(n_errs, node->child[0]);
                if(string(node->child[0]->token) == "Literal" && node->child[0]->type_id == TYPEID_STR) {
                    s = node->child[0]->val;
                } else {
                    semantic_error(n_errs, node->pos, "invalid argument to builtin function '%s', expected 'string'", token.c_str());
                }
            }
            sprintf(node->val, "%d", builtin_strlen(s.c_str()));
            sprintf(node->token, "Literal");
            node->n_child = 0;
        } else if (token == "BUILTIN_STRGET") {
            assert(node->n_child == 2);
            string s, ret = "";
            if (string(node->child[0]->token) == "DeclRefExpr" &&
                get_symbol_type(node->child[0]->val) == TYPEID_STR) {
                s = sym_tab.get_value(node->child[0]->val);
            } else if (string(node->child[0]->token) == "Literal" && node->child[0]->type_id == TYPEID_STR) {
                s = node->child[0]->val;
            } else {
                semantic_check_impl(n_errs, node->child[0]);
                if(string(node->child[0]->token) == "Literal" && node->child[0]->type_id == TYPEID_STR) {
                    s = node->child[0]->val;
                } else {
                    semantic_error(n_errs, node->pos,
                        "invalid argument to builtin function '%s', expected 'string, const int'", token.c_str());
                }
            }
            if (node->child[1]->type_id != TYPEID_INT) {
                semantic_error(n_errs, node->pos,
                    "invalid argument to builtin function '%s', expected 'string, const int'", token.c_str());
            }
            int index = atoi(node->child[1]->val);
            ret += builtin_strget(s.c_str(), index);
            strcpy(node->val, ret.c_str());
            sprintf(node->token, "Literal");
            node->n_child = 0;
        }
    } else if (token == "ReturnStmt") {
        assert(node->n_child <= 1);
        already_checked = true;
        for (int i = 0;i < node->n_child;i++) {
            semantic_check_impl(n_errs, node->child[i]);
        }
        if (node->n_child > 0) {
            node->type_id = node->child[0]->type_id;
        }
    } else if (token == "VariadicParms") {
        assert(string(node->parent->token) == "FunctionDecl" || string(node->parent->token) == "ProtoDecl");
        assert(sym_tab.get_cur_sym_tab()->parent != nullptr);
        sym_tab.get_global_sym_tab()->sym_tab_impl.find(node->parent->val)->second.set_va_args();
    } else if (token == "ArrayDecl") {
        assert(string(node->child[0]->token) == "VarDecl");
        if (get_literal_type(node->val) > TYPEID_LONG) {
            semantic_error(n_errs, node->pos, "array size must be an integer");
        }
        if (is_declared(node->child[0]->val)) {
            semantic_error(n_errs, node->pos, "redefinition of '%s'", node->child[0]->val);
        } else {
            sym_tab.add(node->child[0]->val, node->child[0]->type_id);
        }
        if (node->n_child == 2) {
            int init_num = node->child[1]->n_child;
            if (string(node->val) == "length_tbd") {
                sprintf(node->val, "%d", init_num);
            } else if (node->type_id != TYPEID_CHAR && node->child[1]->type_id == TYPEID_CHAR){
                semantic_error(n_errs, node->pos, "initializing wide char array with non-wide string literal");
            } else {
                int len = atoi(node->val);
                if (len < init_num) {
                    semantic_error(n_errs, node->pos, "array length %d is smaller than initializer list size %d", len, init_num);
                }
            } 
        } 
    } else if (token == "ArraySubscriptExpr") {
        assert(node->n_child == 2);
        already_checked = true;
        for (int i = 0;i < node->n_child;i++) {
            semantic_check_impl(n_errs, node->child[i]);
        }
        if (node->child[1]->type_id > TYPEID_LONG) {
            semantic_error(n_errs, node->pos, "array subscript must be an integer");
        }
        node->type_id = ptr_deref_type(node->child[0]->type_id);
    } else if (token == "__SIZEOF") {
        assert(node->n_child == 1);
        already_checked = true;
        semantic_check_impl(n_errs, node->child[0]);
        sprintf(node->token, "%s", "Literal");
        node->type_id = TYPEID_INT;
        if (string(node->child[0]->token) == "Literal"
            && get_literal_type(node->child[0]->val) == TYPEID_STR) {
            sprintf(node->val, "%d", builtin_strlen(node->child[0]->val));
        } else {
            sprintf(node->val, "%d", get_type_size(node->child[0]->type_id));
        }
        node->n_child = 0;
    }
    if (already_checked) return;
    else {
        for (int i = 0;i < node->n_child;i++) {
            semantic_check_impl(n_errs, node->child[i]);
        }
    }
}

void semantic_check(int* n_errs, ast_node_ptr root, int w_flag) {
    warning_flag = w_flag;
    sym_tab.init();
    semantic_check_impl(n_errs, root);
    int valid_nodes = 0;
    bool has_entry = false;
    ast_node_ptr * child = new ast_node_ptr[root->n_child];
    ast_node_ptr * begin = child;
    for (int i=0; i<root->n_child; i++) {
        if(
            string(root->child[i]->token) == "ProtoDecl" &&
            sym_tab.get_global_sym_tab()->sym_tab_impl.at(string(root->child[i]->val)).has_body()
        ){
            continue;
        }else{
            if(
                string(root->child[i]->token) == "FunctionDecl" &&
                string(root->child[i]->val) == "main" &&
                sym_tab.get_global_sym_tab()->sym_tab_impl.at(string(root->child[i]->val)).has_body()
            ){
                has_entry = true;
            }
            if(string(root->child[i]->token) == "ProtoDecl"){
                strcpy(root->child[i]->token, "FunctionDecl");
            }
            *begin++ = root->child[i];
            valid_nodes++;
        };
    }
    if(!has_entry){
        semantic_warning(root->child[root->n_child-1]->pos, "implicit entry/start for main executable");
    }
    root->n_child = valid_nodes;
    root->child = child;
}
