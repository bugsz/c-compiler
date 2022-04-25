#include <frontend.h>
#include <fmt/core.h>
#include <bits/stdc++.h>

using namespace std;

static const char* typeid_deref[] = {
    "void", "char", "short", "int", "long", "float", "double", "string"
};

static string print_node(ast_node_ptr node) {
    static int tabs = 0;
    if (node == NULL)
        return "";
    char position[MAX_TOKEN_LEN] = { 0 }, type[MAX_TOKEN_LEN] = { 0 };
    if (node->pos.last_line * node->pos.last_column) {
        sprintf(position, "<%d:%d>", node->pos.last_line, node->pos.last_column);
    }
    if (strcmp(node->token, "TranslationUnit") == 0 ||
        strcmp(node->token, "CompoundStmt") == 0 ||
        strcmp(node->token, "DeclStmt") == 0 ||
        strcmp(node->token, "ForStmt") == 0 ||
        strcmp(node->token, "IfStmt") == 0 ||
        strcmp(node->token, "WhileStmt") == 0 ||
        strcmp(node->token, "DoStmt") == 0) {
    } else {
        sprintf(type, "%s", typeid_deref[node->type_id]);
    }
    string children = "";
    tabs += 2;
    for(int i = 0; i < node->n_child; i++){
        children += (i > 0 ? ",\n" : "") + print_node(node->child[i]);
    }
    tabs -= 2;
    string whitespace(4*tabs, ' ');
    string whitespaceplus(4*tabs+4, ' ');
    return fmt::format("{}{{ \
                        \n{}\"token\": \"{}\", \
                        \n{}\"position\": \"{}\", \
                        \n{}\"val\": \"{}\",  \
                        \n{}\"type\": \"{}\", \
                        \n{}\"children\": [\n{}{}\n{}] \
                        \n{}}}", 
                        whitespace,
                        whitespaceplus, node->token, 
                        whitespaceplus, position, 
                        whitespaceplus, node->val, 
                        whitespaceplus, type, 
                        whitespaceplus, children, whitespaceplus, whitespaceplus,
                        whitespace
                        );
}


int main(int argc, char const *argv[])
{
    lib_frontend_ret ret = frontend_entry(argc, argv);
    string s = print_node(ret.root);
    cout<<s;
    return 0;
}
