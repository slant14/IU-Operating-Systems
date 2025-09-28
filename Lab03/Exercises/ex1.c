#include "stdio.h"
#include <stdlib.h>

int const_tri(const int n, int * p) {
    if (n < 0) return 0;
    if (n == 0) return 0;
    if (n == 1 || n == 2) return 1;

    int temp = 0;
    for (int i = 3; i <= n; ++i) {
        *(p + 2) = temp + *p + *(p + 1);
        temp = *p;
        *p = *(p + 1);
        *(p + 1) = *(p + 2);
    }
    return *(p + 2);
}

int main() {
    int n;
    const int x = 1;
    const int * q = &x;
    int * const p = malloc(3);
    *p = x; *(p + 1) = x; *(p + 2) = 2 * x;

    printf("Address is: %p & The value is: %d\n", (void *)p, *p);
    printf("Address is: %p & The value is: %d\n", (void *)(p + 1), *(p + 1));
    printf("Address is: %p & The value is: %d\n", (void *)(p + 2), *(p + 2));

    printf("Enter the value to find the tribonacci sequence( < 37): ");
    scanf("%d", &n);

    printf("%d\n", const_tri(n, p));

    return 0;
}
