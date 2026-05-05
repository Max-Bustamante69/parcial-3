#ifndef FILE_FORMAT_H
#define FILE_FORMAT_H

#include <stdint.h>

#define EDITOR_MAGIC "P3EDIT0"
#define EDITOR_VERSION 1

typedef enum {
    COMP_RLE = 1,
    COMP_HUFFMAN = 2,
    COMP_LZW = 3,
    COMP_DEFLATE = 4
} compression_algo_t;

typedef enum {
    IO_WRITE_BLOCKS = 1,
    IO_MMAP = 2
} io_mode_t;

typedef struct __attribute__((packed)) {
    char magic[8];
    uint16_t version;
    uint16_t algorithm;
    uint16_t io_mode;
    uint16_t reserved;
    uint64_t original_size;
    uint64_t compressed_size;
} editor_file_header_t;

#endif
