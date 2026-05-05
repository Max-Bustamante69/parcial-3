#include <stdlib.h>
#include <stdint.h>
#include "compression.h"

#ifdef USE_ZLIB
#include <zlib.h>
#endif

int deflate_compress(
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size
) {
#ifdef USE_ZLIB
    uLongf bound;
    uLongf out_len;
    uint8_t *out;
    int rc;

    if (!output || !output_size) {
        return -1;
    }

    if (input_size == 0) {
        *output = NULL;
        *output_size = 0;
        return 0;
    }

    bound = compressBound((uLong)input_size);
    out = (uint8_t *)malloc(bound);
    if (!out) {
        return -1;
    }
    out_len = bound;

    rc = compress2(out, &out_len, input, (uLong)input_size, Z_BEST_SPEED);
    if (rc != Z_OK) {
        free(out);
        return -1;
    }

    *output = out;
    *output_size = (size_t)out_len;
    return 0;
#else
    (void)input;
    (void)input_size;
    (void)output;
    (void)output_size;
    return -2;
#endif
}

int deflate_decompress(
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size,
    size_t expected_size
) {
#ifdef USE_ZLIB
    uLongf out_len = (uLongf)expected_size;
    uint8_t *out;
    int rc;

    if (!output || !output_size || expected_size == 0) {
        return -1;
    }

    out = (uint8_t *)malloc(expected_size);
    if (!out) {
        return -1;
    }

    rc = uncompress(out, &out_len, input, (uLong)input_size);
    if (rc != Z_OK || out_len != expected_size) {
        free(out);
        return -1;
    }

    *output = out;
    *output_size = (size_t)out_len;
    return 0;
#else
    (void)input;
    (void)input_size;
    (void)output;
    (void)output_size;
    (void)expected_size;
    return -2;
#endif
}
