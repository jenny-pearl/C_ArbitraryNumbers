// TODO: add primitive arithmetic operation
//
// complete addition
// add subtraction, multiplication and division

#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define LONGEST_DIGIT 19
#define INITIAL_COUNT 10

static const unsigned long long BIGGEST_NUMBER_USED = 9999999999999999999ULL;

typedef struct {
        uint64_t count;
        const char *content;
} StringView;

typedef struct {
        uint32_t count;
        uint32_t capacity;
        uint64_t *limbs;
} Whole;

typedef struct {
        uint32_t count;
        uint32_t capacity;
        uint64_t *limbs;
} Decimal;

typedef struct {
        Whole whole;
        Decimal decimal;
} AP;

uint64_t pow_uint64_t(uint64_t number, uint64_t power)
{
        uint64_t result = 1;

        for (uint64_t i = 0; i < power; i++) {
                result *= number;
        }

        return result;
}

StringView c_string_into_view(char *format)
{
        uint64_t i = 0;

        while (format[i] != '\0') { i++; }

        return (StringView){ .content = format, .count = i };
}

void append_decimal(AP *ap, uint64_t number, uint64_t digitCount)
{
        if (ap->decimal.count >= ap->decimal.capacity) {
                if (ap->decimal.capacity == 0) {
                        ap->decimal.capacity = INITIAL_COUNT;
                } else {
                        ap->decimal.capacity *= 2;
                }

                ap->decimal.limbs = realloc(ap->decimal.limbs,
                                sizeof(uint64_t) * ap->decimal.capacity);
                if (ap->decimal.limbs == NULL) {
                        fprintf(stderr, "Decimal part allocation failed.\n");
                        return;
                }
        }

        number = number * pow_uint64_t(10, LONGEST_DIGIT - digitCount);

        ap->decimal.limbs[ap->decimal.count++] = number;
}

void append_whole(AP *ap, uint64_t number)
{
        if (ap->whole.count >= ap->whole.capacity) {
                if (ap->whole.capacity == 0) {
                        ap->whole.capacity = INITIAL_COUNT;
                } else {
                        ap->whole.capacity *= 2;
                }

                ap->whole.limbs = realloc(ap->whole.limbs,
                                sizeof(uint64_t) * ap->whole.capacity);

                if (ap->whole.limbs == NULL) {
                        fprintf(stderr, "Whole part allocation failed.\n");
                        return;
                }
        }

        ap->whole.limbs[ap->whole.count++] = number;
}

void AP_print(AP *ap)
{
        if (ap->whole.count != 0) {
                printf("%lu", ap->whole.limbs[ap->whole.count - 1]);
                for (int32_t i = ap->whole.count - 2; i >= 0; i--) {
                        printf("%019lu", ap->whole.limbs[i]);
                }
        } else {
                printf("0");
        }

        if (ap->decimal.count == 0) {
                printf("\n");
                return;
        }

        printf(".");

        for (uint32_t i = 0; i < ap->decimal.count; i++) {
                printf("%lu", ap->decimal.limbs[i]);
        }

        printf("\n");
}

AP AP_init(char *format)
{
        AP result = {0};
        uint64_t terminator = 0;
        uint64_t temp = 0;

        StringView stringView = c_string_into_view(format);

        while (stringView.content[terminator] != '.'
                        && terminator < stringView.count) {
                terminator++;
        }

        for (int64_t digitValue = 1, i = terminator - 1;
                        i >= 0; i--) {
                if ((terminator - i - 1 > 0)
                                && (terminator - i - 1) % LONGEST_DIGIT == 0) {
                        append_whole(&result, temp);
                        temp = 0;
                        digitValue = 1;
                }
                temp += (stringView.content[i] - 48) * digitValue;
                digitValue *= 10;
        }

        if (temp != 0) {
                append_whole(&result, temp);
                temp = 0;
        }

        if (terminator == stringView.count) {
                return result;
        }

        uint32_t digitCount = 0;

        for (uint64_t i = terminator + 1; i < stringView.count; i++) {
                if (digitCount != 0 && digitCount % LONGEST_DIGIT == 0) {
                        append_decimal(&result, temp, digitCount);
                        temp = 0;
                        digitCount = 0;
                }
                temp *= 10;
                temp += stringView.content[i] - 48;
                digitCount++;
        }

        if (temp != 0) {
                append_decimal(&result, temp, digitCount);
        }

        return result;
}

void AP_add_to_index_whole(AP *ap, uint64_t number, uint32_t index)
{
        if (ap->whole.capacity > index) {
                if (BIGGEST_NUMBER_USED - ap->whole.limbs[index] >= number) {
                        ap->whole.limbs[index] += number;
                } else {
                        ap->whole.limbs[index] = number - (BIGGEST_NUMBER_USED - ap->whole.limbs[index] + 1);
                        AP_add_to_index_whole(ap, 1, index + 1);
                }
                return;
        }

        uint32_t oldCount = ap->whole.count;

        uint32_t max = MAX(INITIAL_COUNT, index + 1);
        max = MAX(ap->decimal.capacity * 2, max);

        ap->whole.count = index + 1;
        ap->whole.capacity = max;
        ap->whole.limbs = realloc(ap->whole.limbs, sizeof(uint64_t) * ap->whole.capacity);

        while (oldCount < ap->decimal.capacity) {
                ap->decimal.limbs[oldCount] = 0;
                oldCount++;
        }

        ap->whole.limbs[index] = number;

        return;
}

