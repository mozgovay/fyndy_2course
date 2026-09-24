#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <math.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    ERR_OVERFLOW,
    NO_CONVERGENCE,
    ERR_TASK,
    ERR_ARGC
} status;

static int is_int(const char *s)
{
    if (s == NULL || *s == '\0') return 0;
    int i = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    if (s[i] == '\0') return 0;
    for (; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') return 0;
    }
    return 1;
}

static int is_double(const char *s)
{
    if (s == NULL || *s == '\0') return 0;
    int i = 0;
    int digits = 0;
    int dots = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    for (; s[i] != '\0'; i++) {
        if (s[i] >= '0' && s[i] <= '9') { digits++; continue; }
        if (s[i] == '.') { dots++; if (dots > 1) return 0; continue; }
        if (s[i] == 'e' || s[i] == 'E') {
            if (digits == 0) return 0;
            i++;
            if (s[i] == '-' || s[i] == '+') i++;
            if (s[i] < '0' || s[i] > '9') return 0;
            for (; s[i] != '\0'; i++) {
                if (s[i] < '0' || s[i] > '9') return 0;
            }
            return 1;
        }
        return 0;
    }
    return digits > 0;
}

static int to_int(const char *s, int *out)
{
    if (!is_int(s)) return 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') return 0;
    if (v < INT_MIN || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

static int to_double(const char *s, double *out)
{
    if (!is_double(s)) return 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || *end != '\0') return 0;
    if (isinf(v) || isnan(v)) return 0;
    *out = v;
    return 1;
}

static status check_eps(const char *s)
{
    double v;
    if (!to_double(s, &v)) return INVALID_INPUT;
    if (v <= 0.0 || v >= 1.0) return INVALID_INPUT;
    return OK;
}

status validate_args(int argc, char *argv[])
{
    if (argc < 3) return ERR_ARGC;

    int task;
    if (!to_int(argv[1], &task)) return ERR_TASK;
    if (task < 1 || task > 6) return ERR_TASK;

    if (task == 1) {
        if (argc < 9) return ERR_ARGC;
        if (check_eps(argv[2]) != OK) return INVALID_INPUT;

        int total = argc - 3;
        if (total < 6 || total % 2 != 0) return ERR_ARGC;
        if (total / 2 > 6) return INVALID_INPUT;

        for (int i = 0; i < total; i++) {
            double tmp;
            if (!to_double(argv[3 + i], &tmp)) return INVALID_INPUT;
        }
        return OK;
    }

    if (task == 2) {
        if (argc < 5) return ERR_ARGC;

        double x;
        int n;
        if (!to_double(argv[2], &x)) return INVALID_INPUT;
        if (!to_int(argv[3], &n)) return INVALID_INPUT;
        if (n < 0 || n > 6) return INVALID_INPUT;
        if (argc != 4 + (n + 1)) return ERR_ARGC;

        for (int i = 0; i <= n; i++) {
            double tmp;
            if (!to_double(argv[4 + i], &tmp)) return INVALID_INPUT;
        }
        return OK;
    }

    if (task == 3) {
        if (argc < 5) return ERR_ARGC;

        int base, n;
        if (!to_int(argv[2], &base)) return INVALID_INPUT;
        if (!to_int(argv[3], &n)) return INVALID_INPUT;
        if (base < 2 || base > 36) return INVALID_INPUT;
        if (n < 1 || n > 6) return INVALID_INPUT;
        if (argc != 4 + n) return ERR_ARGC;

        for (int i = 0; i < n; i++) {
            if (argv[4 + i] == NULL || *argv[4 + i] == '\0')
                return INVALID_INPUT;
        }
        return OK;
    }

    if (task == 4) {
        if (argc < 5) return ERR_ARGC;
        if (check_eps(argv[2]) != OK) return INVALID_INPUT;

        int n;
        if (!to_int(argv[3], &n)) return INVALID_INPUT;
        if (n <= 0 || n > 6) return INVALID_INPUT;
        if (argc != 4 + n) return ERR_ARGC;

        for (int i = 0; i < n; i++) {
            double v;
            if (!to_double(argv[4 + i], &v)) return INVALID_INPUT;
            if (v <= 0.0) return INVALID_INPUT;
        }
        return OK;
    }

    if (task == 5) {
        if (argc != 4) return ERR_ARGC;

        double base;
        int exp;
        if (!to_double(argv[2], &base)) return INVALID_INPUT;
        if (!to_int(argv[3], &exp)) return INVALID_INPUT;
        if (exp < 0) return INVALID_INPUT;
        return OK;
    }

    if (task == 6) {
        if (argc != 6) return ERR_ARGC;

        double a, b;
        int fid;
        if (!to_double(argv[2], &a)) return INVALID_INPUT;
        if (!to_double(argv[3], &b)) return INVALID_INPUT;
        if (check_eps(argv[4]) != OK) return INVALID_INPUT;
        if (!to_int(argv[5], &fid)) return INVALID_INPUT;
        if (a >= b) return INVALID_INPUT;
        if (fid < 1 || fid > 4) return INVALID_INPUT;
        return OK;
    }

    return ERR_TASK;
}

static void print_status(status st)
{
    switch (st) {
        case INVALID_INPUT:  printf("Error: invalid input\n"); break;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); break;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); break;
        case NO_CONVERGENCE: printf("Error: no convergence\n"); break;
        case ERR_TASK:       printf("Error: unknown task\n"); break;
        case ERR_ARGC:       printf("Error: bad argument count\n"); break;
        default: break;
    }
}

