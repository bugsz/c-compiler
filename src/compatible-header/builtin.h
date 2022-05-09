#define NULL 0
int printf(char *format, ...);
int sprintf(char *buf, char *format, ...);
int scanf(char *format,...);
int strlen(char *str,...);
char *strcpy(char *destin, char *source);
char *strncpy(char *dest, char *src, int n);
char * strcat(char *str1, char *str2);
int strcmp(char *str1, char *str2);
int putchar(int ch);
int puts(char* s);
int puti(int n);
int getchar();
int gets(char* buf);
void *malloc(int size);
void free(void* p);
void exit(int status);
void assert(int exp);
double sqrt(double num);
int execl(char *path, char *arg, ...);
int execlp(char *path, char *arg, ...);