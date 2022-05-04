export const Examples = [
    {
        id: 1,
        name: 'Basic',
        code: `int main() {
    __builtin_printf("hello world!");
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
    __builtin_printf("global b: %d\\n", b);
    b = 555;
    __builtin_printf("new_global b: %d\\n", b);
    __builtin_printf("a+c+d: %d\\n", a+c+d);
    a = -1;
    c = -1;
    d = -1;
}

int main(){
    __builtin_printf("global b: %d\\n", b);
    int d = 1000;
    __builtin_printf("%d\\n", d);
    int a = 2;
    int c = 1;
    __builtin_printf("a:%d c:%d d:%d\\n", a, c, d);
    // this is global b
    b = 888;
    // this is local b
    int b = 777;
    __builtin_printf("local b: %d\\n", b);
    foo(a, c, d);
    __builtin_printf("After call foo\\n");
    __builtin_printf("local b: %d\\n", b);
    __builtin_printf("a:%d c:%d d:%d\\n", a, c, d);
    return 0;
}
`
    },
    {
        id: 9,
        name: 'Type System',
        code: `int main() {
    bool a = false;
    byte b = 0o3;
    short c = 0x3;
    int d = 0b1011;
    long f = 0xcafebabe;

    float g = 0.5;
    double h = 0.25;

    return a;
}`
    },
    {
        id: 10,
        name: 'Cast Expression',
        code: `int cast() {

    double a = 10.5;

    int b = (int) a;

    return b;
}`
    },
    {
        id: 11,
        name: 'String',
        code: `int main() {
    string s = "String Test";

    __builtin_printf("%s", s);

    return 0;
}`
    },
    {
        id: 12,
        name: 'xterm-256color',
        code: `int main() {
    int i;
    int j;
    for(i = 0; i < 16; i=i+1){
        for(j = 0; j < 16; j=j+1){
            char buf[100];
            int code = i*16+j;
            __builtin_sprintf(buf, "\\u001b[48;5;%dm%-4d", code, code);
            __builtin_printf("%s", buf);
        }
        __builtin_printf("\\u001b[0m\\n");
    }
    return 0;
}`
    }

]