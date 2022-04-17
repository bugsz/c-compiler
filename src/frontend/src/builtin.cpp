/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-17 00:13:39
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/src/builtin.cpp
 * @Description: 
 */
#include "builtin.h"
#include <string>
#include <cstdlib>
#include <assert.h>

using namespace std;

enum radix_type {
    BIN = 2,
    OCT = 8,
    DEC = 10,
    HEX = 16,
};

#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i)    \
    (((i) & 0x80ll) ? '1' : '0'), \
    (((i) & 0x40ll) ? '1' : '0'), \
    (((i) & 0x20ll) ? '1' : '0'), \
    (((i) & 0x10ll) ? '1' : '0'), \
    (((i) & 0x08ll) ? '1' : '0'), \
    (((i) & 0x04ll) ? '1' : '0'), \
    (((i) & 0x02ll) ? '1' : '0'), \
    (((i) & 0x01ll) ? '1' : '0')
 
#define PRINTF_BINARY_PATTERN_INT16 \
    PRINTF_BINARY_PATTERN_INT8              PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) \
    PRINTF_BYTE_TO_BINARY_INT8((i) >> 8),   PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 \
    PRINTF_BINARY_PATTERN_INT16             PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) \
    PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
#define PRINTF_BINARY_PATTERN_INT64    \
    PRINTF_BINARY_PATTERN_INT32             PRINTF_BINARY_PATTERN_INT32
#define PRINTF_BYTE_TO_BINARY_INT64(i) \
    PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)

const char* builtin_itoa(int n, int base) {
    assert(sizeof(int) == 4);
    char* buf = new char[40];
    if (base == BIN) {
        char* temp = new char[35];
        sprintf(temp, PRINTF_BINARY_PATTERN_INT32, PRINTF_BYTE_TO_BINARY_INT32(n));
        sprintf(buf, "\"0b%d\"", stoi(temp));
        delete [] temp;
    } else if (base == OCT) {
        sprintf(buf, "\"0%o\"", n);
    } else if (base == DEC) {
        sprintf(buf, "\"%d\"", n);
    } else if (base == HEX) {
        sprintf(buf, "\"0x%X\"", n);
    } else {
        assert(false);
    }
    return buf;
}

int builtin_atoi(const char* str) {
    return stoi(str);
}

const char* builtin_strcat(const char* str1, const char* str2) {
    string s1(str1), s2(str2);
    if(s1.find("\"") != string::npos) {
        s1.erase(s1.find("\""), 1);
        s1.erase(s1.find("\""), 1);
    }
    if(s2.find("\"") != string::npos) {
        s2.erase(s2.find("\""), 1);
        s2.erase(s2.find("\""), 1);
    }
    string ret("\"" + s1 + s2 + "\"");
    assert(ret.length() < 128);
    return strdup(ret.c_str());
}

int builtin_strlen(const char* str) {
    if(string(str).find("\"") == 0) {
        return string(str).length() - 2;
    } else {
        return string(str).length();
    }
}

char builtin_strget(const char* str, int index) {
    if(string(str).find("\"") == 0) {
        return string(str).at(index + 1);
    } else {
        return string(str).at(index);
    }
}