#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    FILE_ERROR,
    OVERFLOW
} status;

status validate_flag(const char *s, char *out_action, int *out_has_n)
{
    if (s == NULL || *s == '\0') return INVALID_INPUT;
    if (s[0] != '-' && s[0] != '/') return INVALID_INPUT;

    int i = 1;
    int has_n = 0;
    if (s[i] == 'n') {
        has_n = 1;
        i++;
    }

    char action = s[i];
    if (action != 'd' && action != 'i' && action != 's' && action != 'a')
        return INVALID_INPUT;
    if (s[i + 1] != '\0') return INVALID_INPUT;

    *out_action = action;
    *out_has_n = has_n;
    return OK;
}

status make_out_path(const char *in_path, char **out_path)
{
    size_t len = strlen(in_path);
    char *buffer = (char *)malloc(len + 5);
    if (buffer == NULL) return INVALID_MEMORY;

    strcpy(buffer, "out_");
    strcat(buffer, in_path);

    *out_path = buffer;
    return OK;
}

status process_d(FILE *in, FILE *out)
{
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (c >= '0' && c <= '9') continue;
        if (fputc(c, out) == EOF) return FILE_ERROR;
    }
    return OK;
}

status process_a(FILE *in, FILE *out)
{
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (c >= '0' && c <= '9') {
            if (fputc(c, out) == EOF) return FILE_ERROR;
        } else {
            if (fprintf(out, "%X", c) < 0) return FILE_ERROR;
        }
    }
    return OK;
}

status process_i(FILE *in, FILE *out)
{
    int c;
    int count = 0;
    int has_line = 0;

    while ((c = fgetc(in)) != EOF) {
        has_line = 1;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            if (count == INT_MAX) return OVERFLOW;
            count++;
        }
        if (c == '\n') {
            if (fprintf(out, "%d\n", count) < 0) return FILE_ERROR;
            count = 0;
            has_line = 0;
        }
    }
    if (has_line) {
        if (fprintf(out, "%d\n", count) < 0) return FILE_ERROR;
    }
    return OK;
}

status process_s(FILE *in, FILE *out)
{
    int c;
    int count = 0;
    int has_line = 0;

    while ((c = fgetc(in)) != EOF) {
        has_line = 1;
        if (c == '\n') {
            if (fprintf(out, "%d\n", count) < 0) return FILE_ERROR;
            count = 0;
            has_line = 0;
            continue;
        }
        int is_letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        int is_digit = (c >= '0' && c <= '9');
        int is_space = (c == ' ');
        if (!is_letter && !is_digit && !is_space) {
            if (count == INT_MAX) return OVERFLOW;
            count++;
        }
    }
    if (has_line) {
        if (fprintf(out, "%d\n", count) < 0) return FILE_ERROR;
    }
    return OK;
}

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 4) {
        printf("Error: invalid number of arguments\n");
        return INVALID_INPUT;
    }

    char action = 0;
    int has_n = 0;
    status st = validate_flag(argv[1], &action, &has_n);
    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case FILE_ERROR:     printf("Error: file operation failed\n"); return st;
        case OVERFLOW:       printf("Error: counter overflow\n"); return st;
    }

    const char *in_path = argv[2];
    char *out_path = NULL;

    if (has_n) {
        if (argc != 4) {
            printf("Error: invalid number of arguments\n");
            return INVALID_INPUT;
        }
        out_path = argv[3];
    } else {
        if (argc != 3) {
            printf("Error: invalid number of arguments\n");
            return INVALID_INPUT;
        }
        st = make_out_path(in_path, &out_path);
        switch (st) {
            case OK: break;
            case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
            case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
            case FILE_ERROR:     printf("Error: file operation failed\n"); return st;
            case OVERFLOW:       printf("Error: counter overflow\n"); return st;
        }
    }

    FILE *in = fopen(in_path, "r");
    if (in == NULL) {
        printf("Error: cannot open input file\n");
        if (!has_n) free(out_path);
        return FILE_ERROR;
    }

    FILE *out = fopen(out_path, "w");
    if (out == NULL) {
        printf("Error: cannot open output file\n");
        fclose(in);
        if (!has_n) free(out_path);
        return FILE_ERROR;
    }

    switch (action) {
        case 'd': st = process_d(in, out); break;
        case 'i': st = process_i(in, out); break;
        case 's': st = process_s(in, out); break;
        case 'a': st = process_a(in, out); break;
        default:  st = INVALID_INPUT; break;
    }

    fclose(in);
    fclose(out);
    if (!has_n) free(out_path);

    switch (st) {
        case OK: break;
        case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
        case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
        case FILE_ERROR:     printf("Error: file operation failed\n"); return st;
        case OVERFLOW:       printf("Error: counter overflow\n"); return st;
    }

    return 0;
}
