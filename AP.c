#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

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

        number = number * pow_uint64_t(10, 18 - digitCount);

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

void print_AP(AP *ap)
{
        if (ap->whole.count != 0) {
                printf("%ld", ap->whole.limbs[ap->whole.count - 1]);
                for (int32_t i = ap->whole.count - 2; i >= 0; i--) {
                        printf("%ld", ap->whole.limbs[i]);
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
                                && (terminator - i - 1) % 18 == 0) {
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
                if (digitCount != 0 && digitCount % 18 == 0) {
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

int main(void)
{
        AP ap = {0};

        ap = AP_init("123.123");

        print_AP(&ap);

        return 0;
}
