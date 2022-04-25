#include<bits/stdc++.h>

extern "C" {
    int loop(int a);
    int new_loop();
    int test_while();
    int test_do_while();
    void test_printf();
}

using namespace std;
int main() {
    cout << "Start testing" << endl;
    int jj = test_do_while();
    cout << jj << endl;

    test_printf();
}