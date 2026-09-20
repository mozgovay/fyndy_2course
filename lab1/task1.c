#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

typedef enum {
    OK = 0,
    ERR_ARGC,
    ERR_FLAG,
    ERR_NUMBER,
    ERR_RANGE,
    ERR_MEMORY,
    ERR_OVERFLOW,
    ERR_DIV_ZERO
} Status;

Status validate_ulong(const char *s, unsigned long *out)
{
    if (s == NULL || *s == '\0') {
        return ERR_NUMBER;
    }
    if (*s == '-') {
        return ERR_NUMBER;
    }
    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(s, &end, 10);
    if (errno == ERANGE) {
        return ERR_OVERFLOW;
    }
    if (*end != '\0') {
        return ERR_NUMBER;
    }
    if (value == 0) {
        return ERR_NUMBER;
    }
    *out = value;
    return OK;
}

Status parse_args(int argc, const char *argv[], char *out_action, unsigned long *out_x)
{
    if (argc != 3) {
        return ERR_ARGC;
    }

    if (argv[1][0] != '-' && argv[1][0] != '/') {
        return ERR_FLAG;
    }
    if (argv[1][1] == '\0' || argv[1][2] != '\0') {
        return ERR_FLAG;
    }
    char action = argv[1][1];
    if (action != 'h' && action != 'p' && action != 's' &&
        action != 'e' && action != 'a') {
        return ERR_FLAG;
    }
    Status status = validate_ulong(argv[2], out_x);
    if (status != OK) {
        return status;
    }
    *out_action = action;
    return OK;
}

Status task_h(unsigned long x, int **out_arr, int *out_n)
{
    int capacity = 4;
    int *arr = (int *)malloc((size_t)capacity * sizeof(int));
    if (arr == NULL) {
        return ERR_MEMORY;
    }
    int count = 0;
    for (int i = 1; i <= 100; i++) {
        if ((unsigned long)i % x == 0) {
            if (count == capacity) {
                capacity *= 2;
                int *new_arr = (int *)realloc(arr, (size_t)capacity * sizeof(int));
                if (new_arr == NULL) {
                    free(arr);
                    return ERR_MEMORY;
                }
                arr = new_arr;
            }
            arr[count] = i;
            count++;
        }
    }
    *out_arr = arr;
    *out_n = count;
    return OK;
}

Status task_p(unsigned long x, int *out_kind)
{
    if (x < 2) {
        *out_kind = 2;
        return OK;
    }
    if (x == 2) {
        *out_kind = 0;
        return OK;
    }
    if (x % 2 == 0) {
        *out_kind = 1;
        return OK;
    }
    for (unsigned long d = 3; d * d <= x; d += 2) {
        if (x % d == 0) {
            *out_kind = 1;
            return OK;
        }
    }
    *out_kind = 0;
    return OK;
}

Status task_s(unsigned long x, char **out_str, int *out_len)
{
    int capacity = 4;
    char *buffer = (char *)malloc((size_t)capacity * sizeof(char));
    if (buffer == NULL) {
        return ERR_MEMORY;
    }

    int count = 0;
    unsigned long value = x;
    while (value > 0) {
        if (count == capacity) {
            capacity *= 2;
            char *new_buffer = (char *)realloc(buffer, (size_t)capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return ERR_MEMORY;
            }
            buffer = new_buffer;
        }
        int digit = (int)(value % 16);
        if (digit < 10) {
            buffer[count] = (char)('0' + digit);
        } else {
            buffer[count] = (char)('A' + digit - 10);
        }
        count++;
        value /= 16;
    }
    *out_str = buffer;
    *out_len = count;
    return OK;
}

Status task_e(unsigned long x, unsigned long long **out_table)
{
    if (x > 10) {
        return ERR_RANGE;
    }

    unsigned long long *table = (unsigned long long *)malloc(10 * x * sizeof(unsigned long long));
    if (table == NULL) {
        return ERR_MEMORY;
    }

    for (int base = 1; base <= 10; base++) {
        unsigned long long value = 1;
        for (unsigned long exponent = 1; exponent <= x; exponent++) {
            if (value > ULLONG_MAX / (unsigned long long)base) {
                free(table);
                return ERR_OVERFLOW;
            }
            value *= (unsigned long long)base;
            table[(base - 1) * x + (exponent - 1)] = value;
        }
    }
    *out_table = table;
    return OK;
}

