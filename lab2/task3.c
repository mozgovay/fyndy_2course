#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    ERR_FILE_OPEN,
    INVALID_SUBSTR
} status;

#define MAX_FILES 10

static size_t my_strlen(const char *s)
{
    size_t len = 0;
    while (s[len] != '\0') len++;
    return len;
}

static int substr_eq(const char *hay, const char *needle, size_t nlen)
{
    for (size_t i = 0; i < nlen; i++)
        if (hay[i] != needle[i]) return 0;
    return 1;
}

static status read_file(const char *path, char **out_buf, size_t *out_len)
{
    if (path == NULL || out_buf == NULL || out_len == NULL)
        return INVALID_INPUT;

    FILE *f = fopen(path, "rb");
    if (!f) return ERR_FILE_OPEN;

    size_t cap = 4096, len = 0;
    char *buf = malloc(cap);
    if (!buf) { fclose(f); return INVALID_MEMORY; }

    size_t got;
    while ((got = fread(buf + len, 1, cap - len, f)) > 0) {
        len += got;
        if (len == cap) {
            size_t new_cap = cap * 2;
            char *tmp = realloc(buf, new_cap);
            if (!tmp) { free(buf); fclose(f); return INVALID_MEMORY; }
            buf = tmp;
            cap = new_cap;
        }
    }

    if (ferror(f)) { free(buf); fclose(f); return ERR_FILE_OPEN; }
    fclose(f);

    *out_buf = buf;
    *out_len = len;
    return OK;
}

static status search_in_buffer(const char *buf, size_t len,
                               const char *sub, size_t sub_len,
                               size_t *out_count)
{
    if (buf == NULL || sub == NULL || out_count == NULL)
        return INVALID_INPUT;

    size_t line = 1, col = 1, count = 0;

    for (size_t i = 0; i < len; i++) {
        if (sub_len > 0 && i + sub_len <= len &&
            substr_eq(buf + i, sub, sub_len)) {
            printf("line %zu, char %zu\n", line, col);
            count++;
        }
        if (buf[i] == '\n') { line++; col = 1; }
        else col++;
    }

    *out_count = count;
    return OK;
}

static status find_in_files(const char *substr, size_t file_count, ...)
{
    if (substr == NULL) return INVALID_SUBSTR;
    size_t sub_len = my_strlen(substr);
    if (sub_len == 0) return INVALID_SUBSTR;

    va_list ap;
    va_start(ap, file_count);

    status st = OK;

    for (size_t k = 0; k < file_count; k++) {
        const char *path = va_arg(ap, const char *);
        if (path == NULL) { st = INVALID_INPUT; break; }

        printf("=== %s ===\n", path);

        char *buf = NULL;
        size_t len = 0;
        status rs = read_file(path, &buf, &len);
        if (rs != OK) {
            printf("error: cannot read file\n");
            st = rs;
            continue;
        }

        size_t count = 0;
        rs = search_in_buffer(buf, len, substr, sub_len, &count);
        free(buf);

        if (rs != OK) { st = rs; break; }
        printf("total matches: %zu\n", count);
    }

    va_end(ap);
    return st;
}

static void print_status(status st)
{
    switch (st) {
        case INVALID_INPUT:   printf("Error: invalid input\n"); break;
        case INVALID_MEMORY:  printf("Error: memory allocation failed\n"); break;
        case ERR_FILE_OPEN:   printf("Error: cannot open file\n"); break;
        case INVALID_SUBSTR:  printf("Error: invalid substring\n"); break;
        default: break;
    }
}

int main(void)
{
    char substr[256];
    size_t count;

    if (scanf("%255s %zu", substr, &count) != 2) {
        printf("Error: invalid input\n");
        return INVALID_INPUT;
    }

    if (count == 0 || count > MAX_FILES) {
        printf("Error: file count must be 1..%d\n", MAX_FILES);
        return INVALID_INPUT;
    }

    char paths[MAX_FILES][256];
    for (size_t i = 0; i < count; i++) {
        if (scanf("%255s", paths[i]) != 1) {
            printf("Error: invalid input\n");
            return INVALID_INPUT;
        }
    }

    status st;

    switch (count) {
        case 1:  st = find_in_files(substr, 1,  paths[0]); break;
        case 2:  st = find_in_files(substr, 2,  paths[0], paths[1]); break;
        case 3:  st = find_in_files(substr, 3,  paths[0], paths[1], paths[2]); break;
        case 4:  st = find_in_files(substr, 4,  paths[0], paths[1], paths[2], paths[3]); break;
        case 5:  st = find_in_files(substr, 5,  paths[0], paths[1], paths[2], paths[3], paths[4]); break;
        case 6:  st = find_in_files(substr, 6,  paths[0], paths[1], paths[2], paths[3], paths[4], paths[5]); break;
        case 7:  st = find_in_files(substr, 7,  paths[0], paths[1], paths[2], paths[3], paths[4], paths[5], paths[6]); break;
        case 8:  st = find_in_files(substr, 8,  paths[0], paths[1], paths[2], paths[3], paths[4], paths[5], paths[6], paths[7]); break;
        case 9:  st = find_in_files(substr, 9,  paths[0], paths[1], paths[2], paths[3], paths[4], paths[5], paths[6], paths[7], paths[8]); break;
        case 10: st = find_in_files(substr, 10, paths[0], paths[1], paths[2], paths[3], paths[4], paths[5], paths[6], paths[7], paths[8], paths[9]); break;
        default: st = INVALID_INPUT; break;
    }

    if (st != OK) print_status(st);
    return st;
}
