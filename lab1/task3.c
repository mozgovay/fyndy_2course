#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    ERR_OVERFLOW,
    ZERO
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

status validate_nonzero_int(const char *s, long *out)
{
    if (s == NULL || *s == '\0') return INVALID_INPUT;

    char *end = NULL;
    long value = strtol(s, &end, 10);

    if (*end != '\0') return INVALID_INPUT;
    if (value == 0) return ZERO;

    *out = value;
    return OK;
}

status task_m(const char *sx, const char *sy, int *out_result)
{
    long x = 0, y = 0;

    status st = validate_nonzero_int(sx, &x);
    if (st != OK) return st;

    st = validate_nonzero_int(sy, &y);
    if (st != OK) return st;

    *out_result = (x % y == 0) ? 1 : 0;
    return OK;
}

status task_q(const char *se, const char *sa, const char *sb, const char *sc,
              double *out_eps, double coeffs[3])
{
    status st = validate_epsilon(se, out_eps);
    if (st != OK) return st;

    st = validate_double(sa, &coeffs[0]);
    if (st != OK) return st;

    st = validate_double(sb, &coeffs[1]);
    if (st != OK) return st;

    st = validate_double(sc, &coeffs[2]);
    if (st != OK) return st;

    return OK;
}

status task_t(const char *se, const char *sa, const char *sb, const char *sc,
              double *out_eps, double sides[3])
{
    status st = validate_epsilon(se, out_eps);
    if (st != OK) return st;

    st = validate_double(sa, &sides[0]);
    if (st != OK) return st;

    st = validate_double(sb, &sides[1]);
    if (st != OK) return st;

    st = validate_double(sc, &sides[2]);
    if (st != OK) return st;

    return OK;
}

int is_unique_permutation(const double *p, double seen[][3], int n_seen)
{
    for (int i = 0; i < n_seen; i++) {
        if (seen[i][0] == p[0] && seen[i][1] == p[1] && seen[i][2] == p[2]) {
            return 0;
        }
    }
    return 1;
}

status solve_quadratic(double a, double b, double c, double eps, int *out_kind,
                       double *out_x1, double *out_x2)
{
    if (fabs(a) < eps) {
        if (fabs(b) < eps) {
            *out_kind = -1;
            return OK;
        }
        *out_kind = 1;
        *out_x1 = -c / b;
        return OK;
    }

    double d = b * b - 4.0 * a * c;

    if (d > eps) {
        *out_kind = 2;
        *out_x1 = (-b + sqrt(d)) / (2.0 * a);
        *out_x2 = (-b - sqrt(d)) / (2.0 * a);
    } else if (fabs(d) <= eps) {
        *out_kind = 1;
        *out_x1 = -b / (2.0 * a);
    } else {
        *out_kind = 0;
    }
    return OK;
}

status print_quadratic(double a, double b, double c, double eps)
{
    int kind = 0;
    double x1 = 0, x2 = 0;
    status st = solve_quadratic(a, b, c, eps, &kind, &x1, &x2);
    if (st != OK) return st;

    printf("a=%.6f b=%.6f c=%.6f: ", a, b, c);

    if (kind == -1) {
        printf("not an equation\n");
    } else if (kind == 0) {
        printf("no real roots\n");
    } else if (kind == 1) {
        printf("x = %.6f\n", x1);
    } else {
        printf("x1 = %.6f, x2 = %.6f\n", x1, x2);
    }
    return OK;
}

status run_q(double eps, double coeffs[3])
{
    double seen[6][3];
    int n_seen = 0;
    int indices[6][3] = {
        {0, 1, 2}, {0, 2, 1}, {1, 0, 2},
        {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
    };

    for (int i = 0; i < 6; i++) {
        double p[3];
        p[0] = coeffs[indices[i][0]];
        p[1] = coeffs[indices[i][1]];
        p[2] = coeffs[indices[i][2]];

        if (is_unique_permutation(p, seen, n_seen)) {
            seen[n_seen][0] = p[0];
            seen[n_seen][1] = p[1];
            seen[n_seen][2] = p[2];
            n_seen++;

            status st = print_quadratic(p[0], p[1], p[2], eps);
            if (st != OK) return st;
        }
    }
    return OK;
}

status is_right_triangle(double a, double b, double c, double eps, int *out_result)
{
    *out_result = 0;

    if (a <= 0 || b <= 0 || c <= 0) return OK;

    double a2 = a * a, b2 = b * b, c2 = c * c;

    if (fabs(a2 + b2 - c2) < eps) { *out_result = 1; return OK; }
    if (fabs(a2 + c2 - b2) < eps) { *out_result = 1; return OK; }
    if (fabs(b2 + c2 - a2) < eps) { *out_result = 1; return OK; }

    return OK;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Error: wrong number of arguments\n");
        return INVALID_INPUT;
    }

    if (argv[1][0] != '-' && argv[1][0] != '/') {
        printf("Error: invalid flag. Use -q, -m, -t (or /)\n");
        return INVALID_INPUT;
    }
    if (argv[1][1] == '\0' || argv[1][2] != '\0') {
        printf("Error: invalid flag. Use -q, -m, -t (or /)\n");
        return INVALID_INPUT;
    }

    char action = argv[1][1];
    if (action != 'q' && action != 'm' && action != 't') {
        printf("Error: invalid flag. Use -q, -m, -t (or /)\n");
        return INVALID_INPUT;
    }

    if (action == 'm') {
        if (argc != 4) {
            printf("Error: wrong number of arguments\n");
            return INVALID_INPUT;
        }

        int result = 0;
        status st = task_m(argv[2], argv[3], &result);
        switch (st) {
            case OK: break;
            case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
            case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
            case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
            case ZERO:           printf("Error: number must be non-zero\n"); return st;
        }
        if (result) {
            printf("%s is divisible by %s\n", argv[2], argv[3]);
        } else {
            printf("%s is not divisible by %s\n", argv[2], argv[3]);
        }
    } else if (action == 'q') {
        if (argc != 6) {
            printf("Error: wrong number of arguments\n");
            return INVALID_INPUT;
        }

        double eps = 0;
        double coeffs[3] = {0, 0, 0};
        status st = task_q(argv[2], argv[3], argv[4], argv[5], &eps, coeffs);
        switch (st) {
            case OK: break;
            case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
            case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
            case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
            case ZERO:           printf("Error: number must be non-zero\n"); return st;
        }
        st = run_q(eps, coeffs);
        switch (st) {
            case OK: break;
            case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
            case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
            case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
            case ZERO:           printf("Error: number must be non-zero\n"); return st;
        }
    } else if (action == 't') {
        if (argc != 6) {
            printf("Error: wrong number of arguments\n");
            return INVALID_INPUT;
        }

        double eps = 0;
        double sides[3] = {0, 0, 0};
        status st = task_t(argv[2], argv[3], argv[4], argv[5], &eps, sides);
        switch (st) {
            case OK: break;
            case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
            case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
            case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
            case ZERO:           printf("Error: number must be non-zero\n"); return st;
        }

        int right = 0;
        st = is_right_triangle(sides[0], sides[1], sides[2], eps, &right);
        switch (st) {
            case OK: break;
            case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
            case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
            case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); return st;
            case ZERO:           printf("Error: number must be non-zero\n"); return st;
        }
        if (right) {
            printf("Sides can form a right triangle\n");
        } else {
            printf("Sides cannot form a right triangle\n");
        }
    }

    return 0;
}
