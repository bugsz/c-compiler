int a;
int d = 3;

int positive(int x) {
    int res;
    if (x > 0) {
        res = 1;
    } else {
        res = 0;
    }
    return res;
}

int main() {
    int a;
    int b = 5;
    int c;
    a = 3;
    c = a + b;
    b = positive(c);
}