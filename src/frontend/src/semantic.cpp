#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cassert>

#include "semantic.h"
#include "config.h"

using namespace std;

static const char* typeid_deref[] = {
    "void", "char", "short", "int", "long", "float", "double"
};

class SymbolAttr {
    friend class SymbolTable;
    
    SymbolAttr(int type_id = 0, int is_used = 0) {
        this->type_id = type_id;
        this->is_used = is_used;
    }
    ~SymbolAttr() = default;

    int type_id;
    int is_used;
};

struct _table {
    _table(bool global = false, string name = "Annonymous") {
        parent = nullptr;
        is_global = global;
        this->name = is_global ? "TranslationUnit" : name;
    }
    unordered_map<string, int> sym_tab_impl;
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
    
    void add(const char* name, int type) {
        cur_table->sym_tab_impl[name] = type;
    }
    
    int get(const char* name) {
        auto iter = global_sym_tab;
        return get_impl(iter, name);
    }
    
    bool is_redef(const char* name) {
        auto sym_tab = cur_table->sym_tab_impl;
        return sym_tab.find(name) != sym_tab.end();
    }
    
    void print() {
        cout << "----------------------------------------" << endl;
        cout << "Symbol Table of " << filename << ":" << endl;
        cout << "----------------------------------------" << endl;
        _table* cur = global_sym_tab;
        print_impl(cur);
        cout << "----------------------------------------" << endl;
    }
    
    const char* get_filename() {
        return filename.c_str();
    }

private:
    string filename;
    _table* global_sym_tab;
    _table* cur_table;
    
    int get_impl(_table* t, const char* name) {
        if(t->sym_tab_impl.find(name) != t->sym_tab_impl.end()) {
            return t->sym_tab_impl[name];
        } else if(t->child.size() > 0) {
            for(auto child : t->child) {
                int ret = get_impl(child, name);
                if(ret >= 0) return ret;
            }
        }
        return -1;
    }

    void print_impl(_table* t) {
        static int tabs = 0;
        if (t->is_global) tabs = 0;
        for(int i = 0; i < tabs; i++) cout << "  ";
        cout << "Scope: " << hex << &t << " " << t->name
            << (t->is_global ? " Global" : " Local") << endl;
        for (auto iter : t->sym_tab_impl) {
            for(int i = 0; i < tabs; i++) cout << "  ";
            cout << iter.first << " : " << typeid_deref[iter.second] << endl;
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

int is_declared(const char* name) {
    return sym_tab.is_redef(name);
}

void print_sym_tab() {
    sym_tab.print();
}

void semantic_check_impl(int* n_errs, ast_node_ptr node) {
    if (node == nullptr) {
        return;
    }
    bool already_checked = false;
    string token(node->token);
    if (token == "VarDecl" || token == "ParmVarDecl") {
        if (is_declared(node->val)) {
            printf("%s:%d:%d: " COLOR_RED "error:" COLOR_NORMAL " redefinition of '%s'\n",
                sym_tab.get_filename(), node->ln, node->col, node->val);
            (*n_errs)++;
        } else {
            add_symbol(node->val, node->type_id);
        }
    } else if (token == "CompoundStmt" || token == "FunctionDecl") {
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
            printf("%s:%d:%d: " COLOR_RED "error:" COLOR_NORMAL " use of undeclared identifier '%s'\n",
                   sym_tab.get_filename(), node->ln, node->col, node->val);
            (*n_errs)++;
        }
    }
    if (already_checked) return;
    else {
        for (int i = 0;i < node->n_child;i++) {
            semantic_check_impl(n_errs, node->child[i]);
        }
    }
}

void semantic_check(const char* filename, int* n_errs, ast_node_ptr root) {
    sym_tab.init(filename);
    semantic_check_impl(n_errs, root);
}