static int parse_double_arg(const char *s, double *out)
{
    return to_double(s, out);
}

static int parse_int_arg(const char *s, int *out)
{
    return to_int(s, out);
}

status is_convex(int n, double eps, int *out_result, ...)
{
    if (n < 3) return INVALID_INPUT;
    if (eps <= 0.0) return INVALID_INPUT;
    if (out_result == NULL) return INVALID_INPUT;

    va_list ap;
    va_start(ap, out_result);

    double x0 = va_arg(ap, double);
    double y0 = va_arg(ap, double);
    double x1 = va_arg(ap, double);
    double y1 = va_arg(ap, double);

    double first_x = x0, first_y = y0;
    double prev_x = x1, prev_y = y1;

    int sign = 0;
    int result = 1;

    for (int i = 2; i < n; i++) {
        double x2 = va_arg(ap, double);
        double y2 = va_arg(ap, double);

        double cross = (prev_x - first_x) * (y2 - prev_y) -
                       (prev_y - first_y) * (x2 - prev_x);

        int s = (cross > eps) ? 1 : ((cross < -eps) ? -1 : 0);

        if (s != 0) {
            if (sign == 0) sign = s;
            else if (s != sign) { result = 0; break; }
        }

        prev_x = x2;
        prev_y = y2;
    }

    if (result) {
        double x2 = first_x, y2 = first_y;
        double cross = (prev_x - first_x) * (y2 - prev_y) -
                       (prev_y - first_y) * (x2 - prev_x);
        int s = (cross > eps) ? 1 : ((cross < -eps) ? -1 : 0);
        if (s != 0) {
            if (sign == 0) sign = s;
            else if (s != sign) result = 0;
        }
    }

    if (result) {
        double x2 = x1, y2 = y1;
        double cross = (prev_x - first_x) * (y2 - prev_y) -
                       (prev_y - first_y) * (x2 - prev_x);
        int s = (cross > eps) ? 1 : ((cross < -eps) ? -1 : 0);
        if (s != 0) {
            if (sign == 0) sign = s;
            else if (s != sign) result = 0;
        }
    }

    va_end(ap);
    *out_result = result;
    return OK;
}

status poly_value(double x, int n, double *out, ...)
{
    if (n < 0) return INVALID_INPUT;
    if (out == NULL) return INVALID_INPUT;

    va_list ap;
    va_start(ap, out);

    double result = 0.0;
    for (int i = 0; i <= n; i++) {
        double coeff = va_arg(ap, double);
        result = result * x + coeff;
        if (isinf(result)) {
            va_end(ap);
            return ERR_OVERFLOW;
        }
    }

    va_end(ap);
    *out = result;
    return OK;
}

static int char_to_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

static long long parse_in_base(const char *s, int base, int *ok)
{
    long long result = 0;
    *ok = 1;

    if (s == NULL || *s == '\0') { *ok = 0; return 0; }

    for (int i = 0; s[i] != '\0'; i++) {
        int d = char_to_digit(s[i]);
        if (d < 0 || d >= base) { *ok = 0; return 0; }
        result = result * base + d;
        if (result < 0) { *ok = 0; return 0; }
    }
    return result;
}

static int is_kaprekar(long long n, int base)
{
    if (n == 0) return 0;

    long long sq = n * n;
    if (sq < 0) return 0;

    long long pow_base = 1;
    while (pow_base * base <= sq) pow_base *= base;

    for (long long split = pow_base; split > 0; split /= base) {
        long long left = sq / split;
        long long right = sq % split;
        if (left + right == n) return 1;
        if (split == 1) break;
    }
    return 0;
}

status kaprekar(int base, int n, int *out_count, ...)
{
    if (base < 2 || base > 36) return INVALID_INPUT;
    if (n < 1) return INVALID_INPUT;
    if (out_count == NULL) return INVALID_INPUT;

    va_list ap;
    va_start(ap, out_count);

    int count = 0;
    for (int i = 0; i < n; i++) {
        const char *s = va_arg(ap, const char *);
        int ok = 0;
        long long value = parse_in_base(s, base, &ok);
        if (!ok) {
            va_end(ap);
            return INVALID_INPUT;
        }
        if (is_kaprekar(value, base)) count++;
    }

    va_end(ap);
    *out_count = count;
    return OK;
}

