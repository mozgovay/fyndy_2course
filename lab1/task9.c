#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY
} status;

static int str_to_int(const char *str, int *result)
{
    if (str == NULL || *str == '\0') return 0;
    char *end = NULL;
    long v = strtol(str, &end, 10);
    if (end == str || *end != '\0') return 0;
    if (v < INT_MIN || v > INT_MAX) return 0;
    *result = (int)v;
    return 1;
}

status validate_args(int argc, char *argv[])
{
    if (argc < 2) return INVALID_INPUT;

    int task;
    if (!str_to_int(argv[1], &task)) return INVALID_INPUT;
    if (task < 1 || task > 2) return INVALID_INPUT;

    if (task == 1) {
        if (argc != 4) return INVALID_INPUT;
        int a, b;
        if (!str_to_int(argv[2], &a)) return INVALID_INPUT;
        if (!str_to_int(argv[3], &b)) return INVALID_INPUT;
        if (a > b) return INVALID_INPUT;
        return OK;
    }

    if (task == 2) {
        if (argc != 2) return INVALID_INPUT;
        return OK;
    }

    return INVALID_INPUT;
}

static void fill_random_int(int *arr, int n, int a, int b)
{
    for (int i = 0; i < n; i++) {
        arr[i] = a + rand() % (b - a + 1);
    }
}

status find_min_max_swap(int *arr, int n, int *out_min, int *out_max)
{
    if (arr == NULL || out_min == NULL || out_max == NULL) return INVALID_INPUT;
    if (n <= 0) return INVALID_INPUT;

    int min_idx = 0, max_idx = 0;
    for (int i = 1; i < n; i++) {
        if (arr[i] < arr[min_idx]) min_idx = i;
        if (arr[i] > arr[max_idx]) max_idx = i;
    }

    int tmp = arr[min_idx];
    arr[min_idx] = arr[max_idx];
    arr[max_idx] = tmp;

    *out_min = arr[max_idx];
    *out_max = arr[min_idx];

    return OK;
}

status fill_random_int_range(int *arr, int n, int a, int b)
{
    if (arr == NULL) return INVALID_INPUT;
    if (n <= 0) return INVALID_INPUT;

    for (int i = 0; i < n; i++) {
        arr[i] = a + rand() % (b - a + 1);
    }
    return OK;
}

status build_c(const int *A, int na, const int *B, int nb, int *C)
{
    if (A == NULL || B == NULL || C == NULL) return INVALID_INPUT;
    if (na <= 0 || nb <= 0) return INVALID_INPUT;

    for (int i = 0; i < na; i++) {
        int target = A[i];
        int best = B[0];
        int best_diff = abs(target - B[0]);

        for (int j = 1; j < nb; j++) {
            int diff = abs(target - B[j]);
            if (diff < best_diff) {
                best_diff = diff;
                best = B[j];
            }
        }
        C[i] = A[i] + best;
    }
    return OK;
}

int main(int argc, char *argv[])
{
    status st = validate_args(argc, argv);
    if (st != OK) {
        switch (st) {
            case INVALID_INPUT:  printf("Error: invalid input\n"); break;
            case INVALID_MEMORY: printf("Error: memory allocation failed\n"); break;
            default: break;
        }
        return st;
    }

    int task;
    str_to_int(argv[1], &task);

    srand((unsigned)time(NULL));

    if (task == 1) {
        int a, b;
        str_to_int(argv[2], &a);
        str_to_int(argv[3], &b);

        int arr[20];
        fill_random_int(arr, 20, a, b);

        printf("Original: ");
        for (int i = 0; i < 20; i++) printf("%d ", arr[i]);
        printf("\n");

        int mn = 0, mx = 0;
        st = find_min_max_swap(arr, 20, &mn, &mx);
        switch (st) {
            case OK: break;
            case INVALID_INPUT:  printf("Error: invalid input\n"); return st;
            case INVALID_MEMORY: printf("Error: memory allocation failed\n"); return st;
            default: break;
        }

        printf("After swap: ");
        for (int i = 0; i < 20; i++) printf("%d ", arr[i]);
        printf("\n");
        printf("min = %d, max = %d\n", mn, mx);
        return OK;
    }

    if (task == 2) {
        int na = 10 + rand() % (10000 - 10 + 1);
        int nb = 10 + rand() % (10000 - 10 + 1);

        int *A = malloc(sizeof(int) * na);
        if (!A) { printf("Error: memory allocation failed\n"); return INVALID_MEMORY; }

        int *B = malloc(sizeof(int) * nb);
        if (!B) { free(A); printf("Error: memory allocation failed\n"); return INVALID_MEMORY; }

        st = fill_random_int_range(A, na, -1000, 1000);
        if (st != OK) {
            free(A); free(B);
            printf("Error: invalid input\n");
            return st;
        }

        st = fill_random_int_range(B, nb, -1000, 1000);
        if (st != OK) {
            free(A); free(B);
            printf("Error: invalid input\n");
            return st;
        }

        int *C = malloc(sizeof(int) * na);
        if (!C) {
            free(A); free(B);
            printf("Error: memory allocation failed\n");
            return INVALID_MEMORY;
        }

        st = build_c(A, na, B, nb, C);
        if (st != OK) {
            free(A); free(B); free(C);
            switch (st) {
                case INVALID_INPUT:  printf("Error: invalid input\n"); break;
                case INVALID_MEMORY: printf("Error: memory allocation failed\n"); break;
                default: break;
            }
            return st;
        }

        printf("na = %d, nb = %d\n", na, nb);
        printf("A: ");
        for (int i = 0; i < na && i < 10; i++) printf("%d ", A[i]);
        printf("...\n");
        printf("B: ");
        for (int i = 0; i < nb && i < 10; i++) printf("%d ", B[i]);
        printf("...\n");
        printf("C: ");
        for (int i = 0; i < na && i < 10; i++) printf("%d ", C[i]);
        printf("...\n");

        free(A);
        free(B);
        free(C);
        return OK;
    }

    printf("Error: unknown task\n");
    return INVALID_INPUT;
}
