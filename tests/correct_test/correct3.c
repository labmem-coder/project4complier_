#include <stdio.h>

int ififElse() {
    int ififElse_result;
    int a;
    int b;
    a = 5;
    b = 10;
    if ((a == 5)) {
        if ((b == 10)) {
            a = 25;
        } else {
            a = (a + 15);
        }
    }
    ififElse_result = a;
    return ififElse_result;
}


int main() {
    printf("%d", ififElse());
    return 0;
}
