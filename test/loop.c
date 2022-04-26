int tmp = 123;
int loop(int a) {
    int i;
    for (i = 0; i < 10; i = i + 1) {
        a = a + 1;
    }
    // a = a + 1;
    return a;
}

int new_loop() {
    int j=1;
    return loop(j+1) * (j+1);
}

int test_while() {
    int i=0;
    int a=0;
    while(i < 10) {
        i = i + 1;
        a = a + 1;
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

void test_printf() {
    __builtin_printf("hello %d\n", 1);
}

void test_sum() {
    int i;
    tmp = 0;
    for(i=0;i<10;i=i+1) tmp = tmp + i;

    __builtin_printf("sum: %d\n", tmp);
}

int test_global() {
    int a=1;
    tmp = 2;
    __builtin_printf("Global tmp: %d, a: %d\n", tmp, a);
    return 1;
}

int test_fib(int n) {
    if (n == 0) return 1;
    if (n == 1) return 1;
    return test_fib(n-1) + test_fib(n-2);
}

int main() {
    tmp = 2;
    test_sum();
    __builtin_printf("fib: %d\n", test_fib(10));
    
    // test_printf();
    // string ss = "test global";
    // __builtin_printf("test global: %d", test_global());
    return 0;
}