status geometric_mean(int n, double eps, double *out, ...)
{
    if (n <= 0) return INVALID_INPUT;
    if (eps <= 0.0 || eps >= 1.0) return INVALID_INPUT;
    if (out == NULL) return INVALID_INPUT;

    va_list ap;
    va_start(ap, out);

    double log_sum = 0.0;
    for (int i = 0; i < n; i++) {
        double x = va_arg(ap, double);
        if (x <= eps) {
            va_end(ap);
            return INVALID_INPUT;
        }
        log_sum += log(x);
    }

    va_end(ap);
    double res = exp(log_sum / n);
    if (isinf(res)) return ERR_OVERFLOW;
    *out = res;
    return OK;
}

status fast_pow(double base, int exp, double *out)
{
    if (exp < 0) return INVALID_INPUT;
    if (out == NULL) return INVALID_INPUT;

    if (exp == 0) {
        *out = 1.0;
        return OK;
    }

    if (exp % 2 == 0) {
        double half = 0.0;
        status st = fast_pow(base, exp / 2, &half);
        if (st != OK) return st;
        double result = half * half;
        if (isinf(result)) return ERR_OVERFLOW;
        *out = result;
        return OK;
    } else {
        double prev = 0.0;
        status st = fast_pow(base, exp - 1, &prev);
        if (st != OK) return st;
        double result = base * prev;
        if (isinf(result)) return ERR_OVERFLOW;
        *out = result;
        return OK;
    }
}

static double f1(double x) { return x * x * x - 2.0 * x - 5.0; }
static double f2(double x) { return cos(x) - x; }
static double f3(double x) { return exp(x) - 3.0 * x; }
static double f4(double x) { return x * x - 2.0; }

