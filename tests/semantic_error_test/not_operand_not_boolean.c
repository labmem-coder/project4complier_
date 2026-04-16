#include <stdio.h>

int x;

int main() {
    x = 1;
    if (~(x)) {
        printf("%d", x);
    }
    return 0;
}
