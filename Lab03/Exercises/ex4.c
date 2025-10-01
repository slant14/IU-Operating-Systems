#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

void* aggregate(void* base, size_t size, int n, void* initial_value, void* (*opr)(const void*, const void*)) {
    void* acc = malloc(size);
    if (!acc) exit(1);
    memcpy(acc, initial_value, size);
    for (int i = 0; i < n; ++i) {
        void* elem = (char*)base + (size_t)i * size;
        void* next = opr(acc, elem);
        free(acc);
        acc = next;
    }
    return acc;
}

void* add_int(const void* a, const void* b) {
    int* r = malloc(sizeof(int));
    *r = *(const int*)a + *(const int*)b;
    return r;
}
void* mul_int(const void* a, const void* b) {
    int* r = malloc(sizeof(int));
    *r = *(const int*)a * *(const int*)b;
    return r;
}
void* max_int(const void* a, const void* b) {
    int* r = malloc(sizeof(int));
    int x = *(const int*)a, y = *(const int*)b;
    *r = x > y ? x : y;
    return r;
}

void* add_double(const void* a, const void* b) {
    double* r = malloc(sizeof(double));
    *r = *(const double*)a + *(const double*)b;
    return r;
}
void* mul_double(const void* a, const void* b) {
    double* r = malloc(sizeof(double));
    *r = *(const double*)a * *(const double*)b;
    return r;
}
void* max_double(const void* a, const void* b) {
    double* r = malloc(sizeof(double));
    double x = *(const double*)a, y = *(const double*)b;
    *r = x > y ? x : y;
    return r;
}

int main(void) {
    int ai[5] = {1, 2, 3, 4, 5};
    double ad[5] = {1.5, -2.0, 3.25, 4.0, 5.5};

    int i0 = 0, i1 = 1, imin = INT_MIN;
    double d0 = 0.0, d1 = 1.0, dmin = -DBL_MAX;

    void* ri_add = aggregate(ai, sizeof(int), 5, &i0, add_int);
    void* ri_mul = aggregate(ai, sizeof(int), 5, &i1, mul_int);
    void* ri_max = aggregate(ai, sizeof(int), 5, &imin, max_int);

    void* rd_add = aggregate(ad, sizeof(double), 5, &d0, add_double);
    void* rd_mul = aggregate(ad, sizeof(double), 5, &d1, mul_double);
    void* rd_max = aggregate(ad, sizeof(double), 5, &dmin, max_double);

    printf("int sum: %d\n", *(int*)ri_add);
    printf("int product: %d\n", *(int*)ri_mul);
    printf("int max: %d\n", *(int*)ri_max);

    printf("double sum: %.6g\n", *(double*)rd_add);
    printf("double product: %.6g\n", *(double*)rd_mul);
    printf("double max: %.6g\n", *(double*)rd_max);

    free(ri_add); free(ri_mul); free(ri_max);
    free(rd_add); free(rd_mul); free(rd_max);
    return 0;
}

