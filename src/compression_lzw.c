#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "compression.h"

#define LZW_MAX_DICT 4096

typedef struct {
    int prefix;
    uint8_t value;
} lzw_entry_t;

static int lzw_find(const lzw_entry_t *dict, int dict_size, int prefix, uint8_t value) {
    int i;
    for (i = 256; i < dict_size; i++) {
        if (dict[i].prefix == prefix && dict[i].value == value) {
            return i;
        }
    }
    return -1;
}

int lzw_compress(
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size
) {
    lzw_entry_t dict[LZW_MAX_DICT];
    uint16_t *codes;
    size_t code_count = 0;
    int dict_size = 256;
    size_t i;
    int w;
    uint8_t *out;
    size_t out_size;

    if (!output || !output_size) {
        return -1;
    }

    if (input_size == 0) {
        *output = NULL;
        *output_size = 0;
        return 0;
    }

    codes = (uint16_t *)malloc(sizeof(uint16_t) * (input_size + 1));
    if (!codes) {
        return -1;
    }

    w = input[0];
    for (i = 1; i < input_size; i++) {
        uint8_t k = input[i];
        int wk = lzw_find(dict, dict_size, w, k);
        if (wk >= 0) {
            w = wk;
        } else {
            codes[code_count++] = (uint16_t)w;
            if (dict_size < LZW_MAX_DICT) {
                dict[dict_size].prefix = w;
                dict[dict_size].value = k;
                dict_size++;
            }
            w = k;
        }
    }
    codes[code_count++] = (uint16_t)w;

    out_size = code_count * sizeof(uint16_t);
    out = (uint8_t *)malloc(out_size);
    if (!out) {
        free(codes);
        return -1;
    }
    memcpy(out, codes, out_size);
    free(codes);

    *output = out;
    *output_size = out_size;
    return 0;
}

static int lzw_build_string(
    const lzw_entry_t *dict,
    int code,
    uint8_t *stack,
    int *stack_len
) {
    int len = 0;
    int current = code;

    while (current >= 256) {
        if (len >= LZW_MAX_DICT - 1) {
            return -1;
        }
        stack[len++] = dict[current].value;
        current = dict[current].prefix;
        if (current < 0) {
            return -1;
        }
    }

    stack[len++] = (uint8_t)current;
    *stack_len = len;
    return 0;
}

int lzw_decompress(
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size,
    size_t expected_size
) {
    lzw_entry_t dict[LZW_MAX_DICT] = {{0, 0}};
    const uint16_t *codes = (const uint16_t *)input;
    size_t code_count = input_size / sizeof(uint16_t);
    uint8_t *out;
    size_t out_idx = 0;
    int dict_size = 256;
    size_t i;
    int old_code;
    uint8_t stack[LZW_MAX_DICT];
    int stack_len;
    uint8_t first_char;

    if (!output || !output_size) {
        return -1;
    }

    if (input_size == 0) {
        *output = NULL;
        *output_size = 0;
        return 0;
    }

    if ((input_size % sizeof(uint16_t)) != 0 || code_count == 0) {
        return -1;
    }

    out = (uint8_t *)malloc(expected_size > 0 ? expected_size : input_size * 4);
    if (!out) {
        return -1;
    }

    old_code = codes[0];
    if (old_code < 0 || old_code > 255) {
        free(out);
        return -1;
    }
    if (lzw_build_string(dict, old_code, stack, &stack_len) != 0) {
        free(out);
        return -1;
    }
    first_char = stack[stack_len - 1];
    while (stack_len > 0) {
        out[out_idx++] = stack[--stack_len];
    }

    for (i = 1; i < code_count; i++) {
        int new_code = codes[i];
        int use_code = new_code;

        if (new_code > dict_size) {
            free(out);
            return -1;
        }

        if (new_code == dict_size) {
            use_code = old_code;
        }

        if (lzw_build_string(dict, use_code, stack, &stack_len) != 0) {
            free(out);
            return -1;
        }

        first_char = stack[stack_len - 1];

        while (stack_len > 0) {
            if (expected_size > 0 && out_idx >= expected_size) {
                break;
            }
            out[out_idx++] = stack[--stack_len];
        }

        if (new_code >= dict_size) {
            if (expected_size > 0 && out_idx < expected_size) {
                out[out_idx++] = first_char;
            } else if (expected_size == 0) {
                out[out_idx++] = first_char;
            }
        }

        if (dict_size < LZW_MAX_DICT) {
            dict[dict_size].prefix = old_code;
            dict[dict_size].value = first_char;
            dict_size++;
        }

        old_code = new_code;
    }

    if (expected_size > 0 && out_idx != expected_size) {
        free(out);
        return -1;
    }

    *output = out;
    *output_size = out_idx;
    return 0;
}
