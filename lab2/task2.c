#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    ERR_ARGC,
    ERR_FLAG,
    ERR_FILE_OPEN,
    ERR_EMPTY_FILE,
    INVALID_RECORD
} status;

#define EPS 1e-9

typedef struct {
    unsigned int id;
    char *name;
    char *surname;
    double salary;
} Employee;

static status validate_flag(const char *s, int *out_desc)
{
    if (s == NULL) return ERR_FLAG;
    if (s[0] != '-' && s[0] != '/') return ERR_FLAG;
    if (s[1] == '\0' || s[2] != '\0') return ERR_FLAG;
    char c = s[1];
    if (c != 'a' && c != 'd') return ERR_FLAG;
    *out_desc = (c == 'd');
    return OK;
}

static status parse_uint(const char *s, unsigned int *out)
{
    if (s == NULL || out == NULL || *s == '\0') return INVALID_RECORD;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 10);
    if (end == s || *end != '\0') return INVALID_RECORD;
    if (v > UINT_MAX) return INVALID_RECORD;
    *out = (unsigned int)v;
    return OK;
}

static status parse_double(const char *s, double *out)
{
    if (s == NULL || out == NULL || *s == '\0') return INVALID_RECORD;
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || *end != '\0') return INVALID_RECORD;
    if (v < 0.0 || !isfinite(v)) return INVALID_RECORD;
    *out = v;
    return OK;
}

static status validate_word(const char *s)
{
    if (s == NULL || *s == '\0') return INVALID_RECORD;
    for (size_t i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
            return INVALID_RECORD;
    }
    return OK;
}

static status parse_record(char *line, Employee *out)
{
    if (line == NULL || out == NULL) return INVALID_INPUT;

    char *save = NULL;
    char *tok_id = strtok_r(line, " \t\r\n", &save);
    char *tok_name = strtok_r(NULL, " \t\r\n", &save);
    char *tok_surname = strtok_r(NULL, " \t\r\n", &save);
    char *tok_salary = strtok_r(NULL, " \t\r\n", &save);
    char *tok_tail = strtok_r(NULL, " \t\r\n", &save);

    if (tok_id == NULL || tok_name == NULL || tok_surname == NULL ||
        tok_salary == NULL || tok_tail != NULL)
        return INVALID_RECORD;

    unsigned int id;
    status st = parse_uint(tok_id, &id);
    if (st != OK) return st;

    st = validate_word(tok_name);
    if (st != OK) return st;
    st = validate_word(tok_surname);
    if (st != OK) return st;

    double salary;
    st = parse_double(tok_salary, &salary);
    if (st != OK) return st;

    char *name = strdup(tok_name);
    if (!name) return INVALID_MEMORY;
    char *surname = strdup(tok_surname);
    if (!surname) { free(name); return INVALID_MEMORY; }

    out->id = id;
    out->name = name;
    out->surname = surname;
    out->salary = salary;
    return OK;
}

static status read_employees(const char *path, Employee **out_arr, size_t *out_n)
{
    if (path == NULL || out_arr == NULL || out_n == NULL) return INVALID_INPUT;

    FILE *f = fopen(path, "r");
    if (!f) return ERR_FILE_OPEN;

    Employee *arr = NULL;
    size_t n = 0, cap = 0;
    char *line = NULL;
    size_t line_cap = 0;
    ssize_t read = 0;
    status st = OK;

    while ((read = getline(&line, &line_cap, f)) != -1) {
        size_t i = 0;
        while (i < (size_t)read && (line[i] == ' ' || line[i] == '\t' ||
                                     line[i] == '\r' || line[i] == '\n')) i++;
        if (i == (size_t)read) continue;

        Employee rec;
        st = parse_record(line, &rec);
        if (st != OK) break;

        if (n == cap) {
            size_t new_cap = (cap == 0) ? 8 : cap * 2;
            Employee *tmp = realloc(arr, new_cap * sizeof(Employee));
            if (!tmp) { free(rec.name); free(rec.surname); st = INVALID_MEMORY; break; }
            arr = tmp;
            cap = new_cap;
        }
        arr[n++] = rec;
    }

    free(line);
    fclose(f);

    if (st != OK) {
        for (size_t k = 0; k < n; k++) { free(arr[k].name); free(arr[k].surname); }
        free(arr);
        return st;
    }
    if (n == 0) { free(arr); return ERR_EMPTY_FILE; }

    *out_arr = arr;
    *out_n = n;
    return OK;
}

static int cmp_asc(const void *pa, const void *pb)
{
    const Employee *a = pa, *b = pb;
    if (fabs(a->salary - b->salary) > EPS)
        return (a->salary < b->salary) ? -1 : 1;
    int c = strcmp(a->surname, b->surname);
    if (c != 0) return c;
    c = strcmp(a->name, b->name);
    if (c != 0) return c;
    if (a->id < b->id) return -1;
    if (a->id > b->id) return 1;
    return 0;
}

static int cmp_desc(const void *pa, const void *pb)
{
    return -cmp_asc(pa, pb);
}

static status write_employees(const char *path, const Employee *arr, size_t n)
{
    if (path == NULL || arr == NULL) return INVALID_INPUT;

    FILE *f = fopen(path, "w");
    if (!f) return ERR_FILE_OPEN;

    status st = OK;
    for (size_t i = 0; i < n; i++) {
        if (fprintf(f, "%u %s %s %.10g\n",
                    arr[i].id, arr[i].name, arr[i].surname, arr[i].salary) < 0) {
            st = ERR_FILE_OPEN;
            break;
        }
    }
    fclose(f);
    return st;
}

static void free_employees(Employee *arr, size_t n)
{
    if (arr == NULL) return;
    for (size_t i = 0; i < n; i++) {
        free(arr[i].name);
        free(arr[i].surname);
    }
    free(arr);
}

static void print_status(status st)
{
    switch (st) {
        case INVALID_INPUT:   printf("Error: invalid input\n"); break;
        case INVALID_MEMORY:  printf("Error: memory allocation failed\n"); break;
        case ERR_ARGC:        printf("Error: bad argument count\n"); break;
        case ERR_FLAG:        printf("Error: unknown flag\n"); break;
        case ERR_FILE_OPEN:   printf("Error: cannot open file\n"); break;
        case ERR_EMPTY_FILE:  printf("Error: empty input file\n"); break;
        case INVALID_RECORD:  printf("Error: invalid record in input file\n"); break;
        default: break;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4) { print_status(ERR_ARGC); return ERR_ARGC; }

    const char *in_path = argv[1];
    int descending = 0;
    status st = validate_flag(argv[2], &descending);
    if (st != OK) { print_status(st); return st; }

    const char *out_path = argv[3];

    Employee *arr = NULL;
    size_t n = 0;
    st = read_employees(in_path, &arr, &n);
    if (st != OK) { print_status(st); return st; }

    qsort(arr, n, sizeof(Employee), descending ? cmp_desc : cmp_asc);

    st = write_employees(out_path, arr, n);
    free_employees(arr, n);

    if (st != OK) { print_status(st); return st; }
    return OK;
}
