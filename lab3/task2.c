#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

typedef struct {
    unsigned long long id;
    int start;
    int finish;
} Talk;

typedef enum {
    STATUS_OK,
    STATUS_INVALID_INPUT,
    STATUS_MEMORY_ERROR,
    STATUS_IO_ERROR,
    STATUS_END_OF_INPUT
} Status;

static Status read_token(char *buffer, size_t capacity)
{
    int c;
    size_t i = 0;

    do {
        c = getchar();

        if (c == EOF) {
            if (ferror(stdin)) {
                return STATUS_IO_ERROR;
            }
            return STATUS_END_OF_INPUT;
        }
    } while (isspace((unsigned char)c));

    do {
        if (i + 1 >= capacity) {
            while ((c = getchar()) != EOF &&
                   !isspace((unsigned char)c)) {
            }

            if (c == EOF && ferror(stdin)) {
                return STATUS_IO_ERROR;
            }

            return STATUS_INVALID_INPUT;
        }

        buffer[i++] = (char)c;
        c = getchar();

    } while (c != EOF && !isspace((unsigned char)c));

    if (c == EOF && ferror(stdin)) {
        return STATUS_IO_ERROR;
    }

    buffer[i] = '\0';

    return STATUS_OK;
}

static Status parse_positive_number(
    const char *str,
    unsigned long long *result)
{
    unsigned long long value = 0;
    size_t i = 0;

    if (str == NULL || result == NULL || str[0] == '\0') {
        return STATUS_INVALID_INPUT;
    }

    while (str[i] != '\0') {
        unsigned int digit;

        if (str[i] < '0' || str[i] > '9') {
            return STATUS_INVALID_INPUT;
        }

        digit = (unsigned int)(str[i] - '0');

        if (value > (ULLONG_MAX - digit) / 10) {
            return STATUS_INVALID_INPUT;
        }

        value = value * 10 + digit;
        i++;
    }

    if (value == 0) {
        return STATUS_INVALID_INPUT;
    }

    *result = value;

    return STATUS_OK;
}

static Status parse_time(const char *str, int *result)
{
    int hours;
    int minutes;

    if (str == NULL || result == NULL || strlen(str) != 5) {
        return STATUS_INVALID_INPUT;
    }

    if (str[0] < '0' || str[0] > '9' ||
        str[1] < '0' || str[1] > '9' ||
        str[2] != ':' ||
        str[3] < '0' || str[3] > '9' ||
        str[4] < '0' || str[4] > '9') {
        return STATUS_INVALID_INPUT;
    }

    hours = (str[0] - '0') * 10 + (str[1] - '0');
    minutes = (str[3] - '0') * 10 + (str[4] - '0');

    if (hours > 23 || minutes > 59) {
        return STATUS_INVALID_INPUT;
    }

    *result = hours * 60 + minutes;

    return STATUS_OK;
}

static int cmp_by_id(const void *a, const void *b)
{
    const Talk *ta = (const Talk *)a;
    const Talk *tb = (const Talk *)b;

    if (ta->id < tb->id) {
        return -1;
    }
    if (ta->id > tb->id) {
        return 1;
    }
    return 0;
}

static int cmp_by_finish(const void *a, const void *b)
{
    const Talk *ta = (const Talk *)a;
    const Talk *tb = (const Talk *)b;

    if (ta->finish < tb->finish) {
        return -1;
    }
    if (ta->finish > tb->finish) {
        return 1;
    }
    if (ta->start < tb->start) {
        return -1;
    }
    if (ta->start > tb->start) {
        return 1;
    }
    return 0;
}

static Status validate_ids(Talk *talks, size_t n)
{
    size_t i;

    if (talks == NULL || n == 0) {
        return STATUS_INVALID_INPUT;
    }

    for (i = 1; i < n; i++) {
        if (talks[i - 1].id == talks[i].id) {
            return STATUS_INVALID_INPUT;
        }
    }

    return STATUS_OK;
}

