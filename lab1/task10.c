#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY
} status;

static int digit_value(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

status validate_base(int base)
{
    if (base < 2 || base > 36) return INVALID_INPUT;
    return OK;
}

static int validate_number(const char *s, int base)
{
    if (s == NULL || *s == '\0') return 0;
    int i = 0;
    if (s[0] == '-') i = 1;
    if (s[i] == '\0') return 0;
    for (; s[i] != '\0'; i++) {
        int d = digit_value((unsigned char)s[i]);
        if (d < 0 || d >= base) return 0;
    }
    return 1;
}

static status parse_number(const char *s, int base, long long *out)
{
    if (!validate_number(s, base)) return INVALID_INPUT;

    int i = 0;
    int neg = 0;
    if (s[0] == '-') { neg = 1; i = 1; }

    long long value = 0;
    for (; s[i] != '\0'; i++) {
        int d = digit_value((unsigned char)s[i]);
        long long old = value;
        value = value * base + d;
        if (value < old) return INVALID_INPUT;
        if (value < 0) return INVALID_INPUT;
    }

    if (neg) value = -value;
    *out = value;
    return OK;
}

status to_base_string(long long value, int base, char **out)
{
    if (base < 2 || base > 36) return INVALID_INPUT;
    if (out == NULL) return INVALID_INPUT;

    int neg = 0;
    unsigned long long v;
    if (value < 0) { neg = 1; v = (unsigned long long)(-value); }
    else v = (unsigned long long)value;

    char tmp[128];
    int len = 0;
    if (v == 0) tmp[len++] = '0';
    while (v > 0) {
        int d = (int)(v % (unsigned long long)base);
        if (d < 10) tmp[len++] = (char)('0' + d);
        else tmp[len++] = (char)('A' + d - 10);
        v /= (unsigned long long)base;
    }
    if (neg) tmp[len++] = '-';

    char *res = malloc(len + 1);
    if (!res) return INVALID_MEMORY;
    for (int i = 0; i < len; i++) {
        res[i] = tmp[len - 1 - i];
    }
    res[len] = '\0';
    *out = res;
    return OK;
}

static void strip_leading_zeros(char *s)
{
    int neg = 0;
    int start = 0;
    if (s[0] == '-') { neg = 1; start = 1; }

    int i = start;
    while (s[i] == '0' && s[i + 1] != '\0') i++;

    if (neg) {
        s[0] = '-';
        memmove(s + 1, s + i, strlen(s + i) + 1);
    } else {
        memmove(s, s + i, strlen(s + i) + 1);
    }
}

int main(void)
{
    int base;
    char buf[256];

    printf("Enter base (2..36): ");
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
        printf("Error: invalid input\n");
        return INVALID_INPUT;
    }

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';

    char *end = NULL;
    long b = strtol(buf, &end, 10);
    if (end == buf || *end != '\0' || b < 2 || b > 36) {
        printf("Error: invalid input\n");
        return INVALID_INPUT;
    }
    base = (int)b;

    long long max_abs = 0;
    long long max_val = 0;
    int has_max = 0;
    long long sum = 0;

    printf("Enter numbers (Stop to finish):\n");
    while (1) {
        printf("> ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) break;

        len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';

        if (strcmp(buf, "Stop") == 0) break;

        long long value;
        status st = parse_number(buf, base, &value);
        switch (st) {
            case OK: break;
            case INVALID_INPUT:
                printf("Error: invalid number\n");
                return st;
            case INVALID_MEMORY:
                printf("Error: memory allocation failed\n");
                return st;
        }

        long long a = value < 0 ? -value : value;
        if (!has_max || a > max_abs) {
            max_abs = a;
            max_val = value;
            has_max = 1;
        }

        sum += value;
    }

    if (!has_max) {
        printf("Error: no numbers entered\n");
        return INVALID_INPUT;
    }

    char *max_repr = NULL;
    status st = to_base_string(max_val, base, &max_repr);
    if (st != OK) {
        printf("Error: memory allocation failed\n");
        return st;
    }
    strip_leading_zeros(max_repr);

    char *sum_repr = NULL;
    st = to_base_string(sum, base, &sum_repr);
    if (st != OK) {
        free(max_repr);
        printf("Error: memory allocation failed\n");
        return st;
    }
    strip_leading_zeros(sum_repr);

    printf("Max by abs: %s\n", max_repr);
    printf("Sum: %s\n", sum_repr);

    int bases[4] = { 9, 18, 27, 36 };
    for (int i = 0; i < 4; i++) {
        int bb = bases[i];

        char *m = NULL;
        st = to_base_string(max_val, bb, &m);
        if (st != OK) {
            free(max_repr); free(sum_repr);
            printf("Error: memory allocation failed\n");
            return st;
        }
        strip_leading_zeros(m);

        char *s = NULL;
        st = to_base_string(sum, bb, &s);
        if (st != OK) {
            free(m); free(max_repr); free(sum_repr);
            printf("Error: memory allocation failed\n");
            return st;
        }
        strip_leading_zeros(s);

        printf("Base %d: max = %s, sum = %s\n", bb, m, s);

        free(m);
        free(s);
    }

    free(max_repr);
    free(sum_repr);
    return OK;
}
