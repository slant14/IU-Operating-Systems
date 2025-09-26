#include <stdio.h>
#include <math.h>

struct Point { double x, y; };

static double area(struct Point a, struct Point b, struct Point c) {
    double d2 = a.x * (b.y - c.y)
              + b.x * (c.y - a.y)
              + c.x * (a.y - b.y);
    return fabs(d2) * 0.5;
}

int main(void) {
    struct Point A, B, C;

    printf("Ax: "); if (scanf("%lf", &A.x) != 1) return 1;
    printf("Ay: "); if (scanf("%lf", &A.y) != 1) return 1;
    printf("A = (%.6f, %.6f)\n", A.x, A.y);

    printf("Bx: "); if (scanf("%lf", &B.x) != 1) return 1;
    printf("By: "); if (scanf("%lf", &B.y) != 1) return 1;
    printf("B = (%.6f, %.6f)\n", B.x, B.y);

    printf("Cx: "); if (scanf("%lf", &C.x) != 1) return 1;
    printf("Cy: "); if (scanf("%lf", &C.y) != 1) return 1;
    printf("C = (%.6f, %.6f)\n", C.x, C.y);

    printf("Area: %.6f\n", area(A, B, C));
    return 0;
}

