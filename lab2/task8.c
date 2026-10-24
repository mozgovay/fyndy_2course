#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    INVALID_BASE,
    INVALID_NUMBER
} status;

static size_t my_strlen(const char *s)
{
    size_t len = 0;
    while (s[len] != '\0') len++;
    return len;
}

static int char_to_digit(char c, int base)
{
    int d;
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
    else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
    else return -1;
    if (d >= base) return -1;
    return d;
}

static char digit_to_char(int d)
{
    if (d < 10) return (char)('0' + d);
    return (char)('a' + d - 10);
}

static status validate_number(const char *s, int base)
{
    if (s == NULL || *s == '\0') return INVALID_NUMBER;
    for (size_t i = 0; s[i] != '\0'; i++)
        if (char_to_digit(s[i], base) < 0) return INVALID_NUMBER;
    return OK;
}

static void strip_leading_zeros(char *s)
{
    size_t i = 0;
    while (s[i] == '0' && s[i + 1] != '\0') i++;
    if (i > 0) {
        size_t j = 0;
        while (s[i + j] != '\0') { s[j] = s[i + j]; j++; }
        s[j] = '\0';
    }
}

static status add_base(const char *a, const char *b, int base, char **out)
{
    if (a == NULL || b == NULL || out == NULL) return INVALID_INPUT;
    if (base < 2 || base > 36) return INVALID_BASE;

    size_t la = my_strlen(a), lb = my_strlen(b);
    size_t maxlen = (la > lb ? la : lb) + 2;

    char *res = malloc(maxlen);
    if (!res) return INVALID_MEMORY;

    size_t ia = la, ib = lb, k = 0;
    int carry = 0;

    while (ia > 0 || ib > 0 || carry > 0) {
        int da = 0, db = 0;
        if (ia > 0) {
            da = char_to_digit(a[ia - 1], base);
            if (da < 0) { free(res); return INVALID_NUMBER; }
            ia--;
        }
        if (ib > 0) {
            db = char_to_digit(b[ib - 1], base);
            if (db < 0) { free(res); return INVALID_NUMBER; }
            ib--;
        }
        int sum = da + db + carry;
        res[k++] = digit_to_char(sum % base);
        carry = sum / base;
    }
    res[k] = '\0';

    for (size_t i = 0, j = k - 1; i < j; i++, j--) {
        char t = res[i]; res[i] = res[j]; res[j] = t;
    }

    strip_leading_zeros(res);

    *out = res;
    return OK;
}

static status sum_base(int base, size_t count, char **out, ...)
{
    if (out == NULL || count == 0) return INVALID_INPUT;
    if (base < 2 || base > 36) return INVALID_BASE;

    va_list ap;
    va_start(ap, out);

    char *acc = malloc(2);
    if (!acc) { va_end(ap); return INVALID_MEMORY; }
    acc[0] = '0'; acc[1] = '\0';

    for (size_t i = 0; i < count; i++) {
        const char *s = va_arg(ap, const char *);
        if (validate_number(s, base) != OK) {
            free(acc);
            va_end(ap);
            return INVALID_NUMBER;
        }
        char *next = NULL;
        status st = add_base(acc, s, base, &next);
        free(acc);
        if (st != OK) { va_end(ap); return st; }
        acc = next;
    }

    va_end(ap);
    *out = acc;
    return OK;
}

static void print_status(status st)
{
    switch (st) {
        case INVALID_INPUT:   printf("Error: invalid input\n"); break;
        case INVALID_MEMORY:  printf("Error: memory allocation failed\n"); break;
        case INVALID_BASE:    printf("Error: base must be in [2..36]\n"); break;
        case INVALID_NUMBER:  printf("Error: invalid number for given base\n"); break;
        default: break;
    }
}

int main(void)
{
    int base;
    size_t count;

    if (scanf("%d", &base) != 1) return INVALID_INPUT;
    if (scanf("%zu", &count) != 1 || count == 0) return INVALID_INPUT;

    char **nums = calloc(count, sizeof(char *));
    if (!nums) return INVALID_MEMORY;

    for (size_t i = 0; i < count; i++) {
        char buf[1024];
        if (scanf("%1023s", buf) != 1) {
            for (size_t j = 0; j < i; j++) free(nums[j]);
            free(nums);
            return INVALID_INPUT;
        }
        nums[i] = malloc(strlen(buf) + 1);
        if (!nums[i]) {
            for (size_t j = 0; j < i; j++) free(nums[j]);
            free(nums);
            return INVALID_MEMORY;
        }
        strcpy(nums[i], buf);
    }

    char *acc = malloc(2);
    if (!acc) { for (size_t i = 0; i < count; i++) free(nums[i]); free(nums); return INVALID_MEMORY; }
    acc[0] = '0'; acc[1] = '\0';

    status st = OK;
    for (size_t i = 0; i < count; i++) {
        if (validate_number(nums[i], base) != OK) { st = INVALID_NUMBER; break; }
        char *next = NULL;
        st = add_base(acc, nums[i], base, &next);
        free(acc);
        if (st != OK) break;
        acc = next;
    }

    if (st == OK) printf("%s\n", acc);

    free(acc);
    for (size_t i = 0; i < count; i++) free(nums[i]);
    free(nums);
    return st;
}
