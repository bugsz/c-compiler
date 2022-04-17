/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-17 14:06:52
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/test/builtin_functions.c
 * @Description: 
 */
int main() {
    string a = "hello";
    string b = __builtin_strcat(a, " world");
    int len = __builtin_strlen(b);
    char ch = __builtin_strget(b, 0);
    int i = __builtin_itoa("123", 16);
}