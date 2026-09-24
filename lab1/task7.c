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

static int is_flag(const char *s, char *out)
{
    if (s == NULL) return 0;
    if (s[0] != '-' && s[0] != '/') return 0;
    if (s[1] == '\0') return 0;
    if (s[2] != '\0') return 0;
    if (s[1] != 'r' && s[1] != 'a') return 0;
    *out = s[1];
    return 1;
}

status validate_args(int argc, char *argv[], char *flag)
{
    if (argc < 2) return INVALID_INPUT;
    if (!is_flag(argv[1], flag)) return INVALID_INPUT;

    if (*flag == 'r') {
        if (argc != 5) return INVALID_INPUT;
        if (argv[2] == NULL || *argv[2] == '\0') return INVALID_INPUT;
        if (argv[3] == NULL || *argv[3] == '\0') return INVALID_INPUT;
        if (argv[4] == NULL || *argv[4] == '\0') return INVALID_INPUT;
        return OK;
    }

    if (*flag == 'a') {
        if (argc != 4) return INVALID_INPUT;
        if (argv[2] == NULL || *argv[2] == '\0') return INVALID_INPUT;
        if (argv[3] == NULL || *argv[3] == '\0') return INVALID_INPUT;
        return OK;
    }

    return INVALID_INPUT;
}

static void print_status(status st)
{
    switch (st) {
        case INVALID_INPUT:  printf("Error: invalid input\n"); break;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); break;
        case ERR_OVERFLOW:   printf("Error: arithmetic overflow\n"); break;
        case NO_CONVERGENCE: printf("Error: no convergence\n"); break;
        default: break;
    }
}

