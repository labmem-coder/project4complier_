#include <stdio.h>

int a;
int b;
int func(int p) {
    int func_result;
    p = (p - 1);
    func_result = p;
    return func_result;
}


int main() {
    a = 10;
    b = func(a);
    printf("%d", b);
    return 0;
}