status bisection(double a, double b, double eps,
                 double (*f)(double), double *out)
{
    if (a >= b) return INVALID_INPUT;
    if (eps <= 0.0 || eps >= 1.0) return INVALID_INPUT;
    if (f == NULL || out == NULL) return INVALID_INPUT;

    double fa = f(a);
    double fb = f(b);

    if (fa * fb > 0.0) return INVALID_INPUT;

    int iter = 0;
    while (fabs(b - a) > eps) {
        double mid = (a + b) / 2.0;
        double fm = f(mid);

        if (fabs(fm) < eps) {
            *out = mid;
            return OK;
        }

        if (fa * fm < 0.0) {
            b = mid;
            fb = fm;
        } else {
            a = mid;
            fa = fm;
        }

        iter++;
        if (iter > 1000000) return NO_CONVERGENCE;
    }

    *out = (a + b) / 2.0;
    return OK;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s 1 eps x0 y0 x1 y1 ...\n", argv[0]);
        printf("  %s 2 x n c0 c1 ... cn\n", argv[0]);
        printf("  %s 3 base n s0 s1 ... s_{n-1}\n", argv[0]);
        printf("  %s 4 eps n x0 x1 ... x_{n-1}\n", argv[0]);
        printf("  %s 5 base exp\n", argv[0]);
        printf("  %s 6 a b eps func_id\n", argv[0]);
        return ERR_ARGC;
    }

    status st = validate_args(argc, argv);
    if (st != OK) {
        print_status(st);
        return st;
    }

    int task;
    parse_int_arg(argv[1], &task);

    int ires = 0;
    double dres = 0.0;

    if (task == 1) {
        int total = argc - 3;
        int n = total / 2;

        double eps;
        parse_double_arg(argv[2], &eps);

        double *c = malloc(sizeof(double) * total);
        if (!c) { print_status(INVALID_MEMORY); return INVALID_MEMORY; }
        for (int i = 0; i < total; i++) {
            parse_double_arg(argv[3 + i], &c[i]);
        }

        switch (n) {
            case 3:
                st = is_convex(n, eps, &ires,
                    c[0], c[1], c[2], c[3], c[4], c[5]);
                break;
            case 4:
                st = is_convex(n, eps, &ires,
                    c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
                break;
            case 5:
                st = is_convex(n, eps, &ires,
                    c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7],
                    c[8], c[9]);
                break;
            case 6:
                st = is_convex(n, eps, &ires,
                    c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7],
                    c[8], c[9], c[10], c[11]);
                break;
            default:
                printf("Error: only n <= 6 supported\n");
                free(c);
                return INVALID_INPUT;
        }
        free(c);
        if (st != OK) { print_status(st); return st; }
        printf("Result: %s\n", ires ? "convex" : "not convex");
        return OK;
    }

    if (task == 2) {
        double x;
        int n;
        parse_double_arg(argv[2], &x);
        parse_int_arg(argv[3], &n);

        double *c = malloc(sizeof(double) * (n + 1));
        if (!c) { print_status(INVALID_MEMORY); return INVALID_MEMORY; }
        for (int i = 0; i <= n; i++) {
            parse_double_arg(argv[4 + i], &c[i]);
        }

        switch (n) {
            case 0: st = poly_value(x, n, &dres, c[0]); break;
            case 1: st = poly_value(x, n, &dres, c[0], c[1]); break;
            case 2: st = poly_value(x, n, &dres, c[0], c[1], c[2]); break;
            case 3: st = poly_value(x, n, &dres, c[0], c[1], c[2], c[3]); break;
            case 4: st = poly_value(x, n, &dres, c[0], c[1], c[2], c[3], c[4]); break;
            case 5: st = poly_value(x, n, &dres, c[0], c[1], c[2], c[3], c[4], c[5]); break;
            case 6: st = poly_value(x, n, &dres, c[0], c[1], c[2], c[3], c[4], c[5], c[6]); break;
            default:
                printf("Error: only n <= 6 supported\n");
                free(c);
                return INVALID_INPUT;
        }
        free(c);
        if (st != OK) { print_status(st); return st; }
        printf("Result: %f\n", dres);
        return OK;
    }

    if (task == 3) {
        int base, n;
        parse_int_arg(argv[2], &base);
        parse_int_arg(argv[3], &n);

        switch (n) {
            case 1: st = kaprekar(base, n, &ires, argv[4]); break;
            case 2: st = kaprekar(base, n, &ires, argv[4], argv[5]); break;
            case 3: st = kaprekar(base, n, &ires, argv[4], argv[5], argv[6]); break;
            case 4: st = kaprekar(base, n, &ires, argv[4], argv[5], argv[6], argv[7]); break;
            case 5: st = kaprekar(base, n, &ires, argv[4], argv[5], argv[6], argv[7], argv[8]); break;
            case 6: st = kaprekar(base, n, &ires, argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]); break;
            default:
                printf("Error: only n <= 6 supported\n");
                return INVALID_INPUT;
        }
        if (st != OK) { print_status(st); return st; }
        printf("Kaprekar count: %d\n", ires);
        return OK;
    }

    if (task == 4) {
        double eps;
        int n;
        parse_double_arg(argv[2], &eps);
        parse_int_arg(argv[3], &n);

        double *xs = malloc(sizeof(double) * n);
        if (!xs) { print_status(INVALID_MEMORY); return INVALID_MEMORY; }
        for (int i = 0; i < n; i++) {
            parse_double_arg(argv[4 + i], &xs[i]);
        }

        switch (n) {
            case 1: st = geometric_mean(n, eps, &dres, xs[0]); break;
            case 2: st = geometric_mean(n, eps, &dres, xs[0], xs[1]); break;
            case 3: st = geometric_mean(n, eps, &dres, xs[0], xs[1], xs[2]); break;
            case 4: st = geometric_mean(n, eps, &dres, xs[0], xs[1], xs[2], xs[3]); break;
            case 5: st = geometric_mean(n, eps, &dres, xs[0], xs[1], xs[2], xs[3], xs[4]); break;
            case 6: st = geometric_mean(n, eps, &dres, xs[0], xs[1], xs[2], xs[3], xs[4], xs[5]); break;
            default:
                printf("Error: only n <= 6 supported\n");
                free(xs);
                return INVALID_INPUT;
        }
        free(xs);
        if (st != OK) { print_status(st); return st; }
        printf("Geometric mean: %f\n", dres);
        return OK;
    }

    if (task == 5) {
        double base;
        int exp;
        parse_double_arg(argv[2], &base);
        parse_int_arg(argv[3], &exp);

        st = fast_pow(base, exp, &dres);
        if (st != OK) { print_status(st); return st; }
        printf("Result: %f\n", dres);
        return OK;
    }

    if (task == 6) {
        double a, b, eps;
        int fid;
        parse_double_arg(argv[2], &a);
        parse_double_arg(argv[3], &b);
        parse_double_arg(argv[4], &eps);
        parse_int_arg(argv[5], &fid);

        double (*f)(double) = NULL;
        switch (fid) {
            case 1: f = f1; break;
            case 2: f = f2; break;
            case 3: f = f3; break;
            case 4: f = f4; break;
            default:
                printf("Error: func_id must be 1..4\n");
                return INVALID_INPUT;
        }

        st = bisection(a, b, eps, f, &dres);
        if (st != OK) { print_status(st); return st; }
        printf("Root: %f\n", dres);
        return OK;
    }

    print_status(ERR_TASK);
    return ERR_TASK;
}
