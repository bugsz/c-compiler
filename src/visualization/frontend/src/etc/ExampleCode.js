export const Examples = [
    {
        id: 1,
        name: 'Introduction',
        code: 
`/*
A "builtin.h" header file will be added to 
your  code automatically, so the following 
functions and defines are always availiable.
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
        name: 'Manachar Algorithm',
        code: `int min(int a, int b) {
    if(a < b) return a;
    return b;
}

int countSubstrings(char* s) {
    int n = strlen(s);
    char* t = malloc(sizeof(char) * (n * 2 + 4));
    t[0] = '$', t[1] = '#';
    for (int i = 0; i < n; i++) {
        t[2 * i + 2] = s[i];
        t[2 * i + 3] = '#';
    }
    t[2 * n + 2] = '!';
    t[2 * n + 3] = '\\0';
    n = 2 * n + 3;

    int f[n];
    memset(f, 0, n*sizeof(int));
    int iMax = 0, rMax = 0, ans = 0;
    for (int i = 1; i < n; ++i) {
        // 初始化 f[i]
        if(i <= rMax)
            f[i] = min(rMax - i + 1, f[2 * iMax - i]);
        else
            f[i] = 1;
        // 中心拓展
        while (t[i + f[i]] == t[i - f[i]]) ++f[i];
        // 动态维护 iMax 和 rMax
        if (i + f[i] - 1 > rMax) {
            iMax = i;
            rMax = i + f[i] - 1;
        }
        // 统计答案, 当前贡献为 (f[i] - 1) / 2 上取整
        ans += (f[i] / 2);
    }
    free(t);
    return ans;
}

int main(){
    printf("%d\\n", countSubstrings("cabcbabcbabcba"));
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
        name: 'Variadic Function',
        code: `#include "arg.h"
#include "math.h"
double stddev(int count, ...) 
{
    double sum = 0;
    double sum_sq = 0;
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; ++i) {
        double num = va_arg(args, double);
        sum += num;
        sum_sq += num*num;
    }
    va_end(args);
    return sqrt(sum_sq/count - (sum/count)*(sum/count));
}
 
int main(void) {
    printf("%f\\n", stddev(4, 25.0, 27.3, 26.9, 25.7));
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
    printf("%lf %d %d %d %d\\n", a, b, c, d, e);
    e++;
    d++;
    printf("%lf %d %d %d %d\\n", a, b, c, d, e);
    return 0;
}`
    },
    {
        id: 11,
        name: 'Array',
        code: `#define ROW 5
#define COL 5
int main(){
    int arr[ROW][COL];
    int val = 1;
    int Row = ROW;
    int Col = COL;
    int num = 1;
    int row = 0;
    int col = -1;
    while (val <= ROW * COL){
        for (int i = 0; i < Col; ++i){
            col += num;
            arr[row][col] = val++;
        }
        Col--;
        Row--;
        for (int i = 0; i < Row; ++i){
            row += num;
            arr[row][col] = val++;
        }		
        num = -num;
    }

    for (int i = 0; i < ROW; ++i){
        for (int j = 0; j < COL; ++j){
            printf("%4d\\t", arr[i][j]);
        }
        printf("\\n");
    }
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