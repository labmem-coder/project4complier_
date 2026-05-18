#include <stdio.h>

int arr[110];
int n;
int m, a, b, k, loc, i;
int flag;
void init(int n) {
    int i;
    for (i = 1; i <= ((n * n) + 1); i++) {
        arr[i] = -(1);
    }
}

int findfa(int a) {
    int findfa_result;
    if ((arr[a] == a)) {
        findfa_result = a;
    } else {
        arr[a] = findfa(arr[a]);
        findfa_result = arr[a];
    }
    return findfa_result;
}

void mmerge(int a, int b) {
    int m, n;
    m = findfa(a);
    n = findfa(b);
    if ((m != n)) {
        arr[m] = n;
    }
}


int main() {
    n = 4;
    m = 10;
    flag = 0;
    init(n);
    k = ((n * n) + 1);
    for (i = 0; i <= (m - 1); i++) {
        scanf("%d", &a);
        scanf("%d", &b);
        if ((flag == 0)) {
            loc = ((n * (a - 1)) + b);
            arr[loc] = loc;
            if ((a == 1)) {
                arr[0] = 0;
                mmerge(loc, 0);
            }
            if ((a == n)) {
                arr[k] = k;
                mmerge(loc, k);
            }
            if (((b < n) && (arr[(loc + 1)] != -(1)))) {
                mmerge(loc, (loc + 1));
            }
            if (((b > 1) && (arr[(loc - 1)] != -(1)))) {
                mmerge(loc, (loc - 1));
            }
            if (((a < n) && (arr[(loc + n)] != -(1)))) {
                mmerge(loc, (loc + n));
            }
            if (((a > 1) && (arr[(loc - n)] != -(1)))) {
                mmerge(loc, (loc - n));
            }
            if (((arr[0] != -(1)) && (arr[k] != -(1)) && (findfa(0) == findfa(k)))) {
                flag = 1;
                printf("%d", (i + 1));
            }
        }
    }
    if ((flag == 0)) {
        printf("%d", -(1));
    }
    return 0;
}
