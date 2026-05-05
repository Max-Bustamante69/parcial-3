#include <stdlib.h>
#include <stdint.h>
#include "compression.h"

int rle_compress(
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size
) {
    size_t i;
    size_t out_idx = 0;
    uint8_t *out;

    if (!output || !output_size) {
        return -1;
    }

    if (input_size == 0) {
        *output = NULL;
        *output_size = 0;
        return 0;
    }

    out = (uint8_t *)malloc(input_size * 2);
    if (!out) {
        return -1;
    }

    i = 0;
    while (i < input_size) {
        uint8_t value = input[i];
        uint8_t run = 1;
        while (i + run < input_size && input[i + run] == value && run < 255) {
            run++;
        }
        out[out_idx++] = run;
        out[out_idx++] = value;
        i += run;
    }

    *output = out;
    *output_size = out_idx;
    return 0;
}

int rle_decompress(
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size,
    size_t expected_size
) {
    size_t i;
    size_t out_idx = 0;
    uint8_t *out;

    if (!output || !output_size) {
        return -1;
    }

    if (input_size == 0) {
        *output = NULL;
        *output_size = 0;
        return 0;
    }

    if ((input_size % 2) != 0) {
        return -1;
    }

    out = (uint8_t *)malloc(expected_size > 0 ? expected_size : input_size * 2);
    if (!out) {
        return -1;
    }

    for (i = 0; i < input_size; i += 2) {
        uint8_t run = input[i];
        uint8_t value = input[i + 1];
        uint8_t j;

        for (j = 0; j < run; j++) {
            out[out_idx++] = value;
        }
    }

    if (expected_size > 0 && out_idx != expected_size) {
        free(out);
        return -1;
    }

    *output = out;
    *output_size = out_idx;
    return 0;
}
