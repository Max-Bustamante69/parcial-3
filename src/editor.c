#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "editor.h"
#include "compression.h"
#include "io_backend.h"

static char *dup_string(const char *src) {
    size_t len = strlen(src);
    char *dst = (char *)malloc(len + 1);
    if (!dst) {
        return NULL;
    }
    memcpy(dst, src, len + 1);
    return dst;
}

static int ensure_capacity(text_buffer_t *tb, size_t min_capacity) {
    char **new_lines;
    size_t new_capacity = tb->capacity == 0 ? 8 : tb->capacity;

    while (new_capacity < min_capacity) {
        new_capacity *= 2;
    }

    if (new_capacity == tb->capacity) {
        return 0;
    }

    new_lines = (char **)realloc(tb->lines, new_capacity * sizeof(char *));
    if (!new_lines) {
        return -1;
    }

    tb->lines = new_lines;
    tb->capacity = new_capacity;
    return 0;
}

void text_buffer_init(text_buffer_t *tb) {
    tb->lines = NULL;
    tb->count = 0;
    tb->capacity = 0;
}

void text_buffer_free(text_buffer_t *tb) {
    size_t i;
    for (i = 0; i < tb->count; i++) {
        free(tb->lines[i]);
    }
    free(tb->lines);
    tb->lines = NULL;
    tb->count = 0;
    tb->capacity = 0;
}

int text_buffer_append(text_buffer_t *tb, const char *line) {
    char *copy;
    if (ensure_capacity(tb, tb->count + 1) != 0) {
        return -1;
    }
    copy = dup_string(line);
    if (!copy) {
        return -1;
    }
    tb->lines[tb->count++] = copy;
    return 0;
}

int text_buffer_insert(text_buffer_t *tb, size_t index, const char *line) {
    size_t i;
    char *copy;
    if (index > tb->count) {
        return -1;
    }
    if (ensure_capacity(tb, tb->count + 1) != 0) {
        return -1;
    }
    for (i = tb->count; i > index; i--) {
        tb->lines[i] = tb->lines[i - 1];
    }
    copy = dup_string(line);
    if (!copy) {
        return -1;
    }
    tb->lines[index] = copy;
    tb->count++;
    return 0;
}

int text_buffer_delete(text_buffer_t *tb, size_t index) {
    size_t i;
    if (index >= tb->count) {
        return -1;
    }
    free(tb->lines[index]);
    for (i = index; i + 1 < tb->count; i++) {
        tb->lines[i] = tb->lines[i + 1];
    }
    tb->count--;
    return 0;
}

void text_buffer_print(const text_buffer_t *tb) {
    size_t i;
    for (i = 0; i < tb->count; i++) {
        printf("%llu: %s\n", (unsigned long long)i, tb->lines[i]);
    }
}

char *text_buffer_join(const text_buffer_t *tb, size_t *out_size) {
    size_t i;
    size_t total = 0;
    size_t offset = 0;
    char *joined;

    for (i = 0; i < tb->count; i++) {
        total += strlen(tb->lines[i]) + 1;
    }

    if (total == 0) {
        joined = (char *)malloc(1);
        if (!joined) {
            return NULL;
        }
        joined[0] = '\0';
        if (out_size) {
            *out_size = 0;
        }
        return joined;
    }

    joined = (char *)malloc(total);
    if (!joined) {
        return NULL;
    }

    for (i = 0; i < tb->count; i++) {
        size_t len = strlen(tb->lines[i]);
        memcpy(joined + offset, tb->lines[i], len);
        offset += len;
        if (i + 1 < tb->count) {
            joined[offset++] = '\n';
        }
    }

    if (out_size) {
        *out_size = offset;
    }
    return joined;
}

int text_buffer_load_plain(text_buffer_t *tb, const char *plain, size_t size) {
    size_t start = 0;
    size_t i;

    text_buffer_free(tb);
    text_buffer_init(tb);

    if (size == 0) {
        return 0;
    }

    for (i = 0; i <= size; i++) {
        if (i == size || plain[i] == '\n') {
            size_t len = i - start;
            char *line = (char *)malloc(len + 1);
            if (!line) {
                return -1;
            }
            memcpy(line, plain + start, len);
            line[len] = '\0';
            if (ensure_capacity(tb, tb->count + 1) != 0) {
                free(line);
                return -1;
            }
            tb->lines[tb->count++] = line;
            start = i + 1;
        }
    }

    return 0;
}

