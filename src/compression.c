#include <stdlib.h>
#include "compression.h"

int rle_compress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size);
int rle_decompress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size, size_t expected_size);
int huffman_compress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size);
int huffman_decompress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size, size_t expected_size);
int lzw_compress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size);
int lzw_decompress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size, size_t expected_size);
int deflate_compress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size);
int deflate_decompress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size, size_t expected_size);

int compress_data(
    compression_algo_t algo,
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size
) {
    switch (algo) {
        case COMP_RLE:
            return rle_compress(input, input_size, output, output_size);
        case COMP_HUFFMAN:
            return huffman_compress(input, input_size, output, output_size);
        case COMP_LZW:
            return lzw_compress(input, input_size, output, output_size);
        case COMP_DEFLATE:
            return deflate_compress(input, input_size, output, output_size);
        default:
            return -1;
    }
}

int decompress_data(
    compression_algo_t algo,
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size,
    size_t expected_plain_size
) {
    switch (algo) {
        case COMP_RLE:
            return rle_decompress(input, input_size, output, output_size, expected_plain_size);
        case COMP_HUFFMAN:
            return huffman_decompress(input, input_size, output, output_size, expected_plain_size);
        case COMP_LZW:
            return lzw_decompress(input, input_size, output, output_size, expected_plain_size);
        case COMP_DEFLATE:
            return deflate_decompress(input, input_size, output, output_size, expected_plain_size);
        default:
            return -1;
    }
}
