#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    INVALID_BASE,
    INVALID_VALUE
} status;

#define EPS 1e-12
#define MAX_STEPS 100

static status validate_base(int base)
{
    if (base < 2 || base > 36) return INVALID_BASE;
    return OK;
}

static int gcd_int(long long a, long long b)
{
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        long long t = b;
        b = a % b;
        a = t;
    }
    return (int)a;
}

static int has_finite_representation(double x, int base)
{
    if (x <= 0.0 || x >= 1.0) return 0;
    if (base < 2 || base > 36) return 0;

    double frac = x;
    for (int step = 0; step < MAX_STEPS; step++) {
        frac *= (double)base;
        double int_part = floor(frac);
        frac -= int_part;
        if (fabs(frac) < EPS) return 1;
    }
    return 0;
}

static status check_fractions(int base, size_t count, ...)
{
    status st = validate_base(base);
    if (st != OK) return st;
    if (count == 0) return INVALID_INPUT;

    va_list ap;
    va_start(ap, count);

    for (size_t i = 0; i < count; i++) {
        double x = va_arg(ap, double);
        if (x <= 0.0 || x >= 1.0) {
            printf("x=%.17g: invalid (must be in (0;1))\n", x);
            va_end(ap);
            return INVALID_VALUE;
        }

        int finite = has_finite_representation(x, base);
        printf("x=%.17g, base=%d: %s\n",
               x, base, finite ? "finite" : "infinite");
    }

    va_end(ap);
    return OK;
}

static void print_status(status st)
{
    switch (st) {
        case INVALID_INPUT:  printf("Error: invalid input\n"); break;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); break;
        case INVALID_BASE:   printf("Error: base must be in [2..36]\n"); break;
        case INVALID_VALUE:  printf("Error: value must be in (0;1)\n"); break;
        default: break;
    }
}

int main(void)
{
    status st;

    printf("--- base 10 ---\n");
    st = check_fractions(10, 5, 0.5, 0.25, 0.1, 0.3, 1.0/3.0);
    if (st != OK) print_status(st);

    printf("\n--- base 2 ---\n");
    st = check_fractions(2, 4, 0.5, 0.25, 0.1, 1.0/3.0);
    if (st != OK) print_status(st);

    printf("\n--- base 3 ---\n");
    st = check_fractions(3, 3, 0.5, 1.0/3.0, 1.0/9.0);
    if (st != OK) print_status(st);

    printf("\n--- base 16 ---\n");
    st = check_fractions(16, 3, 0.5, 0.0625, 0.1);
    if (st != OK) print_status(st);

    printf("\n--- invalid base ---\n");
    st = check_fractions(1, 1, 0.5);
    if (st != OK) print_status(st);

    printf("\n--- invalid value ---\n");
    st = check_fractions(10, 1, 1.5);
    if (st != OK) print_status(st);

    return OK;
}