int editor_save_file(
    const text_buffer_t *tb,
    const char *path,
    compression_algo_t algo,
    io_mode_t io_mode
) {
    char *plain = NULL;
    size_t plain_size = 0;
    uint8_t *compressed = NULL;
    size_t compressed_size = 0;
    editor_file_header_t header;
    uint8_t *full = NULL;
    size_t full_size;
    int rc = -1;

    plain = text_buffer_join(tb, &plain_size);
    if (!plain) {
        return -1;
    }

    if (compress_data(algo, (const uint8_t *)plain, plain_size, &compressed, &compressed_size) != 0) {
        free(plain);
        return -1;
    }

    memset(&header, 0, sizeof(header));
    memcpy(header.magic, EDITOR_MAGIC, 8);
    header.version = EDITOR_VERSION;
    header.algorithm = (uint16_t)algo;
    header.io_mode = (uint16_t)io_mode;
    header.original_size = (uint64_t)plain_size;
    header.compressed_size = (uint64_t)compressed_size;

    full_size = sizeof(editor_file_header_t) + compressed_size;
    full = (uint8_t *)malloc(full_size);
    if (!full) {
        goto cleanup;
    }

    memcpy(full, &header, sizeof(editor_file_header_t));
    if (compressed_size > 0) {
        memcpy(full + sizeof(editor_file_header_t), compressed, compressed_size);
    }

    if (write_file_payload(path, full, full_size, io_mode) != 0) {
        goto cleanup;
    }

    rc = 0;

cleanup:
    free(full);
    free(compressed);
    free(plain);
    return rc;
}

int editor_open_file(text_buffer_t *tb, const char *path) {
    uint8_t *raw = NULL;
    size_t raw_size = 0;
    editor_file_header_t header;
    const uint8_t *payload;
    uint8_t *plain = NULL;
    size_t plain_size = 0;
    int rc = -1;

    if (read_file_payload(path, &raw, &raw_size) != 0) {
        return -1;
    }

    if (raw_size < sizeof(editor_file_header_t)) {
        goto cleanup;
    }

    memcpy(&header, raw, sizeof(editor_file_header_t));
    if (memcmp(header.magic, EDITOR_MAGIC, 8) != 0 || header.version != EDITOR_VERSION) {
        goto cleanup;
    }

    if (raw_size != sizeof(editor_file_header_t) + (size_t)header.compressed_size) {
        goto cleanup;
    }

    payload = raw + sizeof(editor_file_header_t);
    if (decompress_data(
            (compression_algo_t)header.algorithm,
            payload,
            (size_t)header.compressed_size,
            &plain,
            &plain_size,
            (size_t)header.original_size
        ) != 0) {
        goto cleanup;
    }

    if (text_buffer_load_plain(tb, (const char *)plain, plain_size) != 0) {
        goto cleanup;
    }

    rc = 0;

cleanup:
    free(plain);
    free(raw);
    return rc;
}

const char *compression_algo_name(compression_algo_t algo) {
    switch (algo) {
        case COMP_RLE:
            return "rle";
        case COMP_HUFFMAN:
            return "huffman";
        case COMP_LZW:
            return "lzw";
        case COMP_DEFLATE:
            return "deflate";
        default:
            return "unknown";
    }
}

const char *io_mode_name(io_mode_t mode) {
    switch (mode) {
        case IO_WRITE_BLOCKS:
            return "write";
        case IO_MMAP:
            return "mmap";
        default:
            return "unknown";
    }
}

int parse_algo(const char *name, compression_algo_t *out_algo) {
    if (!name || !out_algo) {
        return -1;
    }
    if (strcmp(name, "rle") == 0) {
        *out_algo = COMP_RLE;
        return 0;
    }
    if (strcmp(name, "huffman") == 0) {
        *out_algo = COMP_HUFFMAN;
        return 0;
    }
    if (strcmp(name, "lzw") == 0) {
        *out_algo = COMP_LZW;
        return 0;
    }
    if (strcmp(name, "deflate") == 0) {
        *out_algo = COMP_DEFLATE;
        return 0;
    }
    return -1;
}

int parse_io_mode(const char *name, io_mode_t *out_mode) {
    if (!name || !out_mode) {
        return -1;
    }
    if (strcmp(name, "write") == 0) {
        *out_mode = IO_WRITE_BLOCKS;
        return 0;
    }
    if (strcmp(name, "mmap") == 0) {
        *out_mode = IO_MMAP;
        return 0;
    }
    return -1;
}
