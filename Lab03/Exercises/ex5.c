#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <float.h>

void* vaggregate(size_t size, int n, void* initial_value, void* (*opr)(const void*, const void*), ...) {
    va_list ap;
    void* acc = malloc(size);
    if (!acc) exit(1);
    memcpy(acc, initial_value, size);
    va_start(ap, opr);
    for (int i = 0; i < n; ++i) {
        void* elem = va_arg(ap, void*);
        void* next = opr(acc, elem);
        free(acc);
        acc = next;
    }
    va_end(ap);
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
    int a=1,b=2,c=3,d=4,e=5;
    double p=1.5,q=-2.0,r=3.25,s=4.0,t=5.5;

    int i0=0,i1=1,imin=INT_MIN;
    double d0=0.0,d1=1.0,dmin=-DBL_MAX;

    void* ri_add = vaggregate(sizeof(int), 5, &i0, add_int, &a,&b,&c,&d,&e);
    void* ri_mul = vaggregate(sizeof(int), 5, &i1, mul_int, &a,&b,&c,&d,&e);
    void* ri_max = vaggregate(sizeof(int), 5, &imin, max_int, &a,&b,&c,&d,&e);

    void* rd_add = vaggregate(sizeof(double), 5, &d0, add_double, &p,&q,&r,&s,&t);
    void* rd_mul = vaggregate(sizeof(double), 5, &d1, mul_double, &p,&q,&r,&s,&t);
    void* rd_max = vaggregate(sizeof(double), 5, &dmin, max_double, &p,&q,&r,&s,&t);

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

