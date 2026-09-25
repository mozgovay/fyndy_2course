#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    ERR_OVERFLOW,
    NO_CONVERGENCE
} status;

typedef struct {
    char **lines;
    size_t count;
    size_t cap;
} result_t;

status validate_args(int argc, char *argv[])
{
    if (argc != 3) return INVALID_INPUT;
    if (argv[1] == NULL || *argv[1] == '\0') return INVALID_INPUT;
    if (argv[2] == NULL || *argv[2] == '\0') return INVALID_INPUT;
    return OK;
}

static int is_space_char(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

static int digit_value(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

static status read_token(FILE *f, char **out, int *has)
{
    int c;
    size_t cap = 32, len = 0;
    char *buf = malloc(cap);
    if (!buf) return INVALID_MEMORY;

    do {
        c = fgetc(f);
        if (c == EOF) {
            if (ferror(f)) { free(buf); return INVALID_INPUT; }
            free(buf);
            *out = NULL;
            *has = 0;
            return OK;
        }
    } while (is_space_char(c));

    while (c != EOF && !is_space_char(c)) {
        if (len + 1 >= cap) {
            size_t newcap = cap * 2;
            char *tmp = realloc(buf, newcap);
            if (!tmp) { free(buf); return INVALID_MEMORY; }
            buf = tmp;
            cap = newcap;
        }
        buf[len++] = (char)c;
        c = fgetc(f);
    }
    if (c == EOF && ferror(f)) { free(buf); return INVALID_INPUT; }
    buf[len] = '\0';
    *out = buf;
    *has = 1;
    return OK;
}

static status process_token(const char *tok,
                            char **out_clean,
                            long long *out_base,
                            unsigned long long *out_value)
{
    size_t len = strlen(tok);
    if (len == 0) return INVALID_INPUT;

    int min_base = 2;
    for (size_t i = 0; i < len; i++) {
        int d = digit_value((unsigned char)tok[i]);
        if (d < 0) return INVALID_INPUT;
        if (d + 1 > min_base) min_base = d + 1;
    }

    size_t start = 0;
    while (tok[start] == '0' && tok[start + 1] != '\0') start++;
    size_t clean_len = strlen(tok + start);

    char *clean = malloc(clean_len + 1);
    if (!clean) return INVALID_MEMORY;
    memcpy(clean, tok + start, clean_len + 1);

    unsigned long long value = 0;
    for (size_t i = start; tok[i] != '\0'; i++) {
        int d = digit_value((unsigned char)tok[i]);
        unsigned long long old = value;
        value = value * (unsigned long long)min_base + (unsigned long long)d;
        if (value < old) {
            free(clean);
            return ERR_OVERFLOW;
        }
    }

    *out_clean = clean;
    *out_base = (long long)min_base;
    *out_value = value;
    return OK;
}

static status result_add(result_t *r, const char *line)
{
    if (r->count == r->cap) {
        size_t newcap = r->cap ? r->cap * 2 : 16;
        char **tmp = realloc(r->lines, newcap * sizeof(char*));
        if (!tmp) return INVALID_MEMORY;
        r->lines = tmp;
        r->cap = newcap;
    }
    char *copy = malloc(strlen(line) + 1);
    if (!copy) return INVALID_MEMORY;
    strcpy(copy, line);
    r->lines[r->count++] = copy;
    return OK;
}

static void result_free(result_t *r)
{
    if (r == NULL) return;
    for (size_t i = 0; i < r->count; i++) free(r->lines[i]);
    free(r->lines);
    r->lines = NULL;
    r->count = 0;
    r->cap = 0;
}

status do_task(FILE *fi, result_t *r)
{
    if (fi == NULL || r == NULL) return INVALID_INPUT;

    status st = OK;
    char *tok = NULL;
    int has = 0;

    while (1) {
        st = read_token(fi, &tok, &has);
        if (st != OK) break;
        if (!has) break;

        char *clean = NULL;
        long long base = 0;
        unsigned long long value = 0;

        st = process_token(tok, &clean, &base, &value);
        free(tok);
        tok = NULL;
        if (st != OK) break;

        char line[128];
        snprintf(line, sizeof(line), "%s %lld %llu", clean, base, value);
        free(clean);

        st = result_add(r, line);
        if (st != OK) break;
    }

    free(tok);
    return st;
}

int main(int argc, char *argv[])
{
    status st = validate_args(argc, argv);
    if (st != OK) {
        switch (st) {
            case INVALID_INPUT:  printf("Error: invalid input\n"); break;
            case INVALID_MEMORY: printf("Error: memory allocation failed\n"); break;
            case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); break;
            case NO_CONVERGENCE: printf("Error: no convergence\n"); break;
            default: break;
        }
        return st;
    }

    FILE *fi = fopen(argv[1], "r");
    if (!fi) {
        printf("Error: cannot open input file\n");
        return INVALID_INPUT;
    }

    result_t res = { NULL, 0, 0 };

    st = do_task(fi, &res);
    fclose(fi);

    if (st != OK) {
        result_free(&res);
        switch (st) {
            case INVALID_INPUT:  printf("Error: invalid input\n"); break;
            case INVALID_MEMORY: printf("Error: memory allocation failed\n"); break;
            case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); break;
            case NO_CONVERGENCE: printf("Error: no convergence\n"); break;
            default: break;
        }
        return st;
    }

    FILE *fo = fopen(argv[2], "w");
    if (!fo) {
        result_free(&res);
        printf("Error: cannot open output file\n");
        return INVALID_INPUT;
    }

    for (size_t i = 0; i < res.count; i++) {
        if (fprintf(fo, "%s\n", res.lines[i]) < 0) {
            result_free(&res);
            fclose(fo);
            switch (st) {
                case INVALID_INPUT:  printf("Error: invalid input\n"); break;
                case INVALID_MEMORY: printf("Error: memory allocation failed\n"); break;
                case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); break;
                case NO_CONVERGENCE: printf("Error: no convergence\n"); break;
                default: break;
            }
            return INVALID_INPUT;
        }
    }

    fclose(fo);
    result_free(&res);
    return OK;
}
