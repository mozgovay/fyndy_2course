#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

typedef enum{
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    OVERFLOW
} status;

status validate_ulong(const char *s, unsigned long *out) {
    if (s == NULL || *s == '\0') return INVALID_INPUT;
    if (*s == '-') return INVALID_INPUT;

    char *end = NULL;
    unsigned long value = strtoul(s, &end, 10);

    if (*end != '\0') return INVALID_INPUT;
    if (value == 0) return INVALID_INPUT;

    *out = value;
    return OK;
}

status numbers(unsigned long a, int **b, int *kol) {
    if (a == 0) return INVALID_INPUT;
    int cap = 4;
    *b = (int *)malloc(cap * sizeof(int));
    if (*b == NULL) return INVALID_MEMORY;
    *kol = 0;
    for (int i = 1; i <= 100; i++) {
        if ((unsigned long)i % a == 0) {
            if (cap == *kol) {
                cap *= 2;
                int *b_r = (int *)realloc(*b, cap * sizeof(int));
                if (!b_r) {
                    free(*b);
                    *b = NULL;
                    return INVALID_MEMORY;
                }
                *b = b_r;
            }
            (*b)[(*kol)++] = i;
        }
    }
    return OK;
}

status is_primary(unsigned long a, int *out_kind) {
    if (a == 2) { *out_kind = 0; return OK; }
    if (a == 0 || a == 1) { *out_kind = 2; return OK; }
    if (a % 2 == 0) { *out_kind = 1; return OK; }
    for (unsigned long i = 3; i * i <= a; i += 2) {
        if (a % i == 0) { *out_kind = 1; return OK; }
    }
    *out_kind = 0;
    return OK;
}

status fac(unsigned long a, unsigned long long *result) {
    *result = 1;
    for (unsigned long i = 2; i <= a; i++) {
        if (*result > ULLONG_MAX / i) return OVERFLOW;
        *result *= i;
    }
    return OK;
}

status sum_num(unsigned long a, unsigned long long *result) {
    if (a == 0) return INVALID_INPUT;
    *result = 1;
    for (unsigned long i = 2; i <= a; i++) {
        if (*result > ULLONG_MAX - i) return OVERFLOW;
        *result += i;
    }
    return OK;
}

status degree(unsigned long a, int n, unsigned long long **result) {
    *result = (unsigned long long *)malloc(a * sizeof(unsigned long long));
    if (!*result) return INVALID_MEMORY;
    unsigned long long value = 1;
    for (unsigned long i = 1; i <= a; i++) {
        if (value > ULLONG_MAX / (unsigned long long)n) {
            free(*result);
            *result = NULL;
            return OVERFLOW;
        }
        value *= (unsigned long long)n;
        (*result)[i - 1] = value;
    }
    return OK;
}

status to_16(unsigned long a, char **result, int *kol)
{
    int cap = 4;
    char *buffer = (char *)malloc((size_t)cap * sizeof(char));
    if (buffer == NULL) {
        *result = NULL;
        return INVALID_MEMORY;
    }

    int count = 0;
    unsigned long value = a;
    while (value > 0) {
        if (count == cap) {
            cap *= 2;
            char *new_buffer = (char *)realloc(buffer, (size_t)cap);
            if (new_buffer == NULL) {
                free(buffer);
                *result = NULL;
                return INVALID_MEMORY;
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
    *result = buffer;
    *kol = count;
    return OK;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error: invalid number of arguments\n");
        return INVALID_INPUT;
    }

    if (!(argv[1][0] == '/' || argv[1][0] == '-') || argv[1][1] == '\0' || argv[1][2] != '\0') {
        printf("Error: invalid flag format\n");
        return INVALID_INPUT;
    }

    unsigned long res = 0;
    if (validate_ulong(argv[2], &res) != OK) {
        printf("Error: number must be a positive integer\n");
        return INVALID_INPUT;
    }

    switch (argv[1][1]) {
        case 'h': {
            int *result = NULL;
            int kol = 0;
            status st = numbers(res, &result, &kol);
            if (st == OK) {
                if (kol == 0) {
                    printf("No numbers in [1, 100] divisible by %lu\n", res);
                } else {
                    printf("Numbers in [1, 100] divisible by %lu:\n", res);
                    for (int i = 0; i < kol; i++) {
                        printf("%d ", result[i]);
                    }
                    printf("\n");
                }
                free(result);
            } else {
                printf("Error: memory allocation failed\n");
                return INVALID_MEMORY;
            }
            break;
        }

        case 'p': {
            int kind = 0;
            status st = is_primary(res, &kind);
            if (st != OK) {
                printf("Error: invalid input\n");
                return st;
            }
            if (kind == 2) {
                printf("%lu is neither prime nor composite\n", res);
            } else if (kind == 1) {
                printf("%lu is composite\n", res);
            } else {
                printf("%lu is prime\n", res);
            }
            break;
        }

        case 's': {
            int kol = 0;
            char *res_s = NULL;
            if (to_16(res, &res_s, &kol) == OK) {
                for (int i = kol - 1; i >= 0; i--) {
                    printf("%c ", res_s[i]);
                }
                printf("\n");
                free(res_s);
            } else {
                printf("Error: memory allocation failed\n");
                return INVALID_MEMORY;
            }
            break;
        }

        case 'e': {
            if (res > 10) {
                printf("Error: number must be not greater than 10\n");
                return INVALID_INPUT;
            }
            int osnov = 1;
            while (osnov != 11) {
                unsigned long long *resu = NULL;
                status st = degree(res, osnov, &resu);
                if (st == OK) {
                    printf("Base %d:", osnov);
                    for (unsigned long i = 0; i < res; i++) {
                        printf(" %llu", resu[i]);
                    }
                    printf("\n");
                    free(resu);
                } else if (st == OVERFLOW) {
                    printf("Error: arithmetic overflow\n");
                    return OVERFLOW;
                } else {
                    printf("Error: memory allocation failed\n");
                    return INVALID_MEMORY;
                }
                osnov++;
            }
            break;
        }

        case 'a': {
            unsigned long long res_a = 0;
            status st = sum_num(res, &res_a);
            if (st == OK) {
                printf("Sum from 1 to %lu = %llu\n", res, res_a);
            } else if (st == OVERFLOW) {
                printf("Error: arithmetic overflow\n");
                return OVERFLOW;
            } else {
                printf("Error: invalid input\n");
                return INVALID_INPUT;
            }
            break;
        }

        case 'f': {
            unsigned long long res_f = 1;
            status st = fac(res, &res_f);
            if (st == OK) {
                printf("Factorial of %lu = %llu\n", res, res_f);
            } else if (st == OVERFLOW) {
                printf("Error: arithmetic overflow\n");
                return OVERFLOW;
            } else {
                printf("Error: invalid input\n");
                return INVALID_INPUT;
            }
            break;
        }

        default:
            printf("Error: invalid flag. Use -h, -p, -s, -e, -a, -f (or /)\n");
            return INVALID_INPUT;
    }

    return 0;
}
