#include <stdio.h>

const char* str1;
const char* str2;
char ch;
int i;
int getnumber() {
    int getnumber_result;
    getnumber_result = 42;
    return getnumber_result;
}


int main() {
    str1 = "Hello";
    ch = '!';
    printf("%s", str1);
    i = 2;
    switch (i) {
        case 1:
            printf("%s", "One");
            break;
        case 2:
            {
                printf("%s", "Two");
                break;
            }
            break;
        case 3:
            printf("%s", "Three");
            break;
    }
    for (i = 1; i <= 5; i++) {
        if ((i == 3)) {
            continue;
        }
        printf("%d", i);
    }
    return 0;
}
