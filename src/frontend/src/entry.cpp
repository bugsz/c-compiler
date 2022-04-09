#include <stdio.h>

extern "C" {
    extern void yyparse();
}

int main() {
    printf("Hello world\n");
    yyparse();
}