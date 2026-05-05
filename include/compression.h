#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <stddef.h>
#include <stdint.h>
#include "file_format.h"

int compress_data(
    compression_algo_t algo,
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size
);

int decompress_data(
    compression_algo_t algo,
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size,
    size_t expected_plain_size
);

#endif
