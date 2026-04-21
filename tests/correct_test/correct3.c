#include <stdio.h>

int ififelse() {
    int ififelse_result;
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
    ififelse_result = a;
    return ififelse_result;
}


int main() {
    printf("%d", ififelse());
    return 0;
}
