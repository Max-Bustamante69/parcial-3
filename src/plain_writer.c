#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#define SMALL_BLOCK 64

int main(int argc, char **argv) {
    int fd_in;
    int fd_out;
    uint8_t buffer[SMALL_BLOCK];
    ssize_t n;

    if (argc != 3) {
        fprintf(stderr, "Uso: plain_writer <input.txt> <output.txt>\n");
        return 1;
    }

    fd_in = open(argv[1], O_RDONLY);
    if (fd_in < 0) {
        perror("open input");
        return 1;
    }

    fd_out = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd_out < 0) {
        perror("open output");
        close(fd_in);
        return 1;
    }

    while ((n = read(fd_in, buffer, sizeof(buffer))) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t written = write(fd_out, buffer + off, (size_t)(n - off));
            if (written <= 0) {
                perror("write");
                close(fd_in);
                close(fd_out);
                return 1;
            }
            off += written;
        }
    }

    close(fd_in);
    close(fd_out);
    return 0;
}
