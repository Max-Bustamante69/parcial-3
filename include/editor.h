#ifndef EDITOR_H
#define EDITOR_H

#include <stddef.h>
#include "file_format.h"

typedef struct {
    char **lines;
    size_t count;
    size_t capacity;
} text_buffer_t;

void text_buffer_init(text_buffer_t *tb);
void text_buffer_free(text_buffer_t *tb);
int text_buffer_append(text_buffer_t *tb, const char *line);
int text_buffer_insert(text_buffer_t *tb, size_t index, const char *line);
int text_buffer_delete(text_buffer_t *tb, size_t index);
void text_buffer_print(const text_buffer_t *tb);
char *text_buffer_join(const text_buffer_t *tb, size_t *out_size);
int text_buffer_load_plain(text_buffer_t *tb, const char *plain, size_t size);

int editor_save_file(
    const text_buffer_t *tb,
    const char *path,
    compression_algo_t algo,
    io_mode_t io_mode
);

int editor_open_file(text_buffer_t *tb, const char *path);

const char *compression_algo_name(compression_algo_t algo);
const char *io_mode_name(io_mode_t mode);
int parse_algo(const char *name, compression_algo_t *out_algo);
int parse_io_mode(const char *name, io_mode_t *out_mode);

#endif
