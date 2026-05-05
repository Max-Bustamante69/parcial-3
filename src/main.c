#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editor.h"

#define INPUT_MAX 4096

static void print_help(void) {
    printf("Comandos disponibles:\n");
    printf("  help\n");
    printf("  append <texto>\n");
    printf("  insert <indice> <texto>\n");
    printf("  delete <indice>\n");
    printf("  print\n");
    printf("  clear\n");
    printf("  open <archivo.bin>\n");
    printf("  save <archivo.bin> <rle|huffman|lzw|deflate> <write|mmap>\n");
    printf("  stats\n");
    printf("  quit\n");
}

static void trim_newline(char *line) {
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
}

int main(void) {
    text_buffer_t tb;
    char input[INPUT_MAX];

    text_buffer_init(&tb);
    printf("Editor Parcial 3 - Seguridad y Compresion\n");
    printf("Autores: Maximiliano Bustamante y Valeria Hornung\n");
    print_help();

    while (1) {
        char *cmd;
        printf("\neditor> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        trim_newline(input);
        if (input[0] == '\0') {
            continue;
        }

        cmd = strtok(input, " ");
        if (!cmd) {
            continue;
        }

        if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "append") == 0) {
            char *text = strtok(NULL, "");
            if (!text) {
                printf("Uso: append <texto>\n");
                continue;
            }
            if (text_buffer_append(&tb, text) != 0) {
                printf("Error agregando linea.\n");
            }
        } else if (strcmp(cmd, "insert") == 0) {
            char *idx_str = strtok(NULL, " ");
            char *text = strtok(NULL, "");
            size_t idx;
            if (!idx_str || !text) {
                printf("Uso: insert <indice> <texto>\n");
                continue;
            }
            idx = (size_t)strtoull(idx_str, NULL, 10);
            if (text_buffer_insert(&tb, idx, text) != 0) {
                printf("Indice invalido o error de memoria.\n");
            }
        } else if (strcmp(cmd, "delete") == 0) {
            char *idx_str = strtok(NULL, " ");
            size_t idx;
            if (!idx_str) {
                printf("Uso: delete <indice>\n");
                continue;
            }
            idx = (size_t)strtoull(idx_str, NULL, 10);
            if (text_buffer_delete(&tb, idx) != 0) {
                printf("Indice invalido.\n");
            }
        } else if (strcmp(cmd, "print") == 0) {
            text_buffer_print(&tb);
        } else if (strcmp(cmd, "clear") == 0) {
            text_buffer_free(&tb);
            text_buffer_init(&tb);
            printf("Buffer limpio.\n");
        } else if (strcmp(cmd, "open") == 0) {
            char *path = strtok(NULL, " ");
            if (!path) {
                printf("Uso: open <archivo.bin>\n");
                continue;
            }
            if (editor_open_file(&tb, path) != 0) {
                printf("No se pudo abrir o descomprimir %s\n", path);
            } else {
                printf("Archivo cargado: %s\n", path);
            }
        } else if (strcmp(cmd, "save") == 0) {
            char *path = strtok(NULL, " ");
            char *algo_str = strtok(NULL, " ");
            char *io_str = strtok(NULL, " ");
            compression_algo_t algo;
            io_mode_t io_mode;

            if (!path || !algo_str || !io_str) {
                printf("Uso: save <archivo.bin> <rle|huffman|lzw|deflate> <write|mmap>\n");
                continue;
            }
            if (parse_algo(algo_str, &algo) != 0) {
                printf("Algoritmo no valido: %s\n", algo_str);
                continue;
            }
            if (parse_io_mode(io_str, &io_mode) != 0) {
                printf("Modo I/O no valido: %s\n", io_str);
                continue;
            }
            if (editor_save_file(&tb, path, algo, io_mode) != 0) {
                printf("Error guardando %s\n", path);
            } else {
                printf("Guardado OK con %s + %s\n", compression_algo_name(algo), io_mode_name(io_mode));
            }
        } else if (strcmp(cmd, "stats") == 0) {
            printf("Lineas en memoria: %llu\n", (unsigned long long)tb.count);
        } else if (strcmp(cmd, "quit") == 0) {
            break;
        } else {
            printf("Comando desconocido. Usa help.\n");
        }
    }

    text_buffer_free(&tb);
    return 0;
}
