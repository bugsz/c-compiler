int a, d = 3, e = 5;

void positive(int x) {
    int res;
    if (x > 0) {
        res = 1;
    } else {
        res = 0;
    }
    puts(res);
}

int main() {
    int a;
    int b = 5;
    int c;
    a = 3;
    c = a + b;
    positive(c);
}