#include <stdio.h>

int i;

int main() {
    i = 0;
    while ((i < 3)) {
        i = (i + 1);
    }
    printf("%d", i);
    return 0;
}
