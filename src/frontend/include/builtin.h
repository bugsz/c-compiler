/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-17 00:13:31
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/include/builtin.h
 * @Description: 
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    char* builtin_itoa(long n, int base);
    long builtin_atoi(const char* str, int base);
    int builtin_strlen(const char* str);
    const char* builtin_strcat(const char* str1, const char* str2);
    char builtin_strget(const char* str, int index);

#ifdef __cplusplus
}
#endif