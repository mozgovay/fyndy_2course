#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

typedef enum{
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY
} status;

status validate_ulong(const char *s, unsigned long *out) {
    if (s == NULL || *s == '\0') return INVALID_INPUT;
    if (*s == '-') return INVALID_INPUT;
    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(s, &end, 10);
    if (errno == ERANGE) return INVALID_INPUT;
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
                    return INVALID_MEMORY;
                }
                *b = b_r;
            }
            (*b)[(*kol)++] = i;
        }
    }
    return OK;
}

int is_primary(unsigned long a) {
    if (a == 2) return 0;
    if (a == 0 || a == 1) return 2;
    if (a % 2 == 0) return 1;
    for (unsigned long i = 3; i * i <= a; i += 2) {
        if (a % i == 0) return 1;
    }
    return 0;
}

status fac(unsigned long a, unsigned long long *result) {
    *result = 1;
    for (unsigned long i = 2; i <= a; i++) {
        if (*result > ULLONG_MAX / i) return INVALID_MEMORY;
        *result *= i;
    }
    return OK;
}

status sum_num(unsigned long a, unsigned long long *result) {
    if (a == 0) return INVALID_INPUT;
    *result = 1;
    for (unsigned long i = 2; i <= a; i++) {
        if (*result > ULLONG_MAX - i) return INVALID_MEMORY;
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
            return INVALID_MEMORY;
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
            int pr = is_primary(res);
            if (pr == 2) {
                printf("%lu is neither prime nor composite\n", res);
            } else if (pr) {
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
                } else {
                    printf("Error: arithmetic overflow\n");
                    return INVALID_MEMORY;
                }
                osnov++;
            }
            break;
        }

        case 'a': {
            unsigned long long res_a = 0;
            if (sum_num(res, &res_a) != OK) {
                printf("Error: failed to compute sum\n");
                return INVALID_INPUT;
            } else {
                printf("Sum from 1 to %lu = %llu\n", res, res_a);
            }
            break;
        }

        case 'f': {
            unsigned long long res_f = 1;
            if (fac(res, &res_f) != OK) {
                printf("Error: arithmetic overflow\n");
                return INVALID_MEMORY;
            } else {
                printf("Factorial of %lu = %llu\n", res, res_f);
            }
            break;
        }

        default:
            printf("Error: invalid flag. Use -h, -p, -s, -e, -a, -f (or /)\n");
            return INVALID_INPUT;
    }

    return 0;
}
