#ifndef IO_BACKEND_H
#define IO_BACKEND_H

#include <stddef.h>
#include <stdint.h>
#include "file_format.h"

int write_file_payload(
    const char *path,
    const uint8_t *data,
    size_t size,
    io_mode_t mode
);

int read_file_payload(const char *path, uint8_t **data, size_t *size);

#endif
