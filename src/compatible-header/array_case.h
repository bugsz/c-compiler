int a[100];
int b[100]={999};
int c[100]={91,99};

int test1(){
    // a[0] = 9999; // fine
    *a = 9999;// fail
    *(a+1) = 9999; // fail
}

int test2(){
    int la[100];
    // *(a+1) = 99; // fine
    // a[2] = 999; // fine
    *la = 9; // run time error
}

int test3(){
    int a[100]; // fail
    int b[100]={999};// fail
    int c[100]={91,99};// fail
}
