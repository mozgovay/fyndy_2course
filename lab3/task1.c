#include <stdio.h>
#include <limits.h>
#include <stddef.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_INPUT,
    STATUS_INVALID_R,
    STATUS_BUFFER_TOO_SMALL
} Status;

Status validate_number(const char *str, long long *number) {
    unsigned long long value = 0;
    unsigned long long limit;
    int sign = 1;
    size_t i = 0;

    if (str == NULL || number == NULL || str[0] == '\0') {
        return STATUS_INVALID_INPUT;
    }

    if (str[i] == '-') {
        sign = -1; 
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    if (str[i] == '\0') {
        return STATUS_INVALID_INPUT;
    }

    if (sign < 0) {
        limit = (unsigned long long)LLONG_MAX + 1ULL;
    } else {
        limit = (unsigned long long)LLONG_MAX;
    }

    while (str[i] != '\0') {
        unsigned long long digit;

        if (str[i] < '0' || str[i] > '9') {
            return STATUS_INVALID_INPUT;
        }

        digit = (unsigned long long)(str[i] - '0');

        if (value > (limit - digit) / 10ULL) {
            return STATUS_INVALID_INPUT;
        }

        value = value * 10ULL + digit;
        i++;
    }

    if (sign < 0) {
        if (value == (unsigned long long)LLONG_MAX + 1ULL) {
            *number = LLONG_MIN;
        } else {
            *number = -(long long)value;
        }
    } else {
        *number = (long long)value;
    }

    return STATUS_OK;
}

Status validate_r(int r) {
    if (r < 1 || r > 5) {
        return STATUS_INVALID_R;
    }

    return STATUS_OK;
}

Status convert_number(
    long long number,
    int r,
    char *result,
    size_t size
) {
    const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    unsigned long long value;
    unsigned long long mask;
    unsigned long long digit;

    char temp[sizeof(unsigned long long) * CHAR_BIT + 1];

    size_t count = 0;
    size_t i;
    int negative = 0;

    if (result == NULL || size == 0) {
        return STATUS_INVALID_INPUT;
    }

    if (validate_r(r) != STATUS_OK) {
        return STATUS_INVALID_R;
    }

    if (number < 0) {
        negative = 1;
        value = 0ULL - (unsigned long long)number;
    } else {
        value = (unsigned long long)number;
    }

    mask = (1ULL << (unsigned int)r) - 1ULL;

    if (value == 0) {
        if (size < 2) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        result[0] = '0';
        result[1] = '\0';

        return STATUS_OK;
    }

    while (value != 0) {
        digit = value & mask;

        temp[count] = digits[digit];
        count++;

        value >>= (unsigned int)r;
    }

    if (size <= count + (size_t)negative) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    for (i = 0; i < count; i++) {
        result[i + (size_t)negative] = temp[count - 1 - i];
    }

    if (negative) {
        result[0] = '-';
    }

    result[count + (size_t)negative] = '\0';

    return STATUS_OK;
}

int main(void) {
    char input[128];

    long long number;
    int r;

    char result[sizeof(unsigned long long) * CHAR_BIT + 2];

    Status status;

    printf("Enter a decimal number: ");

    if (scanf("%127s", input) != 1) {
        printf("Error: invalid number input.\n");
        return 1;
    }

    status = validate_number(input, &number);

    if (status != STATUS_OK) {
        printf("Error: invalid number or number is out of range.\n");
        return 1;
    }

    printf("Enter r (1-5): ");

    if (scanf("%127s", input) != 1) {
        printf("Error: invalid r input.\n");
        return 1;
    }

    if (input[0] < '1' || input[0] > '5' || input[1] != '\0') {
        printf("Error: r must be from 1 to 5.\n");
        return 1;
    }

    r = input[0] - '0';

    status = validate_r(r);

    if (status != STATUS_OK) {
        printf("Error: invalid r.\n");
        return 1;
    }

    status = convert_number(number, r, result, sizeof(result));

    switch (status) {
        case STATUS_OK:
            printf("Result: %s\n", result);
            break;

        case STATUS_INVALID_INPUT:
            printf("Error: invalid input parameters.\n");
            break;

        case STATUS_INVALID_R:
            printf("Error: r must be from 1 to 5.\n");
            break;

        case STATUS_BUFFER_TOO_SMALL:
            printf("Error: result buffer is too small.\n");
            break;

        default:
            printf("Unknown error.\n");
            return 1;
    }

    return status == STATUS_OK ? 0 : 1;
}
