#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <exception>

using std::string;
using std::cin, std::cout, std::endl, std::hex;
using std::vector, std::unordered_map;
using std::runtime_error;

extern const char* typeid_deref[15];

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

    int get_type_id() {
        return type_id;
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
            if (va_args) ret += ",...";
            ret += ")";
            return ret;
        }
    }

    bool func() {
        return is_func;
    }

    void set_va_args(bool has = true) {
        va_args = true;
    }

    bool has_va_args() {
        return va_args;
    }
    
    int get_arg_type(int i) {
        if(i >= param_type_id.size()){
            return TYPEID_VOID;
        };
        return param_type_id[i];
    }
private:
    // value type for expr/literal, return type for function
    int type_id;
    bool is_func;
    bool va_args = false ;
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
        init();
    }
    ~SymbolTable() = default;

    void init() {
        global_sym_tab = new _table(true);
        cur_table = global_sym_tab;
    }
    
    void enter_scope(string name) {
        if(name != ""){
            vector<_table*> funcs = get_global_sym_tab()->child;
            for(auto iter = funcs.begin(); iter != funcs.end(); iter++){
                if((*iter)->name == name){
                    cur_table = *iter;
                    return;
                }
            }
        }
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
        cout << "Symbol Table" << endl;
        cout << "------------------------------------------" << endl;
        _table* cur = global_sym_tab;
        print_impl(cur);
        cout << "------------------------------------------" << endl;
    }

    _table* get_cur_sym_tab() {
        return cur_table;
    }

    _table* get_global_sym_tab() {
        return global_sym_tab;
    }

private:
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

class TypeAliasTable {
public:
    TypeAliasTable() {
        init_typedef();
    }
    ~TypeAliasTable() = default;

    void add_typedef(const char* name, int type_id) {
        type_alias_table[string(name)] = type_id;
    }

    int get_typeid(const char* name) {
        if (type_alias_table.find(string(name)) != type_alias_table.end()) {
            return type_alias_table[string(name)];
        } else {
            return -1;
        }
    }

private:
    void init_typedef() {
        type_alias_table["bool"] = TYPEID_INT;
        type_alias_table["byte"] = TYPEID_CHAR;
    }
    unordered_map<string, int> type_alias_table;
};