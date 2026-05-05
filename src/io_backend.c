#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "io_backend.h"

#define IO_BLOCK_SIZE 4096

int write_file_payload(
    const char *path,
    const uint8_t *data,
    size_t size,
    io_mode_t mode
) {
    int fd;
    ssize_t written;
    size_t offset = 0;

    fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd < 0) {
        return -1;
    }

    if (mode == IO_MMAP) {
        void *map;
        if (ftruncate(fd, (off_t)size) != 0) {
            close(fd);
            return -1;
        }
        map = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            return -1;
        }
        memcpy(map, data, size);
        if (msync(map, size, MS_SYNC) != 0) {
            munmap(map, size);
            close(fd);
            return -1;
        }
        munmap(map, size);
    } else {
        while (offset < size) {
            size_t chunk = (size - offset > IO_BLOCK_SIZE) ? IO_BLOCK_SIZE : (size - offset);
            written = write(fd, data + offset, chunk);
            if (written <= 0) {
                close(fd);
                return -1;
            }
            offset += (size_t)written;
        }
    }

    close(fd);
    return 0;
}

int read_file_payload(const char *path, uint8_t **data, size_t *size) {
    int fd;
    struct stat st;
    uint8_t *buffer;
    size_t offset = 0;
    ssize_t got;

    if (!data || !size) {
        return -1;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    if (fstat(fd, &st) != 0 || st.st_size < 0) {
        close(fd);
        return -1;
    }

    *size = (size_t)st.st_size;
    if (*size == 0) {
        *data = NULL;
        close(fd);
        return 0;
    }

    buffer = (uint8_t *)malloc(*size);
    if (!buffer) {
        close(fd);
        return -1;
    }

    while (offset < *size) {
        got = read(fd, buffer + offset, *size - offset);
        if (got <= 0) {
            free(buffer);
            close(fd);
            return -1;
        }
        offset += (size_t)got;
    }

    close(fd);
    *data = buffer;
    return 0;
}
