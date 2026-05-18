#include <stdio.h>

const int a = 1;
int arr[2];
int i;
int f(int x) {
    int f_result;
    f_result = x;
    return f_result;
}

void p() {
    printf("%s\n", "ok");
}


int main() {
    i = 1;
    if (!((i == 2))) {
        switch (i) {
            case 1:
                {
                    printf("%d", arr[1 - 1]);
                    break;
                }
                break;
            case 2:
                printf("%s", "two");
                break;
        }
    }
    for (i = 1; i <= 2; i++) {
        continue;
        scanf("%d", &arr[i - 1]);
    }
    scanf("%d", &i);
    p();
    return 0;
}
