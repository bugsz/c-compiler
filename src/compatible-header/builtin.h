#ifndef __BUILTIN_H
#define __BUILTIN_H

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

#endif /* __BUILTIN_H */