#include <stdio.h>

int a, b, c, d, e;
int flag;

int main() {
    a = 1;
    b = 0;
    c = 1;
    d = 2;
    e = 4;
    flag = 0;
    if ((((((a * b) / c) == (e + d)) && (((a * (a + b)) + c) <= (d + e))) || ((a - (b * c)) == (d - (a / c))))) {
        flag = 1;
    }
    if (flag) {
        printf("%d", 1);
    }
    return 0;
}