static int is_space_char(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
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

static status write_token(FILE *f, const char *tok, int *first)
{
    if (!*first) {
        if (fputc(' ', f) == EOF) return INVALID_INPUT;
    }
    *first = 0;
    for (size_t i = 0; tok[i] != '\0'; i++) {
        if (fputc(tok[i], f) == EOF) return INVALID_INPUT;
    }
    return OK;
}

static status do_r(const char *path1, const char *path2, const char *out_path)
{
    FILE *f1 = fopen(path1, "r");
    if (!f1) return INVALID_INPUT;

    FILE *f2 = fopen(path2, "r");
    if (!f2) { fclose(f1); return INVALID_INPUT; }

    FILE *fo = fopen(out_path, "w");
    if (!fo) { fclose(f1); fclose(f2); return INVALID_INPUT; }

    status st = OK;
    int first = 1;
    int has1 = 0, has2 = 0;
    char *t1 = NULL, *t2 = NULL;

    while (1) {
        st = read_token(f1, &t1, &has1);
        if (st != OK) break;
        if (has1) {
            st = write_token(fo, t1, &first);
            free(t1); t1 = NULL;
            if (st != OK) break;
        }

        st = read_token(f2, &t2, &has2);
        if (st != OK) break;
        if (has2) {
            st = write_token(fo, t2, &first);
            free(t2); t2 = NULL;
            if (st != OK) break;
        }

        if (!has1 && !has2) break;
    }

    free(t1);
    free(t2);
    fclose(f1);
    fclose(f2);
    fclose(fo);
    return st;
}

static status to_base4(long code, char *buf, size_t bufsize)
{
    char tmp[64];
    int len = 0;
    if (code == 0) {
        if (bufsize < 2) return ERR_OVERFLOW;
        buf[0] = '0';
        buf[1] = '\0';
        return OK;
    }
    while (code > 0 && len < (int)sizeof(tmp)) {
        tmp[len++] = (char)('0' + (code % 4));
        code /= 4;
    }
    if (len + 1 > (int)bufsize) return ERR_OVERFLOW;
    for (int i = 0; i < len; i++) {
        buf[i] = tmp[len - 1 - i];
    }
    buf[len] = '\0';
    return OK;
}

static status to_base8(long code, char *buf, size_t bufsize)
{
    char tmp[64];
    int len = 0;
    if (code == 0) {
        if (bufsize < 2) return ERR_OVERFLOW;
        buf[0] = '0';
        buf[1] = '\0';
        return OK;
    }
    while (code > 0 && len < (int)sizeof(tmp)) {
        tmp[len++] = (char)('0' + (code % 8));
        code /= 8;
    }
    if (len + 1 > (int)bufsize) return ERR_OVERFLOW;
    for (int i = 0; i < len; i++) {
        buf[i] = tmp[len - 1 - i];
    }
    buf[len] = '\0';
    return OK;
}

static status transform_token_10(FILE *fo, const char *tok, int *first)
{
    char buf[32];
    if (!*first) {
        if (fputc(' ', fo) == EOF) return INVALID_INPUT;
    }
    *first = 0;
    for (size_t i = 0; tok[i] != '\0'; i++) {
        unsigned char c = (unsigned char)tok[i];
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
        status st = to_base4((long)c, buf, sizeof(buf));
        if (st != OK) return st;
        for (size_t j = 0; buf[j] != '\0'; j++) {
            if (fputc(buf[j], fo) == EOF) return INVALID_INPUT;
        }
    }
    return OK;
}

static status transform_token_2(FILE *fo, const char *tok, int *first)
{
    if (!*first) {
        if (fputc(' ', fo) == EOF) return INVALID_INPUT;
    }
    *first = 0;
    for (size_t i = 0; tok[i] != '\0'; i++) {
        unsigned char c = (unsigned char)tok[i];
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
        if (fputc(c, fo) == EOF) return INVALID_INPUT;
    }
    return OK;
}

static status transform_token_5(FILE *fo, const char *tok, int *first)
{
    char buf[32];
    if (!*first) {
        if (fputc(' ', fo) == EOF) return INVALID_INPUT;
    }
    *first = 0;
    for (size_t i = 0; tok[i] != '\0'; i++) {
        unsigned char c = (unsigned char)tok[i];
        status st = to_base8((long)c, buf, sizeof(buf));
        if (st != OK) return st;
        for (size_t j = 0; buf[j] != '\0'; j++) {
            if (fputc(buf[j], fo) == EOF) return INVALID_INPUT;
        }
    }
    return OK;
}

static status do_a(const char *in_path, const char *out_path)
{
    FILE *fi = fopen(in_path, "r");
    if (!fi) return INVALID_INPUT;

    FILE *fo = fopen(out_path, "w");
    if (!fo) { fclose(fi); return INVALID_INPUT; }

    status st = OK;
    int first = 1;
    long idx = 0;
    char *tok = NULL;
    int has = 0;

    while (1) {
        st = read_token(fi, &tok, &has);
        if (st != OK) break;
        if (!has) break;
        idx++;

        if (idx % 10 == 0) {
            st = transform_token_10(fo, tok, &first);
        } else if (idx % 2 == 0) {
            st = transform_token_2(fo, tok, &first);
        } else if (idx % 5 == 0) {
            st = transform_token_5(fo, tok, &first);
        } else {
            st = write_token(fo, tok, &first);
        }

        free(tok);
        tok = NULL;
        if (st != OK) break;
    }

    free(tok);
    fclose(fi);
    fclose(fo);
    return st;
}

int main(int argc, char *argv[])
{
    char flag = 0;
    status st = validate_args(argc, argv, &flag);
    if (st != OK) {
        print_status(st);
        return st;
    }

    if (flag == 'r') {
        st = do_r(argv[2], argv[3], argv[4]);
        if (st != OK) {
            print_status(st);
            return st;
        }
        return OK;
    }

    if (flag == 'a') {
        st = do_a(argv[2], argv[3]);
        if (st != OK) {
            print_status(st);
            return st;
        }
        return OK;
    }

    print_status(INVALID_INPUT);
    return INVALID_INPUT;
}
