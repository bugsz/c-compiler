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

#include "semantic.h"
#include "builtin.h"
#include "config.h"

using namespace std;

static const char* typeid_deref[] = {
    "void", "char", "short", "int", "long", "float", "double", "string"
};

static bool warning_flag = false; // disable warning if true
static void semantic_warning(ast_loc_t loc, const char* fmt, ...);

class SymbolAttr {
    friend class SymbolTable;
    
public:
    SymbolAttr(int type, bool func) {
        type_id = type;
        is_func = func;
    }
    ~SymbolAttr() = default;

    void add_param(const int& type_id) {
        param_type_id.push_back(type_id);
    }

    int param_nums() {
        return is_func ? param_type_id.size() : -1;
    }

    string get_type_name() {
        if (!is_func) {
            return typeid_deref[type_id];
        } else {
            string ret = string(typeid_deref[type_id]) + " (*) (";
            for (int i = 0; i < param_type_id.size(); ++i) {
                ret += typeid_deref[param_type_id[i]];
                if (i != param_type_id.size() - 1) {
                    ret += ",";
                }
            }
            ret += ")";
            return ret;
        }
    }

    bool func() {
        return is_func;
    }

private:
    // value type for expr/literal, return type for function
    int type_id;
    bool is_func;
    vector<int> param_type_id;
};

struct _table {
    _table(bool global = false, string name = "Annonymous") {
        parent = nullptr;
        is_global = global;
        this->name = is_global ? "TranslationUnit" : name;
    }
    unordered_map<string, SymbolAttr> sym_tab_impl;
    unordered_map<string, string> key_value;
    string name;
    bool is_global;
    _table* parent;
    vector<_table*> child;
};

class SymbolTable {
public:
    SymbolTable() {
        init("");
    }
    ~SymbolTable() = default;

    void init(string name) {
        global_sym_tab = new _table(true);
        cur_table = global_sym_tab;
        filename = name;
    }
    
    void enter_scope(string name) {
        _table* new_table = new _table(false, name == "" ? "Annonymous" : name);
        new_table->parent = cur_table;
        cur_table->child.push_back(new_table);
        cur_table = new_table;
    }
    
    void exit_scope() {
        if (cur_table->parent == nullptr) {
            throw runtime_error("Cannot exit global scope");
        }
        cur_table = cur_table->parent;
    }
    
    void add(const char* name, int type, bool is_func = false, string value = "") {
        cur_table->sym_tab_impl.insert({ string(name), SymbolAttr(type, is_func) });
        cur_table->key_value[string(name)] = value;
    }

    string get_value(const char* name) {
        auto t = cur_table;
        while(t->parent != nullptr) {
            if (t->key_value.find(string(name)) != t->key_value.end()) {
                string ret = t->key_value[string(name)];
                if(ret.find("\"") != string::npos) {
                    ret = ret.substr(1, ret.size() - 2);
                }
                return ret;
            }
            t = t->parent;
        }
        return "";
    }

    int get(const char* name) {
        auto iter = cur_table;
        return get_impl(iter, name);
    }
    
    bool is_redef(const char* name) {
        auto sym_tab = cur_table->sym_tab_impl;
        return sym_tab.find(name) != sym_tab.end();
    }
    
    void print() {
        cout << "------------------------------------------" << endl;
        cout << "Symbol Table of " << filename << ":" << endl;
        cout << "------------------------------------------" << endl;
        _table* cur = global_sym_tab;
        print_impl(cur);
        cout << "------------------------------------------" << endl;
    }
    
    const char* get_filename() {
        return filename.c_str();
    }

    _table* get_cur_sym_tab() {
        return cur_table;
    }

    _table* get_global_sym_tab() {
        return global_sym_tab;
    }

private:
    string filename;
    _table* global_sym_tab;
    _table* cur_table;
    
    int get_impl(_table* t, const char* name) {
        if(t->sym_tab_impl.find(name) != t->sym_tab_impl.end()) {
            return t->sym_tab_impl.find(name)->second.type_id;
        } else if(t->parent != nullptr) {
            int ret = get_impl(t->parent, name);
            if(ret >= 0) return ret;
        }
        return -1;
    }

    void print_impl(_table* t) {
        static int tabs = 0;
        if (t->is_global) tabs = 0;
        for(int i = 0; i < tabs; i++) cout << " ";
        cout << "Scope: " << hex << &t << " " << t->name
            << (t->is_global ? " Global" : " Local") << endl;
        for (auto iter : t->sym_tab_impl) {
            for(int i = 0; i < tabs; i++) cout << "  ";
            cout << iter.first << " : " << iter.second.get_type_name() << endl;
        }
        for (auto child : t->child) {
            tabs += 2;
            print_impl(child);
            tabs -= 2;
        }
    }
};

