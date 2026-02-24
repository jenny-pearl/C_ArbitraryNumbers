#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define LONGEST_NUMBER 19
#define INITIAL_COUNT 16

typedef struct {
	uint32_t count;
	uint32_t capacity;
	uint64_t *limbs;
} Decimal;

typedef struct {
	uint32_t count;
	uint32_t capacity;
	uint64_t *limbs;
} Whole;

typedef struct {
	Whole whole;
	Decimal decimal;
} AP;

const uint64_t BIGGEST_NUMBER_USABLE = 9999999999999999999ULL;

void AP_append_decimal(AP *ap, uint64_t number, uint32_t leadingZeroCount)
{
	if (ap->decimal.count >= ap->decimal.capacity) {
		if (ap->decimal.capacity == 0) ap->decimal.capacity = INITIAL_COUNT;
		else ap->decimal.capacity *= 2;

		ap->decimal.limbs = realloc(ap->decimal.limbs, sizeof(uint64_t) * ap->decimal.capacity);

		memset(ap->decimal.limbs + ap->decimal.count, 0,
			sizeof(uint64_t) * (ap->decimal.capacity - ap->decimal.count));
	}

	uint32_t digitCount = 0;
	uint64_t digitValue = 1;

	for (; digitValue <= number; digitValue *= 10, digitCount++);

	for (int32_t i = 0; i < LONGEST_NUMBER - digitCount - leadingZeroCount; i++) {
		number *= 10;
	}


	ap->decimal.limbs[ap->decimal.count++] = number;
}

void AP_append_whole(AP *ap, uint64_t number)
{
	if (ap->whole.count >= ap->whole.capacity) {
		if (ap->whole.capacity == 0) ap->whole.capacity = INITIAL_COUNT;
		else ap->whole.capacity *= 2;

		ap->whole.limbs = realloc(ap->whole.limbs, sizeof(uint64_t) * ap->whole.capacity);
		memset(ap->whole.limbs + ap->whole.count, 0,
			sizeof(uint64_t) * (ap->whole.capacity - ap->whole.count));
	}

	ap->whole.limbs[ap->whole.count++] = number;
}

void AP_add_to_index_whole(AP *ap, uint32_t index, uint64_t whole)
{
	if (ap->whole.capacity > index) {
		if (BIGGEST_NUMBER_USABLE - ap->whole.limbs[index] >= whole) {
			ap->whole.limbs[index] += whole;
		} else {
			ap->whole.limbs[index] -= (BIGGEST_NUMBER_USABLE - whole + 1);
			AP_add_to_index_whole(ap, index + 1, 1);
		}

		if (ap->whole.count <= index) {
			ap->whole.count = index + 1;
		}

		return;
	}

	uint32_t oldCount = ap->whole.count;

	uint32_t max = MAX(ap->whole.capacity * 2, index + 1);
	max = MAX(max, INITIAL_COUNT);

	ap->whole.capacity = max;
	ap->whole.count = index + 1;

	ap->whole.limbs = realloc(ap->whole.limbs, ap->whole.capacity * sizeof(uint64_t));

	memset(ap->whole.limbs + oldCount, 0, (ap->whole.capacity - oldCount) * sizeof(uint64_t));

	ap->whole.limbs[index] = whole;

	return;
}

