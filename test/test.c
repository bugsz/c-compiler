int tmp;

int fib(int n) {
    if (n < 2) return 1;
    return fib(n-1) + fib(n-2);
}

int test_while() {
    int i=0;
    int a=0;
    while(i < 10) {
        i = i + 1;
        a = a + i;
    }
    return a;
}

int test_do_while() {
    int i=0;
    int a=0;
    do {
        a = a + 1;
    }while(i < 0);
    return a;
}

int test_for() {
    int i;
    tmp = 0;
    for(i=0;i<10;i=i+1) tmp = tmp + i;
    return tmp;
}

void test_string() {
    string s = "test string";
    __builtin_printf("output string: %s\n", s);
}

double test_double() {
    double d = 1.0;
    int a = 10;
    while(a > 0) {
        d = d + a;
        a = a - 1;
    }
    return d;
}


int main() {
    __builtin_printf("fib: %d\n", fib(10));
    __builtin_printf("test_while: %d\n", test_while());
    __builtin_printf("test_do_while: %d\n", test_do_while());
    __builtin_printf("test_for: %d\n", test_for());
    test_string();
    __builtin_printf("test_double: %lf\n", test_double());

    return 0;
}