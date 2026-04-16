#include <stdio.h>

int a;
int defn() {
    int defn_result;
    defn_result = 4;
    return defn_result;
}


int main() {
    a = defn();
    printf("%d", a);
    return 0;
}
