#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

typedef enum {
    OK = 0,
    ERR_ARGC,
    ERR_NUMBER,
    ERR_RANGE,
    ERR_MEMORY,
    ERR_OVERFLOW,
    ERR_CONVERGENCE
} Status;

Status validate_epsilon(const char *s, double *out)
{
    if (s == NULL || *s == '\0') {
        return ERR_NUMBER;
    }
    if (*s == '-') {
        return ERR_NUMBER;
    }

    errno = 0;
    char *end = NULL;
    double value = strtod(s, &end);

    if (errno == ERANGE) {
        return ERR_OVERFLOW;
    }
    if (*end != '\0') {
        return ERR_NUMBER;
    }
    if (value <= 0.0 || value >= 1.0) {
        return ERR_RANGE;
    }

    *out = value;
    return OK;
}

Status limit_e(double epsilon, double *out)
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

Status row_e(double epsilon, double *out)
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

Status equation_e(double epsilon, double *out)
{
    double x = 3.0, f = 1.0;
    while (fabs(f) > epsilon) {
        f = log(x) - 1.0;
        x = x - f / (1.0 / x);
    }
    *out = x;
    return OK;
}

Status limit_pi(double epsilon, double *out)
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

Status row_pi(double epsilon, double *out)
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

Status equation_pi(double epsilon, double *out)
{
    double x = 3.0, f = 1.0;
    while (fabs(f) > epsilon) {
        f = cos(x) + 1.0;
        x = x - f / (-sin(x));
    }
    *out = x;
    return OK;
}

Status limit_ln(double epsilon, double *out)
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

Status row_ln(double epsilon, double *out)
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

Status equation_ln(double epsilon, double *out)
{
    double x = 1.0, f = 1.0;
    while (fabs(f) > epsilon) {
        f = exp(x) - 2.0;
        x = x - f / exp(x);
    }
    *out = x;
    return OK;
}

Status limit_sqrt(double epsilon, double *out)
{
    double current = -0.5, previous = 0.0;
    do {
        previous = current;
        current = (2.0 * previous - previous * previous + 2.0) / 2.0;
    } while (fabs(previous - current) >= epsilon);
    *out = current;
    return OK;
}

Status row_sqrt(double epsilon, double *out)
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

Status equation_sqrt(double epsilon, double *out)
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

Status limit_gamma(double epsilon, double *out)
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

Status row_gamma(double epsilon, double *out)
{
    double current = 0.5, previous = 0.0;
    int k = 2;
    do {
        previous = current;
        k++;
        double root = sqrt(k);
        if (fabs(root - floor(root)) < 1e-12) {
            k++;
            root = sqrt(k);
        }
        current += 1.0 / (floor(root) * floor(root)) - 1.0 / k;
    } while (fabs(previous - current) >= epsilon);
    *out = current - M_PI * M_PI / 6.0;
    return OK;
}

Status equation_gamma(double epsilon, double *out)
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

void print_status(Status status)
{
    switch (status) {
        case OK:                break;
        case ERR_ARGC:          printf("Error: expected 1 argument: <epsilon>\n"); break;
        case ERR_NUMBER:        printf("Error: epsilon must be a positive number\n"); break;
        case ERR_RANGE:         printf("Error: epsilon out of range (0 < e < 1)\n"); break;
        case ERR_MEMORY:        printf("Error: memory allocation failed\n"); break;
        case ERR_OVERFLOW:      printf("Error: arithmetic overflow\n"); break;
        case ERR_CONVERGENCE:   printf("Error: convergence not reached\n"); break;
    }
}

void print_header(void)
{
    printf("Const Limit Row Equation\n");
}

void print_row(const char *name, double limit, double row, double equation)
{
    printf("%s %.10f %.10f %.10f\n", name, limit, row, equation);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        print_status(ERR_ARGC);
        return (int)ERR_ARGC;
    }

    double epsilon = 0.0;
    Status status = validate_epsilon(argv[1], &epsilon);
    if (status != OK) {
        print_status(status);
        return (int)status;
    }

    double a, b, c;

    print_header();

    limit_e(epsilon, &a);
    row_e(epsilon, &b);
    equation_e(epsilon, &c);
    print_row("e", a, b, c);

    limit_pi(epsilon, &a);
    row_pi(epsilon, &b);
    equation_pi(epsilon, &c);
    print_row("pi", a, b, c);

    limit_ln(epsilon, &a);
    row_ln(epsilon, &b);
    equation_ln(epsilon, &c);
    print_row("ln2", a, b, c);

    limit_sqrt(epsilon, &a);
    row_sqrt(epsilon, &b);
    equation_sqrt(epsilon, &c);
    print_row("sqrt(2)", a, b, c);

    limit_gamma(epsilon, &a);
    row_gamma(epsilon, &b);
    equation_gamma(epsilon, &c);
    print_row("gamma", a, b, c);

    return 0;
}
