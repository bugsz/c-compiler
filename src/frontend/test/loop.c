int main() {
    int i, a = 0;
    for (i = 0; i < 10; i = i + 1) {
        a = a + 1;
    }
    while(a < 20) {
        a = a + 1;
    }
    do {
        a = a + 1;
    } while(a < 30);
}