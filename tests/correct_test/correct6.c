#include <stdio.h>

int ret;
int ifwhile() {
    int ifwhile_result;
    int a, b;
    a = 0;
    b = 1;
    if ((a == 5)) {
        for (b = 1; b <= 3; b++) {
        }
        b = (b + 25);
        ifwhile_result = b;
    } else {
        for (a = 0; a <= 4; a++) {
            b = (b * 2);
        }
    }
    ifwhile_result = b;
    return ifwhile_result;
}


int main() {
    printf("%d", ifwhile());
    return 0;
}
