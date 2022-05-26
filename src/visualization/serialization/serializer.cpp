#include <frontend.h>
#include <fmt/core.h>


using namespace std;
extern char* typeid_deref[];
static string print_node(ast_node_ptr node, bool pretty = false) {
    static int count = 0;
    static int tabs = 0;
    if (node == NULL)
        return "";
    char position[MAX_TOKEN_LEN] = { 0 }, type[MAX_TOKEN_LEN] = { 0 };
    if (node->pos.last_line * node->pos.last_column) {
        sprintf(position, "<%d:%d>", node->pos.last_line, node->pos.last_column);
    }
    if (strcmp(node->token, "TranslationUnitDecl") == 0 ||
        strcmp(node->token, "DeclStmt") == 0 ||
        strcmp(node->token, "ForStmt") == 0 ||
        strcmp(node->token, "IfStmt") == 0 ||
        strcmp(node->token, "WhileStmt") == 0 ||
        strcmp(node->token, "DoStmt") == 0 ||
        strcmp(node->token, "ForDelimiter") == 0 ||
        strcmp(node->token, "VariadicParms") == 0 ||
        strcmp(node->token, "NullStmt") == 0 ||
        strcmp(node->token, "InitializerList") == 0) {
    } else {
        sprintf(type, "'%s'", typeid_deref[node->type_id]);
    }
    string children = "";
    tabs += 2;
    for(int i = 0; i < node->n_child; i++){
        children += (i > 0 ? (pretty ? ",\n":",") : "") + print_node(node->child[i], pretty);
    }
    tabs -= 2;
    string whitespace(4*tabs, ' ');
    string whitespaceplus(4*tabs+4, ' ');
    string val = node->val;
    std::stringstream ss;
    ss << std::quoted(val);
    val = ss.str();
    if(pretty)
        return fmt::format("{}{{ \
                            \n{}\"id\": \"{}\", \
                            \n{}\"label\": \"{}\", \
                            \n{}\"position\": \"{}\", \
                            \n{}\"value\": {},  \
                            \n{}\"ctype\": \"{}\", \
                            \n{}\"type\": \"circle\", \
                            \n{}\"children\": [\n{}{}\n{}] \
                            \n{}}}", 
                            whitespace,
                            whitespaceplus, fmt::format("node-{}", count++), 
                            whitespaceplus, node->token, 
                            whitespaceplus, position, 
                            whitespaceplus, val, 
                            whitespaceplus, type,
                            whitespaceplus, 
                            whitespaceplus, children, whitespaceplus, whitespaceplus,
                            whitespace
                            );
    else
        return fmt::format("{{\"id\":\"{}\",\"label\":\"{}\",\"position\":\"{}\",\"val\":{},\"ctype\":\"{}\",\"type\": \"circle\",\"children\":[{}]}}", 
                            fmt::format("node-{}", count++), 
                            node->token, 
                            position, 
                            val, 
                            type,
                            children
                            );
}


int main(int argc, char const *argv[])
{
    lib_frontend_ret ret = frontend_entry(argc, argv);
    string s = print_node(ret.root);
    cout<<s;
    return 0;
}
