#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    OVERFLO
} status;

status validate_epsilon(const char *s, double *out)
{
    if (s == NULL || *s == '\0') return INVALID_INPUT;
    if (*s == '-') return INVALID_INPUT;
    errno = 0;
    char *end = NULL;
    double value = strtod(s, &end);
    if (errno == ERANGE) return OVERFLO;
    if (*end != '\0') return INVALID_INPUT;
    if (value <= 0.0 || value >= 1.0) return INVALID_INPUT;
    *out = value;
    return OK;
}

status limit_e(double epsilon, double *out)
{
    double n = 1.0, current = 0.0, previous = 0.0;
    do {
        previous = current;
        n *= 2.0;
        current = pow(1.0 + 1.0 / n, n);
    } while (fabs(previous - current) >= epsilon);
    *out = current;
    return OK;
}

status row_e(double epsilon, double *out)
{
    double current = 1.0, previous = 0.0, term = 1.0;
    int n = 1;
    while (fabs(previous - current) >= epsilon) {
        previous = current;
        term *= 1.0 / n;
        current += term;
        n++;
    }
    *out = current;
    return OK;
}

status equation_e(double epsilon, double *out)
{
    double x = 3.0, f = 1.0;
    while (fabs(f) > epsilon) {
        f = log(x) - 1.0;
        x = x - f / (1.0 / x);
    }
    *out = x;
    return OK;
}

status limit_pi(double epsilon, double *out)
{
    int n = 2;
    double current = 4.0, previous = 0.0;
    do {
        previous = current;
        current *= (4.0 * n * (n - 1.0)) / pow(2.0 * n - 1.0, 2);
        n++;
    } while (fabs(previous - current) >= epsilon);
    *out = current;
    return OK;
}

status row_pi(double epsilon, double *out)
{
    int n = 1;
    double current = 1.0, previous = 0.0;
    do {
        previous = current;
        n++;
        current -= 1.0 / (2.0 * n - 1.0);
        n++;
        current += 1.0 / (2.0 * n - 1.0);
    } while (fabs(previous - current) >= epsilon);
    *out = current * 4.0;
    return OK;
}

status equation_pi(double epsilon, double *out)
{
    double x = 3.0, f = 1.0;
    while (fabs(f) > epsilon) {
        f = cos(x) + 1.0;
        x = x - f / (-sin(x));
    }
    *out = x;
    return OK;
}

status limit_ln(double epsilon, double *out)
{
    double n = 1.0, current = 0.0, previous = 0.0;
    do {
        previous = current;
        n *= 2.0;
        current = n * (pow(2.0, 1.0 / n) - 1.0);
    } while (fabs(previous - current) >= epsilon);
    *out = current;
    return OK;
}

status row_ln(double epsilon, double *out)
{
    int n = 1;
    double current = 1.0, previous = 0.0;
    do {
        previous = current;
        n++;
        current -= 1.0 / n;
        n++;
        current += 1.0 / n;
    } while (fabs(previous - current) >= epsilon);
    *out = current;
    return OK;
}

status equation_ln(double epsilon, double *out)
{
    double x = 1.0, f = 1.0;
    while (fabs(f) > epsilon) {
        f = exp(x) - 2.0;
        x = x - f / exp(x);
    }
    *out = x;
    return OK;
}

status limit_sqrt(double epsilon, double *out)
{
    double current = -0.5, previous = 0.0;
    do {
        previous = current;
        current = (2.0 * previous - previous * previous + 2.0) / 2.0;
    } while (fabs(previous - current) >= epsilon);
    *out = current;
    return OK;
}

status row_sqrt(double epsilon, double *out)
{
    int k = 2;
    double current = pow(2.0, 1.0 / 4.0), previous = 0.0;
    do {
        previous = current;
        k++;
        current *= pow(2.0, 1.0 / pow(2.0, k));
    } while (fabs(previous - current) >= epsilon);
    *out = current;
    return OK;
}

status equation_sqrt(double epsilon, double *out)
{
    double x = 1.0, f = 1.0;
    while (fabs(f) > epsilon) {
        f = x * x - 2.0;
        x = x - f / (2.0 * x);
    }
    *out = x;
    return OK;
}

int is_prime(int number)
{
    if (number < 2) return 0;
    if (number == 2) return 1;
    if (number % 2 == 0) return 0;
    for (int i = 3; i * i <= number; i += 2) {
        if (number % i == 0) return 0;
    }
    return 1;
}

status limit_gamma(double epsilon, double *out)
{
    double n = 1.0, current = 0.0, previous = 0.0;
    do {
        previous = current;
        n *= 2.0;
        double harmonic = 0.0;
        for (int i = 1; i <= (int)n; i++) {
            harmonic += 1.0 / i;
        }
        current = harmonic - log(n);
    } while (fabs(previous - current) >= epsilon);
    *out = current;
    return OK;
}

status row_gamma(double epsilon, double *out)
{
    double current = 0.5, previous = 0.0;
    int k = 2;
    do {
        previous = current;
        k++;
        double root = floor(sqrt(k));
        current += 1.0 / (root * root) - 1.0 / k;
    } while (fabs(previous - current) >= epsilon);
    *out = current - M_PI * M_PI / 6.0;
    return OK;
}

status equation_gamma(double epsilon, double *out)
{
    int p = 2;
    double current = log(2.0) * 0.5, previous = 0.0, product = 0.5;
    do {
        previous = current;
        do {
            p++;
        } while (!is_prime(p));
        product *= (p - 1.0) / p;
        current = log(p) * product;
    } while (fabs(previous - current) >= epsilon);
    *out = -log(current);
    return OK;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Error: expected 1 argument: <epsilon>\n");
        return INVALID_INPUT;
    }

    double epsilon = 0.0;
    status st = validate_epsilon(argv[1], &epsilon);
    if (st != OK) {
        if (st == INVALID_INPUT) printf("Error: epsilon must be a positive number in (0, 1)\n");
        else if (st == OVERFLO) printf("Error: arithmetic overflow\n");
        return st;
    }

    double a, b, c;

    printf("Const Limit Row Equation\n");

    st = limit_e(epsilon, &a);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    st = row_e(epsilon, &b);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    st = equation_e(epsilon, &c);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    printf("e %f %f %f\n", a, b, c);

    st = limit_pi(epsilon, &a);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    st = row_pi(epsilon, &b);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    st = equation_pi(epsilon, &c);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    printf("pi %f %f %f\n", a, b, c);

    st = limit_ln(epsilon, &a);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    st = row_ln(epsilon, &b);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    st = equation_ln(epsilon, &c);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    printf("ln2 %f %f %f\n", a, b, c);

    st = limit_sqrt(epsilon, &a);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    st = row_sqrt(epsilon, &b);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    st = equation_sqrt(epsilon, &c);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    printf("sqrt(2) %f %f %f\n", a, b, c);

    st = limit_gamma(epsilon, &a);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    st = row_gamma(epsilon, &b);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    st = equation_gamma(epsilon, &c);
    if (st != OK) { printf("Error: convergence not reached\n"); return st; }
    printf("gamma %f %f %f\n", a, b, c);

    return 0;
}
