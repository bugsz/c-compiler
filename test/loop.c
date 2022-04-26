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
    __builtin_printf("hello %d %s", 1, "world");
}

int main() {
    test_printf();
    return 0;
}