Status max_talks(
    const Talk *talks,
    size_t n,
    size_t k,
    size_t *answer)
{
    Talk *sorted_talks;
    int *room_finish;
    size_t room_count;
    size_t count = 0;
    size_t i;
    size_t j;
    size_t best_room;
    int best_finish;
    Status status;

    if (talks == NULL || answer == NULL || n == 0 || k == 0) {
        return STATUS_INVALID_INPUT;
    }

    for (i = 0; i < n; i++) {
        if (talks[i].id == 0 ||
            talks[i].start < 0 ||
            talks[i].start >= 1440 ||
            talks[i].finish < 0 ||
            talks[i].finish >= 1440 ||
            talks[i].start >= talks[i].finish) {
            return STATUS_INVALID_INPUT;
        }
    }

    if (n > SIZE_MAX / sizeof(Talk)) {
        return STATUS_MEMORY_ERROR;
    }

    room_count = k < n ? k : n;

    if (room_count > SIZE_MAX / sizeof(int)) {
        return STATUS_MEMORY_ERROR;
    }

    sorted_talks = malloc(n * sizeof(Talk));
    if (sorted_talks == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    room_finish = malloc(room_count * sizeof(int));
    if (room_finish == NULL) {
        free(sorted_talks);
        return STATUS_MEMORY_ERROR;
    }

    memcpy(sorted_talks, talks, n * sizeof(Talk));

    qsort(sorted_talks, n, sizeof(Talk), cmp_by_id);

    status = validate_ids(sorted_talks, n);
    if (status != STATUS_OK) {
        free(sorted_talks);
        free(room_finish);
        return status;
    }

    qsort(sorted_talks, n, sizeof(Talk), cmp_by_finish);

    for (i = 0; i < room_count; i++) {
        room_finish[i] = -1441;
    }

    for (i = 0; i < n; i++) {
        best_room = room_count;
        best_finish = INT_MIN;

        for (j = 0; j < room_count; j++) {
            if (room_finish[j] <= sorted_talks[i].start &&
                room_finish[j] > best_finish) {
                best_finish = room_finish[j];
                best_room = j;
            }
        }

        if (best_room != room_count) {
            room_finish[best_room] = sorted_talks[i].finish;
            count++;
        }
    }

    *answer = count;

    free(sorted_talks);
    free(room_finish);

    return STATUS_OK;
}

int main(void)
{
    char token[128];
    unsigned long long n_input;
    unsigned long long k_input;
    size_t n;
    size_t k;
    size_t answer = 0;
    size_t i;
    Talk *talks;
    Status status;

    status = read_token(token, sizeof(token));
    if (status != STATUS_OK) {
        printf("%d\n", status);
        return -1;
    }

    status = parse_positive_number(token, &n_input);
    if (status != STATUS_OK) {
        return 1;
    }

    status = read_token(token, sizeof(token));
    if (status != STATUS_OK) {
        return 1;
    }

    status = parse_positive_number(token, &k_input);
    if (status != STATUS_OK) {
        return 1;
    }

    if (n_input > (unsigned long long)(SIZE_MAX / sizeof(Talk))) {
        return 1;
    }

    n = (size_t)n_input;
    k = (size_t)k_input;

    talks = malloc(n * sizeof(Talk));
    if (talks == NULL) {
        return 1;
    }

    for (i = 0; i < n; i++) {
        status = read_token(token, sizeof(token));
        if (status != STATUS_OK) {
            free(talks);
            return 1;
        }

        status = parse_positive_number(token, &talks[i].id);
        if (status != STATUS_OK) {
            free(talks);
            return 1;
        }

        status = read_token(token, sizeof(token));
        if (status != STATUS_OK) {
            free(talks);
            return 1;
        }

        status = parse_time(token, &talks[i].start);
        if (status != STATUS_OK) {
            free(talks);
            return 1;
        }

        status = read_token(token, sizeof(token));
        if (status != STATUS_OK) {
            free(talks);
            return 1;
        }

        status = parse_time(token, &talks[i].finish);
        if (status != STATUS_OK ||
            talks[i].start >= talks[i].finish) {
            free(talks);
            return 1;
        }
    }

    status = read_token(token, sizeof(token));
    if (status != STATUS_END_OF_INPUT) {
        free(talks);
        return 1;
    }

    status = max_talks(talks, n, k, &answer);
    free(talks);

    if (status != STATUS_OK) {
        return 1;
    }

    printf("%zu\n", answer);

    return 0;
}