Status task_a(unsigned long x, unsigned long long *out_sum)
{
    if (x == 0) {
        return ERR_RANGE;
    }
    unsigned long long sum = 1;
    for (unsigned long i = 2; i <= x; i++) {
        if (sum > ULLONG_MAX - i) {
            return ERR_OVERFLOW;
        }
        sum += i;
    }
    *out_sum = sum;
    return OK;
}

void print_status(Status status)
{
    switch (status) {
        case OK:
            break;
        case ERR_ARGC:
            printf("Error: expected 2 arguments: <flag> <number>\n");
            break;
        case ERR_FLAG:
            printf("Error: invalid flag. Use -h, -p, -s, -e, -a (or /)\n");
            break;
        case ERR_NUMBER:
            printf("Error: number must be a positive integer\n");
            break;
        case ERR_RANGE:
            printf("Error: number out of allowed range\n");
            break;
        case ERR_MEMORY:
            printf("Error: memory allocation failed\n");
            break;
        case ERR_OVERFLOW:
            printf("Error: arithmetic overflow\n");
            break;
        case ERR_DIV_ZERO:
            printf("Error: division by zero\n");
            break;
    }
}

void print_h_result(const int *arr, int n, unsigned long x)
{
    if (n == 0) {
        printf("No numbers in [1, 100] divisible by %lu\n", x);
        return;
    }
    printf("Numbers in [1, 100] divisible by %lu:\n", x);
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void print_p_result(unsigned long x, int kind)
{
    if (kind == 0) {
        printf("%lu is prime\n", x);
    } else if (kind == 1) {
        printf("%lu is composite\n", x);
    } else {
        printf("%lu is neither prime nor composite\n", x);
    }
}

void print_s_result(const char *s, int len)
{
    for (int i = len - 1; i >= 0; i--) {
        putchar(s[i]);
        if (i > 0) {
            putchar(' ');
        }
    }
    putchar('\n');
}

void print_e_result(const unsigned long long *table, unsigned long x)
{
    for (int base = 1; base <= 10; base++) {
        printf("Base %d:", base);
        for (unsigned long exponent = 1; exponent <= x; exponent++) {
            printf(" %llu", table[(base - 1) * x + (exponent - 1)]);
        }
        printf("\n");
    }
}

void print_a_result(unsigned long x, unsigned long long sum)
{
    printf("Sum from 1 to %lu = %llu\n", x, sum);
}

int main(int argc, char *argv[])
{
    char action = 0;
    unsigned long x = 0;

    Status status = parse_args(argc, (const char **)argv, &action, &x);
    if (status != OK) {
        print_status(status);
        return (int)status;
    }

    switch (action) {
        case 'h': {
            int *arr = NULL;
            int n = 0;
            status = task_h(x, &arr, &n);
            if (status != OK) {
                print_status(status);
                return (int)status;
            }
            print_h_result(arr, n, x);
            free(arr);
            break;
        }

        case 'p': {
            int kind = 0;
            status = task_p(x, &kind);
            if (status != OK) {
                print_status(status);
                return (int)status;
            }
            print_p_result(x, kind);
            break;
        }

        case 's': {
            char *s = NULL;
            int len = 0;
            status = task_s(x, &s, &len);
            if (status != OK) {
                print_status(status);
                return (int)status;
            }
            print_s_result(s, len);
            free(s);
            break;
        }

        case 'e': {
            unsigned long long *table = NULL;
            status = task_e(x, &table);
            if (status != OK) {
                print_status(status);
                return (int)status;
            }
            print_e_result(table, x);
            free(table);
            break;
        }

        case 'a': {
            unsigned long long sum = 0;
            status = task_a(x, &sum);
            if (status != OK) {
                print_status(status);
                return (int)status;
            }
            print_a_result(x, sum);
            break;
        }

        default:
            print_status(ERR_FLAG);
            return (int)ERR_FLAG;
    }

    return 0;
}