void AP_add_to_index_decimal(AP *ap, uint32_t index, uint64_t decimal, uint32_t leadingZeroCount)
{
	if (ap->decimal.capacity > index) {
		if (BIGGEST_NUMBER_USABLE - ap->decimal.limbs[index] >= decimal) {
			ap->decimal.limbs[index] += decimal;
		} else {
			ap->decimal.limbs[index] -= (BIGGEST_NUMBER_USABLE - decimal + 1);

			if (index != 0) {
				AP_add_to_index_decimal(ap, index - 1, 1, 0);
			} else {
				AP_add_to_index_whole(ap, 0, 1);
			}

		}

		if (ap->decimal.count <= index) {
			ap->decimal.count = index + 1;
		}

		return;
	}

	uint32_t oldCount = ap->decimal.count;

	uint32_t max = MAX(ap->decimal.capacity * 2, index + 1);
	max = MAX(max, INITIAL_COUNT);

	ap->decimal.capacity = max;
	ap->decimal.count = index + 1;

	ap->decimal.limbs = realloc(ap->decimal.limbs, ap->decimal.capacity * sizeof(uint64_t));

	memset(ap->decimal.limbs + oldCount, 0,
		(ap->decimal.capacity - oldCount) * sizeof(uint64_t));

	int32_t digitCount = 0;

	for (uint64_t digitValue = 1; digitValue <= decimal; digitValue *= 10, digitCount++);

	for (int32_t i = 0; i < LONGEST_NUMBER - digitCount - leadingZeroCount; i++) {
		decimal *= 10;
	}

	ap->decimal.limbs[index] = decimal;

	return;
}

AP AP_init(char *format)
{
	AP result = {0};

	if (format == NULL) return result;

	char *dot = format;

	while (*dot != '.' && *dot != '\0') { // Maybe in the future I will check for newlines too
		dot++;
	}

	uint64_t digitValue = 1;
	int32_t index = 1;
	uint64_t temp = 0;

	for (; index <= dot - format; index++, digitValue *= 10) {
		if (index != 1 && (index - 1) % LONGEST_NUMBER == 0) {
			AP_append_whole(&result, temp);
			temp = 0;
			digitValue = 1;
		}
		temp += (uint64_t)(*(dot - index) - 48) * digitValue;
	}

	if (index % LONGEST_NUMBER != 0) {
		AP_append_whole(&result, temp);
	}

	if (*dot == '\0') {
		result.decimal.limbs = malloc(INITIAL_COUNT * sizeof(uint64_t));
		result.decimal.limbs[0] = 0;
		result.decimal.count = 1;
		result.decimal.capacity = 1;
		return result;
	}

	char *terminator = dot;

	while (*terminator != '\0') {
		terminator++;
	}

	index = 1;
	temp = 0;
	digitValue = 1;
	uint32_t leadingZero = 0;
	bool touchedNonZero = false;

	for (; index < terminator - dot; index++, digitValue *= 10) {
		if (index != 1 && (index - 1) % LONGEST_NUMBER == 0) {
			uint32_t digitCount = 0;
			uint64_t compare = 1;

			for (; compare <= temp; compare *= 10, digitCount++);

			AP_append_decimal(&result, temp, leadingZero);

			leadingZero = 0;
			digitValue = 1;
			temp = 0;
		}

		if (!touchedNonZero && *(dot + index) == '0') {
			leadingZero++;
		} else {
			touchedNonZero = true;
		}

		temp *= 10;
		temp += (*(dot + index) - 48);
	}

	if (index % LONGEST_NUMBER != 0) {
		uint32_t digitCount = 0;
		uint64_t compare = 1;
		for (; compare <= temp; compare *= 10, digitCount++);
		AP_append_decimal(&result, temp, leadingZero);
	}

	return result;
}

void AP_print(AP *ap)
{
	printf("%llu", ap->whole.limbs[0]);

	if (ap->whole.count > 1) {
		for (int32_t i = ap->whole.count - 2; i >= 0; i--) {
			printf("%019llu", ap->whole.limbs[i]);
		}
	}

	if (ap->decimal.count == 0) {
		printf("\n");
		return;
	}

	printf(".");

	for (int32_t i = 0; i < ap->decimal.count; i++) {
		printf("%019llu", ap->decimal.limbs[i]);
	}

	printf("\n");

	return;
}

