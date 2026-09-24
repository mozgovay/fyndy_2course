#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    ERR_OVERFLOW,
    NO_CONVERGENCE
} status;

status validate_epsilon(const char *s, double *out)
{
    if (s == NULL || *s == '\0') return INVALID_INPUT;
    if (*s == '-') return INVALID_INPUT;

    char *end = NULL;
    double value = strtod(s, &end);

    if (value == HUGE_VAL || value == -HUGE_VAL) return ERR_OVERFLOW;
    if (*end != '\0') return INVALID_INPUT;
    if (value <= 0.0 || value >= 1.0) return INVALID_INPUT;

    *out = value;
    return OK;
}

status validate_double(const char *s, double *out)
{
    if (s == NULL || *s == '\0') return INVALID_INPUT;

    char *end = NULL;
    double value = strtod(s, &end);

    if (value == HUGE_VAL || value == -HUGE_VAL) return ERR_OVERFLOW;
    if (*end != '\0') return INVALID_INPUT;

    *out = value;
    return OK;
}

status sum_a(double eps, double x, double *out)
{
    double current = 0.0, term = 1.0;
    int n = 0;
    int iter = 0;

    while (fabs(term) >= eps) {
        current += term;
        n++;
        term *= x / n;
        iter++;
        if (iter > 100000) return NO_CONVERGENCE;
    }
    *out = current;
    return OK;
}

status sum_b(double eps, double x, double *out)
{
    double current = 0.0, term = 1.0;
    int n = 0;
    int iter = 0;

    while (fabs(term) >= eps) {
        current += term;
        n++;
        term *= -x * x / ((2.0 * n - 1.0) * (2.0 * n));
        iter++;
        if (iter > 100000) return NO_CONVERGENCE;
    }
    *out = current;
    return OK;
}

status sum_c(double eps, double x, double *out)
{
    double current = 1.0, term = 1.0;
    int n = 1;
    int iter = 0;

    while (fabs(term) >= eps) {
        term *= 27.0 * n * n * n * x * x /
                ((3.0 * n - 2.0) * (3.0 * n - 1.0) * (3.0 * n));
        current += term;
        n++;
        iter++;
        if (iter > 100000) return NO_CONVERGENCE;
    }
    *out = current;
    return OK;
}

status sum_d(double eps, double x, double *out)
{
    double current = 0.0, term = x * x;
    int n = 1;
    int iter = 0;

    while (fabs(term) >= eps) {
        current += term;
        n++;
        term *= -x * x * (2.0 * n - 1.0) / (2.0 * n);
        iter++;
        if (iter > 100000) return NO_CONVERGENCE;
    }
    *out = current;
    return OK;
}

double f_a(double x)
{
    if (fabs(x) < 1e-15) return 1.0;
    return log(1.0 + x) / x;
}

double f_b(double x)
{
    return exp(-x * x / 2.0);
}

double f_c(double x)
{
    if (fabs(1.0 - x) < 1e-15) return 0.0;
    return log(1.0 / (1.0 - x));
}

double f_d(double x)
{
    if (fabs(x) < 1e-15) return 1.0;
    return pow(x, x);
}

status integral_rect(double eps, double (*f)(double), double *out)
{
    int n = 1;
    double prev = 0.0, curr = 0.0;
    int iter = 0;

    do {
        prev = curr;
        double h = 1.0 / n;
        curr = 0.0;
        for (int i = 1; i <= n; i++) {
            double x = (i - 0.5) * h;
            curr += f(x);
        }
        curr *= h;
        n *= 2;
        iter++;
        if (iter > 25) return NO_CONVERGENCE;
    } while (fabs(curr - prev) >= eps);

    *out = curr;
    return OK;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Error: expected 2 arguments: <epsilon> <x>\n");
        return INVALID_INPUT;
    }

    double eps = 0.0;
    status st = validate_epsilon(argv[1], &eps);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: epsilon must be in (0, 1)\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
        case NO_CONVERGENCE: printf("Error: no convergence\n"); return st;
    }

    double x = 0.0;
    st = validate_double(argv[2], &x);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid x\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
        case NO_CONVERGENCE: printf("Error: no convergence\n"); return st;
    }

    double a, b, c, d;

    st = sum_a(eps, x, &a);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
        case NO_CONVERGENCE: printf("Error: sum a did not converge\n"); return st;
    }
    st = sum_b(eps, x, &b);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
        case NO_CONVERGENCE: printf("Error: sum b did not converge\n"); return st;
    }
    st = sum_c(eps, x, &c);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
        case NO_CONVERGENCE: printf("Error: sum c did not converge\n"); return st;
    }
    st = sum_d(eps, x, &d);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
        case NO_CONVERGENCE: printf("Error: sum d did not converge\n"); return st;
    }
    printf("Sums:\n");
    printf("a: %f\n", a);
    printf("b: %f\n", b);
    printf("c: %f\n", c);
    printf("d: %f\n", d);

    st = integral_rect(eps, f_a, &a);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
        case NO_CONVERGENCE: printf("Error: integral a did not converge\n"); return st;
    }
    st = integral_rect(eps, f_b, &b);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
        case NO_CONVERGENCE: printf("Error: integral b did not converge\n"); return st;
    }
    st = integral_rect(eps, f_c, &c);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
        case NO_CONVERGENCE: printf("Error: integral c did not converge\n"); return st;
    }
    st = integral_rect(eps, f_d, &d);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
        case NO_CONVERGENCE: printf("Error: integral d did not converge\n"); return st;
    }
    printf("Integrals:\n");
    printf("a: %f\n", a);
    printf("b: %f\n", b);
    printf("c: %f\n", c); 
    printf("d: %f\n", d);

    return 0;
}
