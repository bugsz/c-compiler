export const Examples = [
    {
        id: 1,
        name: 'Introduction',
        code: 
`/*
A "builtin.h" header file will be added to your code
automatically, so the following functions and defines
are always availiable.
#define NULL 0
int printf(char *format, ...);
int sprintf(char *buf, char *format, ...);
int scanf(char *format,...);
int strlen(char *str,...);
char *strcpy(char *destin, char *source);
char *strncpy(char *dest, char *src, int n);
char *strdup(char *s);
char *strcat(char *str1, char *str2);
char *strset(char *str, char ch);
char *strnset(char *str, char ch, int n);
int strcmp(char *str1, char *str2);
void *memset(void *str, int c, int n);
int memcmp(void *str1, void *str2, int n);
void *memcpy(void *dest, void *src, int n);
int putchar(char ch);
int puts(char* s);
int getchar();
int gets(char* buf);
void *malloc(int size);
void free(void* p);
void exit(int status);
*/
int main() {
    printf("hello world!");
    return 0;
}`
    },
    {
        id: 2,
        name: 'Unary',
        code: `int neg_unary() {
    int a = -2;
    return -a;
}

int not_unary() {
    int a = 1;
    return ~a;
}

int complement_unary() {
    int b = !0;
    int a = !23;
    return !b;
}
`
    },
    {
        id: 3,
        name: 'Binary',
        code: `int add() {
    int a = 2 + 2;
    return a;
}

int mul() {
    int a = 1 + 3 * 4 - 2;
    return a + 1;
}

int div() {
    int b = 2;
    int a = 3 / b;
    return b;
}`
    },
    {
        id: 4,
        name: 'If Condition',
        code: `int main() {
    int a = 0;
    int b = 1;

    if (a > b) {
        return 1;
    } else if (a < b) {
        return 2;
    } else {
        return 3;
    }
}`
    },
    {
        id: 5,
        name: 'For Loop',
        code: `int main() {
    int b = 255;

    for (int i = 0; i < b; i += 1) {
        b -= 1;
    }
    return 0;
}
`
    },
    {
        id: 6,
        name: 'While Loop',
        code: `int main() {
    int a = 0;
    int b = 30;
    int c = 10;

    do {
        c = b - c;
    } while (c >= 0);

    while (a < b) {
        a += 1;
        b -= 1;

        if (b == 3)
            break;
    }

    return 0;
}`
    },
    {
        id: 7,
        name: 'Functions',
        code: `
int foo(int a, int b) {
    return a + b;
}

int main() {
    return foo(0, 1);
}

int bar(int a, int b, int c) {
    int rst = 0;
    while (a < b) {
        rst <<= c;
        a += 1;
    }
    return rst;
}
`
    },
    {
        id: 8,
        name: 'Global Variables',
        code: `int b = 6666;
void foo(int a, int c, int d){
    printf("global b: %d\\n", b);
    b = 555;
    printf("new_global b: %d\\n", b);
    printf("a+c+d: %d\\n", a+c+d);
    a = -1;
    c = -1;
    d = -1;
}

int main(){
    printf("global b: %d\\n", b);
    int d = 1000;
    printf("%d\\n", d);
    int a = 2;
    int c = 1;
    printf("a:%d c:%d d:%d\\n", a, c, d);
    // this is global b
    b = 888;
    // this is local b
    int b = 777;
    printf("local b: %d\\n", b);
    foo(a, c, d);
    printf("After call foo\\n");
    printf("local b: %d\\n", b);
    printf("a:%d c:%d d:%d\\n", a, c, d);
    return 0;
}
`
    },
    {
        id: 9,
        name: 'Type System',
        code: `int main() {
    char c;
    short d;
    int e;
    long f;
    float g;
    double h;
    return 0;
}`
    },
    {
        id: 10,
        name: 'Cast Expression',
        code: `int main() {
    double a = 10.5;
    int b = (int) a;
    int c = a;
    int d = 255;
    char e = d;
    printf("%d %d %d\\n", a, b, c, d, e);
    d = d+1;
    printf("%d %d %d\\n", a, b, c, d, e);
    return 0;
}`
    },
    {
        id: 11,
        name: 'String',
        code: `int main() {
    string s = "String Test";

    printf("%s", s);

    return 0;
}`
    },
    {
        id: 12,
        name: 'xterm-256color',
        code: `int main() {
    int i;
    int j;
    char buf[100];
    for(i = 0; i < 16; i=i+1){
        for(j = 0; j < 16; j=j+1){
            int code = i*16+j;
            sprintf(buf, "\\u001b[48;5;%dm%-4d", code, code);
            printf("%s", buf);
        }
        printf("\\u001b[0m\\n");
    }
    return 0;
}`
    }

]