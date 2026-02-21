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

const static uint64_t BIGGEST_NUMBER_USABLE = 9999999999999999999;

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

void append_decimal(Decimal *decimal, uint64_t number, uint64_t digitCount)
{
        if (decimal->count >= decimal->capacity) {
                if (decimal->capacity == 0) {
                        decimal->capacity = INITIAL_COUNT;
                } else {
                        decimal->capacity *= 2;
                }

                decimal->limbs = realloc(decimal->limbs,
                                sizeof(uint64_t) * decimal->capacity);
                if (decimal->limbs == NULL) {
                        fprintf(stderr, "Decimal part allocation failed.\n");
                        return;
                }
        }

        number = number * pow_uint64_t(10, LONGEST_DIGIT - digitCount);

        decimal->limbs[decimal->count++] = number;
}

void append_whole(Whole *whole, uint64_t number)
{
        if (whole->count >= whole->capacity) {
                if (whole->capacity == 0) {
                        whole->capacity = INITIAL_COUNT;
                } else {
                        whole->capacity *= 2;
                }

                whole->limbs = realloc(whole->limbs,
                                sizeof(uint64_t) * whole->capacity);

                if (whole->limbs == NULL) {
                        fprintf(stderr, "Whole part allocation failed.\n");
                        return;
                }
        }

        whole->limbs[whole->count++] = number;
}

void AP_print(AP *ap)
{
        if (ap->whole.count != 0) {
                printf("%ld", ap->whole.limbs[ap->whole.count - 1]);
                for (int32_t i = ap->whole.count - 2; i >= 0; i--) {
                        printf("%019ld", ap->whole.limbs[i]);
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
                printf("%ld", ap->decimal.limbs[i]);
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
                        append_whole(&(result.whole), temp);
                        temp = 0;
                        digitValue = 1;
                }
                temp += (stringView.content[i] - 48) * digitValue;
                digitValue *= 10;
        }

        if (temp != 0) {
                append_whole(&(result.whole), temp);
                temp = 0;
        }

        if (terminator == stringView.count) {
                return result;
        }

        uint32_t digitCount = 0;

        for (uint64_t i = terminator + 1; i < stringView.count; i++) {
                if (digitCount != 0 && digitCount % LONGEST_DIGIT == 0) {
                        append_decimal(&(result.decimal), temp, digitCount);
                        temp = 0;
                        digitCount = 0;
                }
                temp *= 10;
                temp += stringView.content[i] - 48;
                digitCount++;
        }

        if (temp != 0) {
                append_decimal(&(result.decimal), temp, digitCount);
        }

        return result;
}

// If you are going to use this function yourself make sure to
// divide your number into chunks each being 19 numbers long
// :)
void AP_add_to_index_whole(AP *ap, uint64_t number, uint32_t index)
{
        if (ap->whole.capacity > index) {
                if ((uint64_t)BIGGEST_NUMBER_USABLE - ap->whole.limbs[index] > number) {
                        ap->whole.limbs[index] += number;
                } else {
                        ap->whole.limbs[index] = number - (BIGGEST_NUMBER_USABLE - ap->whole.limbs[index]);
                        AP_add_to_index_whole(ap, 1, index + 1);
                }
                return;
        }

        uint32_t max = MAX(ap->whole.capacity * 2, index);

        ap->whole.count = max;
        ap->whole.capacity = max;
        ap->whole.limbs = realloc(ap->whole.limbs, ap->whole.capacity);

        ap->whole.limbs[max - 1] = number; // TODO: fix this stupid assignment error

        return;
}

void AP_add_to_index_decimal(AP *ap, uint64_t number, uint32_t index)
{
        if (ap->decimal.count - 1 > index) {
                if (BIGGEST_NUMBER_USABLE - ap->decimal.limbs[index] > number) {
                        ap->decimal.limbs[index] += number;
                } else {
                        ap->decimal.limbs[index] = number - (BIGGEST_NUMBER_USABLE - ap->decimal.limbs[index]);
                        if (index == 0) {
                                AP_add_to_index_whole(ap, 1, 0);
                        } else {
                                AP_add_to_index_decimal(ap, 1, index - 1);
                        }
                }
                return;
        }

        uint32_t max = MAX(ap->decimal.capacity * 2, index);

        ap->decimal.count = max;
        ap->decimal.capacity = max;
        ap->decimal.limbs = realloc(ap->decimal.limbs, ap->decimal.capacity);

        ap->decimal.limbs[ap->decimal.count - 1] = number;

        return;
}

AP AP_addition(AP *first, AP *second)
{
        AP result = {0};

        for (uint32_t i = 0; i < first->whole.count && second->whole.count; i++) {
                if (BIGGEST_NUMBER_USABLE - first->whole.limbs[i] > second->whole.limbs[i]) {
                        AP_add_to_index_whole(&result, first->whole.limbs[i] + second->whole.limbs[i], i);
                } else {
                        AP_add_to_index_whole(&result, second->whole.count - (BIGGEST_NUMBER_USABLE - first->whole.limbs[i]), i);
                        AP_add_to_index_whole(&result, 1, i);
                }
        }

        if (first->whole.count - 1) {
                for (uint32_t i = second->whole.count; i < first->whole.count; i++) {
                        append_whole(&(result.whole), first->whole.limbs[i]);
                }
        } else {
                for (uint32_t i = first->whole.count; i < second->whole.count; i++) {
                        append_whole(&(result.whole), second->whole.limbs[i]);
                }
        }

        // the whole part is done
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

        first = AP_init("444");
        second = AP_init("555555555555555555555");

        AP_print(&first);
        AP_print(&second);

        //AP result = AP_addition(&first, &second);

        //AP_print(&result);

        AP_free(&first);
        AP_free(&second);

        return 0;
}