void AP_add_to_index_decimal(AP *ap, uint64_t number, uint32_t index)
{
        if (ap->decimal.capacity > index) {
                if (BIGGEST_NUMBER_USED - ap->decimal.limbs[index] >= number) {
                        ap->decimal.limbs[index] += number;
                } else {
                        ap->decimal.limbs[index] = number -
                                (BIGGEST_NUMBER_USED - ap->decimal.limbs[index]);
                        if (index == 0) {
                                AP_add_to_index_whole(ap, 1, 0);
                        } else {
                                AP_add_to_index_decimal(ap, 1, index - 1);
                        }
                }
                return;
        }

        uint32_t oldCount = ap->decimal.count;

        uint32_t max = MAX(INITIAL_COUNT, index + 1);
        max = MAX(ap->decimal.capacity * 2, max);

        ap->decimal.count = index + 1;
        ap->decimal.capacity = max;
        ap->decimal.limbs = realloc(ap->decimal.limbs, sizeof(uint64_t) * ap->decimal.capacity);

        while (oldCount < ap->decimal.capacity) {
                ap->decimal.limbs[oldCount] = 0;
                oldCount++;
        }

        ap->decimal.limbs[index] = number;

        return;
}

AP AP_addition(AP *first, AP *second)
{
        AP result = {0};

        uint32_t secondCount = second->whole.count;
        uint32_t firstCount = first->whole.count;
        uint32_t index = 0;

        for (; index < firstCount && index < secondCount; index++) {
                if (BIGGEST_NUMBER_USED - first->whole.limbs[index]
                                >= second->whole.limbs[index]) {
                        AP_add_to_index_whole(&result,
                                        first->whole.limbs[index]
                                        + second->whole.limbs[index],
                                        index);
                } else {
                        AP_add_to_index_whole(&result,
                                        second->whole.limbs[index]
                                        - (BIGGEST_NUMBER_USED - first->whole.limbs[index] + 1),
                                        index);
                        AP_add_to_index_whole(&result, 1, index + 1);
                }
        }

        if (firstCount != index) {
                for (uint32_t i = second->whole.count; i < first->whole.count; i++) {
                        append_whole(&result, first->whole.limbs[i]);
                }
        } else if (secondCount != index) {
                for (uint32_t i = first->whole.count; i < second->whole.count; i++) {
                        append_whole(&result, second->whole.limbs[i]);
                }
        }

        if (first->decimal.count == 0 && second->decimal.count == 0) {
                return result;
        }

        firstCount = first->decimal.count;
        secondCount = second->decimal.count;

        AP *longest = first->decimal.count > second->decimal.count ? first : second;
        index = MIN(first->decimal.count, second->decimal.count);
        uint32_t anchor = index;

        for (; index > 0; index--) {
                if (BIGGEST_NUMBER_USED - first->decimal.limbs[index - 1]
                                >= second->decimal.limbs[index - 1]) {
                        AP_add_to_index_decimal(&result, first->decimal.limbs[index - 1]
                                        + second->decimal.limbs[index - 1],
                                        index - 1);
                } else {
                        printf("index: %d\n", index);
                        AP_add_to_index_decimal(&result, second->decimal.limbs[index - 1]
                                        - (BIGGEST_NUMBER_USED - first->decimal.limbs[index - 1] + 1),
                                        index - 1);
                        if (index == 1) {
                                AP_add_to_index_whole(&result, 1, 0);
                        } else {
                                AP_add_to_index_decimal(&result, 1, index - 2);
                        }
                }
        }

        for (; anchor < longest->decimal.count; anchor++) {
                uint64_t digitValue;
                uint32_t digitCount;
                for (digitCount = 0, digitValue = 1;
                                digitValue < longest->decimal.limbs[anchor];
                                digitValue *= 10, digitCount++);
                append_decimal(&result, longest->decimal.limbs[anchor], digitCount);
        }

        return result;
}

AP AP_subtraction(AP *first, AP *second)
{
        AP result = {0};
        return result;
}

void AP_free(AP *ap)
{
        if (ap->whole.limbs) {
                free(ap->whole.limbs);
        }

        if (ap->decimal.limbs) {
                free(ap->decimal.limbs);
        }
}

int main(void)
{
        AP first = {0};
        AP second = {0};

        first = AP_init("0.555");
        second = AP_init("0.555");

        AP_print(&first);
        AP_print(&second);

        AP result = AP_addition(&first, &second);

        AP_print(&result);

        AP_free(&first);
        AP_free(&second);
        AP_free(&result);

        return 0;
}
