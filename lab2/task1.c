#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    ERR_ARGC,
    ERR_FLAG,
    ERR_SEED
} status;

static size_t my_strlen(const char *s)
{
    size_t len = 0;
    while (s[len] != '\0') len++;
    return len;
}

static status is_digit(char c)
{
    if (c >= '0' && c <= '9') return OK;
    return INVALID_INPUT;
}

static status is_letter(char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return OK;
    return INVALID_INPUT;
}

static status validate_flag(const char *s, char *out_action)
{
    if (s == NULL) return ERR_FLAG;
    if (s[0] != '-' && s[0] != '/') return ERR_FLAG;
    if (s[1] == '\0' || s[2] != '\0') return ERR_FLAG;
    char c = s[1];
    if (c != 'l' && c != 'r' && c != 'u' && c != 'n' && c != 'c')
        return ERR_FLAG;
    *out_action = c;
    return OK;
}

static status validate_seed(const char *s, unsigned int *out)
{
    if (s == NULL || *s == '\0') return ERR_SEED;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 10);
    if (end == s || *end != '\0') return ERR_SEED;
    if (v > UINT_MAX) return ERR_SEED;
    *out = (unsigned int)v;
    return OK;
}

static status validate_args(int argc, char *argv[],
                            char *out_action,
                            unsigned int *out_seed,
                            char ***out_strings,
                            int *out_n)
{
    if (argc < 3) return ERR_ARGC;

    status st = validate_flag(argv[1], out_action);
    if (st != OK) return st;

    if (*out_action == 'c') {
        if (argc < 4) return ERR_ARGC;

        st = validate_seed(argv[2], out_seed);
        if (st != OK) return st;

        *out_strings = &argv[3];
        *out_n = argc - 3;
        return OK;
    }

    if (argc != 3) return ERR_ARGC;
    if (argv[2] == NULL) return INVALID_INPUT;

    *out_strings = &argv[2];
    *out_n = 1;
    return OK;
}

static status length_string(const char *s, size_t *out_len)
{
    if (s == NULL || out_len == NULL) return INVALID_INPUT;
    *out_len = my_strlen(s);
    return OK;
}

static status reverse_string(const char *s, char **out)
{
    if (s == NULL || out == NULL) return INVALID_INPUT;

    size_t len = my_strlen(s);
    char *res = malloc(len + 1);
    if (!res) return INVALID_MEMORY;

    for (size_t i = 0; i < len; i++)
        res[i] = s[len - 1 - i];
    res[len] = '\0';

    *out = res;
    return OK;
}

static status upper_odd(const char *s, char **out)
{
    if (s == NULL || out == NULL) return INVALID_INPUT;

    size_t len = my_strlen(s);
    char *res = malloc(len + 1);
    if (!res) return INVALID_MEMORY;

    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (i % 2 == 1 && c >= 'a' && c <= 'z')
            c = (char)(c - 'a' + 'A');
        res[i] = c;
    }
    res[len] = '\0';

    *out = res;
    return OK;
}

static status reorder_string(const char *s, char **out)
{
    if (s == NULL || out == NULL) return INVALID_INPUT;

    size_t len = my_strlen(s);
    char *res = malloc(len + 1);
    if (!res) return INVALID_MEMORY;

    size_t k = 0;

    for (size_t i = 0; i < len; i++)
        if (is_digit(s[i])) res[k++] = s[i];

    for (size_t i = 0; i < len; i++)
        if (is_letter(s[i])) res[k++] = s[i];

    for (size_t i = 0; i < len; i++)
        if (!is_digit(s[i]) && !is_letter(s[i])) res[k++] = s[i];

    res[len] = '\0';

    *out = res;
    return OK;
}

static status concat_random(unsigned int seed,
                            char **strings, int n,
                            char **out)
{
    if (strings == NULL || out == NULL) return INVALID_INPUT;
    if (n <= 0) return INVALID_INPUT;

    size_t total = 0;
    for (int i = 0; i < n; i++) {
        if (strings[i] == NULL) return INVALID_INPUT;
        total += my_strlen(strings[i]);
    }

    int *idx = malloc(sizeof(int) * n);
    if (!idx) return INVALID_MEMORY;

    for (int i = 0; i < n; i++) idx[i] = i;

    srand(seed);
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = idx[i];
        idx[i] = idx[j];
        idx[j] = tmp;
    }

    char *res = malloc(total + 1);
    if (!res) { free(idx); return INVALID_MEMORY; }

    size_t k = 0;
    for (int i = 0; i < n; i++) {
        const char *s = strings[idx[i]];
        size_t len = my_strlen(s);
        for (size_t j = 0; j < len; j++)
            res[k++] = s[j];
    }
    res[total] = '\0';

    free(idx);
    *out = res;
    return OK;
}

static void print_status(status st)
{
    switch (st) {
        case INVALID_INPUT:  printf("Error: invalid input\n"); break;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); break;
        case ERR_ARGC:       printf("Error: bad argument count\n"); break;
        case ERR_FLAG:       printf("Error: unknown flag\n"); break;
        case ERR_SEED:       printf("Error: bad seed\n"); break;
        default: break;
    }
}

int main(int argc, char *argv[])
{
    char action = 0;
    unsigned int seed = 0;
    char **strings = NULL;
    int n_strings = 0;

    status st = validate_args(argc, argv, &action, &seed, &strings, &n_strings);
    if (st != OK) {
        print_status(st);
        return st;
    }

    if (action == 'l') {
        size_t len = 0;
        st = length_string(strings[0], &len);
        if (st != OK) { print_status(st); return st; }
        printf("%zu\n", len);
    }
    else if (action == 'r') {
        char *out = NULL;
        st = reverse_string(strings[0], &out);
        if (st != OK) { print_status(st); return st; }
        printf("%s\n", out);
        free(out);
    }
    else if (action == 'u') {
        char *out = NULL;
        st = upper_odd(strings[0], &out);
        if (st != OK) { print_status(st); return st; }
        printf("%s\n", out);
        free(out);
    }
    else if (action == 'n') {
        char *out = NULL;
        st = reorder_string(strings[0], &out);
        if (st != OK) { print_status(st); return st; }
        printf("%s\n", out);
        free(out);
    }
    else if (action == 'c') {
        char *out = NULL;
        st = concat_random(seed, strings, n_strings, &out);
        if (st != OK) { print_status(st); return st; }
        printf("%s\n", out);
        free(out);
    }

    return OK;
}
