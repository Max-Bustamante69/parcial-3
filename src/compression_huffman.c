#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "compression.h"

#define HUFF_HEADER_SIZE (256 * sizeof(uint32_t) + sizeof(uint32_t))

typedef struct huff_node {
    uint32_t freq;
    int symbol;
    int left;
    int right;
} huff_node_t;

typedef struct {
    char bits[256];
    int len;
} huff_code_t;

static void build_codes(
    const huff_node_t *nodes,
    int idx,
    huff_code_t codes[256],
    char *path,
    int depth
) {
    if (idx < 0) {
        return;
    }

    if (nodes[idx].left < 0 && nodes[idx].right < 0) {
        int symbol = nodes[idx].symbol;
        int i;
        if (depth == 0) {
            codes[symbol].bits[0] = '0';
            codes[symbol].len = 1;
            return;
        }
        for (i = 0; i < depth; i++) {
            codes[symbol].bits[i] = path[i];
        }
        codes[symbol].len = depth;
        return;
    }

    if (nodes[idx].left >= 0) {
        path[depth] = '0';
        build_codes(nodes, nodes[idx].left, codes, path, depth + 1);
    }
    if (nodes[idx].right >= 0) {
        path[depth] = '1';
        build_codes(nodes, nodes[idx].right, codes, path, depth + 1);
    }
}

static int build_tree_from_freq(
    const uint32_t freq[256],
    huff_node_t nodes[512],
    int *root_idx,
    int *node_count
) {
    int i;
    int active[512];
    int active_count = 0;
    int count = 0;

    for (i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            nodes[count].freq = freq[i];
            nodes[count].symbol = i;
            nodes[count].left = -1;
            nodes[count].right = -1;
            active[active_count++] = count;
            count++;
        }
    }

    if (active_count == 0) {
        return -1;
    }

    while (active_count > 1) {
        int min1 = 0;
        int min2 = 1;
        int a;
        int left;
        int right;
        int new_node;

        if (nodes[active[min2]].freq < nodes[active[min1]].freq) {
            int tmp = min1;
            min1 = min2;
            min2 = tmp;
        }

        for (a = 2; a < active_count; a++) {
            if (nodes[active[a]].freq < nodes[active[min1]].freq) {
                min2 = min1;
                min1 = a;
            } else if (nodes[active[a]].freq < nodes[active[min2]].freq) {
                min2 = a;
            }
        }

        left = active[min1];
        right = active[min2];

        if (min1 > min2) {
            int tmp = min1;
            min1 = min2;
            min2 = tmp;
        }

        for (a = min2; a < active_count - 1; a++) {
            active[a] = active[a + 1];
        }
        active_count--;
        for (a = min1; a < active_count - 1; a++) {
            active[a] = active[a + 1];
        }
        active_count--;

        new_node = count++;
        nodes[new_node].freq = nodes[left].freq + nodes[right].freq;
        nodes[new_node].symbol = -1;
        nodes[new_node].left = left;
        nodes[new_node].right = right;
        active[active_count++] = new_node;
    }

    *root_idx = active[0];
    *node_count = count;
    return 0;
}

int huffman_compress(
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size
) {
    uint32_t freq[256] = {0};
    huff_node_t nodes[512];
    huff_code_t codes[256] = {{{0}, 0}};
    char path[256];
    int root_idx;
    int node_count;
    size_t i;
    uint32_t total_bits = 0;
    size_t bit_bytes;
    uint8_t *out;
    size_t out_size;
    uint8_t *bitstream;
    size_t bit_pos = 0;

    if (!output || !output_size) {
        return -1;
    }

    if (input_size == 0) {
        *output = NULL;
        *output_size = 0;
        return 0;
    }

    for (i = 0; i < input_size; i++) {
        freq[input[i]]++;
    }

    if (build_tree_from_freq(freq, nodes, &root_idx, &node_count) != 0) {
        return -1;
    }

    (void)node_count;
    build_codes(nodes, root_idx, codes, path, 0);

    for (i = 0; i < input_size; i++) {
        total_bits += (uint32_t)codes[input[i]].len;
    }

    bit_bytes = (total_bits + 7u) / 8u;
    out_size = HUFF_HEADER_SIZE + bit_bytes;
    out = (uint8_t *)calloc(out_size, 1);
    if (!out) {
        return -1;
    }

    memcpy(out, freq, 256 * sizeof(uint32_t));
    memcpy(out + 256 * sizeof(uint32_t), &total_bits, sizeof(uint32_t));
    bitstream = out + HUFF_HEADER_SIZE;

    for (i = 0; i < input_size; i++) {
        const huff_code_t *code = &codes[input[i]];
        int b;
        for (b = 0; b < code->len; b++) {
            if (code->bits[b] == '1') {
                bitstream[bit_pos / 8] |= (uint8_t)(1u << (bit_pos % 8));
            }
            bit_pos++;
        }
    }

    *output = out;
    *output_size = out_size;
    return 0;
}

int huffman_decompress(
    const uint8_t *input,
    size_t input_size,
    uint8_t **output,
    size_t *output_size,
    size_t expected_size
) {
    uint32_t freq[256];
    uint32_t total_bits;
    huff_node_t nodes[512];
    int root_idx;
    int node_count;
    const uint8_t *bitstream;
    size_t bit_pos = 0;
    uint8_t *out;
    size_t out_idx = 0;

    if (!output || !output_size) {
        return -1;
    }

    if (input_size == 0) {
        *output = NULL;
        *output_size = 0;
        return 0;
    }

    if (input_size < HUFF_HEADER_SIZE) {
        return -1;
    }

    memcpy(freq, input, 256 * sizeof(uint32_t));
    memcpy(&total_bits, input + 256 * sizeof(uint32_t), sizeof(uint32_t));

    if (build_tree_from_freq(freq, nodes, &root_idx, &node_count) != 0) {
        return -1;
    }

    (void)node_count;
    bitstream = input + HUFF_HEADER_SIZE;
    out = (uint8_t *)malloc(expected_size > 0 ? expected_size : total_bits);
    if (!out) {
        return -1;
    }

    while (bit_pos < total_bits && out_idx < expected_size) {
        int current = root_idx;
        while (nodes[current].left >= 0 || nodes[current].right >= 0) {
            int bit = (bitstream[bit_pos / 8] >> (bit_pos % 8)) & 1;
            bit_pos++;
            current = bit ? nodes[current].right : nodes[current].left;
            if (current < 0) {
                free(out);
                return -1;
            }
        }
        out[out_idx++] = (uint8_t)nodes[current].symbol;
    }

    if (expected_size > 0 && out_idx != expected_size) {
        free(out);
        return -1;
    }

    *output = out;
    *output_size = out_idx;
    return 0;
}
