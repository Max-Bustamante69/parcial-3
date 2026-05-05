#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "editor.h"
#include "compression.h"
#include "io_backend.h"

static int read_plain_file(const char *path, uint8_t **data, size_t *size) {
    return read_file_payload(path, data, size);
}

static int write_plain_file(const char *path, const uint8_t *data, size_t size) {
    int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    size_t offset = 0;
    if (fd < 0) {
        return -1;
    }
    while (offset < size) {
        ssize_t n = write(fd, data + offset, size - offset);
        if (n <= 0) {
            close(fd);
            return -1;
        }
        offset += (size_t)n;
    }
    close(fd);
    return 0;
}

static int pack_command(
    const char *input_txt,
    const char *output_bin,
    const char *algo_str,
    const char *io_str
) {
    uint8_t *plain = NULL;
    size_t plain_size = 0;
    uint8_t *compressed = NULL;
    size_t compressed_size = 0;
    uint8_t *full = NULL;
    size_t full_size = 0;
    editor_file_header_t header;
    compression_algo_t algo;
    io_mode_t io_mode;
    int rc = -1;

    if (parse_algo(algo_str, &algo) != 0 || parse_io_mode(io_str, &io_mode) != 0) {
        fprintf(stderr, "Algoritmo o I/O invalido.\n");
        return -1;
    }

    if (read_plain_file(input_txt, &plain, &plain_size) != 0) {
        fprintf(stderr, "No se pudo leer input txt.\n");
        return -1;
    }

    if (compress_data(algo, plain, plain_size, &compressed, &compressed_size) != 0) {
        fprintf(stderr, "Fallo compresion.\n");
        goto cleanup;
    }

    memset(&header, 0, sizeof(header));
    memcpy(header.magic, EDITOR_MAGIC, 8);
    header.version = EDITOR_VERSION;
    header.algorithm = (uint16_t)algo;
    header.io_mode = (uint16_t)io_mode;
    header.original_size = (uint64_t)plain_size;
    header.compressed_size = (uint64_t)compressed_size;

    full_size = sizeof(header) + compressed_size;
    full = (uint8_t *)malloc(full_size);
    if (!full) {
        goto cleanup;
    }
    memcpy(full, &header, sizeof(header));
    if (compressed_size > 0) {
        memcpy(full + sizeof(header), compressed, compressed_size);
    }

    if (write_file_payload(output_bin, full, full_size, io_mode) != 0) {
        fprintf(stderr, "Fallo escritura.\n");
        goto cleanup;
    }

    printf("OK pack: %s -> %s | algo=%s io=%s plain=%llu compressed=%llu\n",
           input_txt,
           output_bin,
           algo_str,
           io_str,
           (unsigned long long)plain_size,
           (unsigned long long)compressed_size);
    rc = 0;

cleanup:
    free(full);
    free(compressed);
    free(plain);
    return rc;
}

static int unpack_command(const char *input_bin, const char *output_txt) {
    uint8_t *raw = NULL;
    size_t raw_size = 0;
    editor_file_header_t header;
    uint8_t *plain = NULL;
    size_t plain_size = 0;
    int rc = -1;

    if (read_file_payload(input_bin, &raw, &raw_size) != 0) {
        fprintf(stderr, "No se pudo leer binario.\n");
        return -1;
    }
    if (raw_size < sizeof(header)) {
        goto cleanup;
    }

    memcpy(&header, raw, sizeof(header));
    if (memcmp(header.magic, EDITOR_MAGIC, 8) != 0) {
        fprintf(stderr, "Magic invalido.\n");
        goto cleanup;
    }
    if (raw_size != sizeof(header) + (size_t)header.compressed_size) {
        fprintf(stderr, "Tamano corrupto.\n");
        goto cleanup;
    }

    if (decompress_data(
            (compression_algo_t)header.algorithm,
            raw + sizeof(header),
            (size_t)header.compressed_size,
            &plain,
            &plain_size,
            (size_t)header.original_size
        ) != 0) {
        fprintf(stderr, "Fallo descompresion.\n");
        goto cleanup;
    }

    if (write_plain_file(output_txt, plain, plain_size) != 0) {
        fprintf(stderr, "Fallo escritura txt.\n");
        goto cleanup;
    }

    printf("OK unpack: %s -> %s | recovered=%llu\n", input_bin, output_txt, (unsigned long long)plain_size);
    rc = 0;

cleanup:
    free(plain);
    free(raw);
    return rc;
}

static void print_usage(void) {
    printf("Uso:\n");
    printf("  pipeline_cli pack <input.txt> <output.bin> <rle|huffman|lzw|deflate> <write|mmap>\n");
    printf("  pipeline_cli unpack <input.bin> <output.txt>\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "pack") == 0) {
        if (argc != 6) {
            print_usage();
            return 1;
        }
        return pack_command(argv[2], argv[3], argv[4], argv[5]);
    }

    if (strcmp(argv[1], "unpack") == 0) {
        if (argc != 4) {
            print_usage();
            return 1;
        }
        return unpack_command(argv[2], argv[3]);
    }

    print_usage();
    return 1;
}