AP AP_addition(AP *first, AP *second)
{
	AP result = {0};

	uint32_t shortest = MIN(first->whole.count, second->whole.count);

	for (int32_t i = 0; i < shortest; i++) {
		if (BIGGEST_NUMBER_USABLE - first->whole.limbs[i] >= second->whole.limbs[i]) {
			AP_add_to_index_whole(&result, i,
				first->whole.limbs[i] + second->whole.limbs[i]);
		} else {
			AP_add_to_index_whole(&result, i, first->whole.limbs[i] -
				(BIGGEST_NUMBER_USABLE - second->whole.limbs[i] + 1));
			AP_add_to_index_whole(&result, i + 1, 1);
		}
	}

	if (shortest < first->whole.count) {
		for (uint32_t i = shortest; i < first->whole.count; i++) {
			if (i < result.whole.count) {
				AP_add_to_index_whole(&result, i, first->whole.limbs[i]);
			} else {
				AP_append_whole(&result, first->whole.limbs[i]);
			}
		}
	} else if (shortest < second->whole.count) {
		for (uint32_t i = shortest; i < second->whole.count; i++) {
			if (i < result.whole.count) {
				AP_add_to_index_whole(&result, i, second->whole.limbs[i]);
			} else {
				AP_append_whole(&result, second->whole.limbs[i]);
			}
		}
	}

	if (first->decimal.count == 0 && second->decimal.count == 0) {
		return result;
	}

	shortest = MIN(first->decimal.count, second->decimal.count);

	uint64_t digitValue = 1;
	uint32_t digitCount = 0;

	for (int32_t i = shortest - 1; i >= 0; i--) {
		if (BIGGEST_NUMBER_USABLE - first->decimal.limbs[i] >= second->decimal.limbs[i]) {
			uint64_t temp = first->decimal.limbs[i] + second->decimal.limbs[i];
			for (; digitValue <= temp; digitValue *= 10, digitCount++);
			AP_add_to_index_decimal(&result, i, temp, LONGEST_NUMBER - digitCount);
			digitValue = 1;
			digitCount = 0;
		} else {
			uint64_t temp = first->decimal.limbs[i] -
				(BIGGEST_NUMBER_USABLE - second->decimal.limbs[i] + 1);
			for (; digitValue <= temp; digitValue *= 10, digitCount++);
			AP_add_to_index_decimal(&result, i, temp, LONGEST_NUMBER - digitCount);
			digitValue = 1;
			digitCount = 0;
			if (i != 0) {
				AP_add_to_index_decimal(&result, i - 1, 1, LONGEST_NUMBER - 1);
			} else {
				AP_add_to_index_whole(&result, 0, 1);
			}
		}
	}

	if (shortest < first->decimal.count) {
		for (uint32_t i = shortest; i < first->decimal.count; i++) {
			for (; digitValue <= first->decimal.limbs[i];
				digitValue *= 10, digitCount++);
			if (i < result.decimal.count) {
				AP_add_to_index_decimal(&result, i,
					first->decimal.limbs[i], LONGEST_NUMBER - digitCount);
			} else {
				AP_append_decimal(&result, first->decimal.limbs[i],
					LONGEST_NUMBER - digitCount);
			}
			digitValue = 1;
			digitCount = 0;
		}
	} else if (shortest < second->decimal.count) {
		for (uint32_t i = shortest; i < second->decimal.count; i++) {
			for (; digitValue <= first->decimal.limbs[i];
				digitValue *= 10, digitCount++);
			if (i < result.decimal.count) {
				AP_add_to_index_decimal(&result, i, second->decimal.limbs[i],
					LONGEST_NUMBER - digitCount);
			} else {
				AP_append_decimal(&result, second->decimal.limbs[i],
					LONGEST_NUMBER - digitCount);
			}
			digitValue = 1;
			digitCount = 0;
		}
	}

	return result;
}

int main(void)
{
	AP delta = AP_init("0.0000000000000000001");
	AP cumulative = AP_init("0");

	for (uint32_t i = 0; i < 10000; i++) {
		cumulative = AP_addition(&cumulative, &delta);
		AP_print(&cumulative);
	}

	return 0;
}