static SymbolTable sym_tab;

int get_literal_type(const char* literal) {
    string test(literal);
    if (test.find(".") != string::npos) {
        return TYPEID_DOUBLE;
    } else if (test.find("'") != string::npos) {
        return TYPEID_CHAR;
    } else if (test.find("\"") != string::npos) {
        return TYPEID_STR;
    } else {
        return TYPEID_INT;
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

int is_declared(const char* name) {
    return sym_tab.is_redef(name);
}

void print_sym_tab() {
    sym_tab.print();
}

static int bin_expr_type_check(ast_node_ptr left, ast_node_ptr right, ast_node_ptr op) {
    if (left->type_id == TYPEID_VOID || right->type_id == TYPEID_VOID
        || left->type_id == TYPEID_STR || right->type_id == TYPEID_STR) {
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
        sym_tab.get_filename(), loc.last_line, loc.last_column);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n" COLOR_NORMAL);
    va_end(ap);
    fstream file(sym_tab.get_filename());
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
        sym_tab.get_filename(), loc.last_line, loc.last_column);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n" COLOR_NORMAL);
    va_end(ap);
    fstream file(sym_tab.get_filename());
    goto_line(file, loc.last_line);
    string err_line;
    getline(file, err_line);
    cerr << err_line << endl;
    file.close();
    for (int i = 0; i < loc.last_column - 1; i++) {
        cerr <<  COLOR_GREEN "~";
    }
    cerr << "^" COLOR_NORMAL << endl;
    (*n_errs)++;
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

static void semantic_check_impl(int* n_errs, ast_node_ptr node) {
    if (node == nullptr) {
        return;
    }
    bool already_checked = false;
    string token(node->token);
    if (token == "VarDecl" || token == "ParmVarDecl") {
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
                assert(string(node->parent->token) == "FunctionDecl");
                assert(sym_tab.get_cur_sym_tab()->parent != nullptr);
                sym_tab.get_global_sym_tab()->sym_tab_impl.find(node->parent->val)->second.add_param(node->type_id);
            }
        }
    } else if (token == "CompoundStmt" || token == "FunctionDecl") {
        if (token == "FunctionDecl") {
            assert(sym_tab.get_cur_sym_tab()->parent == nullptr);
            sym_tab.add(node->val, node->type_id, true);
        }
        already_checked = true;
        if (token == "CompoundStmt" && string(node->parent->token) == "FunctionDecl") {
            already_checked = false;
        }
        if (already_checked) {
            sym_tab.enter_scope(node->val);
            for(int i=0;i<node->n_child;i++) {
                semantic_check_impl(n_errs, node->child[i]);
            }
            sym_tab.exit_scope();
        }
    } else if (token == "DeclRefExpr") {
        if (get_symbol_type(node->val) < 0) {
            semantic_error(n_errs, node->pos, "use of undeclared identifier '%s'", node->val);
            assert(node->parent != nullptr);
            if (string(node->parent->token) == "CallExpr" && node == node->parent->child[0]) {
                node->parent->type_id = TYPEID_INT;
            }
        } else {
            node->type_id = get_symbol_type(node->val);
            assert(node->parent != nullptr);
            if (string(node->parent->token) == "CallExpr" && node == node->parent->child[0]) {
                node->parent->type_id = node->type_id;
                auto sym_attr = sym_tab.get_global_sym_tab()->sym_tab_impl.find(node->val)->second;
                assert(sym_attr.param_nums() >= 0);
                if (node->parent->n_child - 1 != sym_attr.param_nums()) {
                    semantic_error(n_errs, node->pos,
                        "invalid number of arguments to function '%s', expected %d argument(s)",
                        node->val, sym_attr.param_nums());
                }
            }
        }
    } else if (token == "BinaryOperator") {
        already_checked = true;
        assert(node->n_child == 2);
        for (int i = 0;i < node->n_child;i++) {
            semantic_check_impl(n_errs, node->child[i]);
        }
        int type = bin_expr_type_check(node->child[0], node->child[1], node);
        if (type < 0) {
            semantic_error(n_errs, node->pos, "incompatible types in binary expression");
        } else {
            node->type_id = type;
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
            printf("s = %s, index = %d\n", s.c_str(), index);
            ret += builtin_strget(s.c_str(), index);
            strcpy(node->val, ret.c_str());
            sprintf(node->token, "Literal");
            node->n_child = 0;
        }
    }
    if (already_checked) return;
    else {
        for (int i = 0;i < node->n_child;i++) {
            semantic_check_impl(n_errs, node->child[i]);
        }
    }
}

void semantic_check(const char* filename, int* n_errs, ast_node_ptr root, int w_flag) {
    warning_flag = w_flag;
    sym_tab.init(filename);
    semantic_check_impl(n_errs, root);
}