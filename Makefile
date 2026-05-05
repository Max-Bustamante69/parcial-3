CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -pedantic -Iinclude
LDFLAGS ?=

USE_ZLIB ?= 1

COMMON_SRC = \
	src/compression.c \
	src/compression_rle.c \
	src/compression_huffman.c \
	src/compression_lzw.c \
	src/compression_deflate.c \
	src/io_backend.c \
	src/editor.c

EDITOR_SRC = src/main.c $(COMMON_SRC)
PIPELINE_SRC = src/pipeline_cli.c $(COMMON_SRC)
PLAIN_SRC = src/plain_writer.c

ifeq ($(USE_ZLIB),1)
	CFLAGS += -DUSE_ZLIB
	LDFLAGS += -lz
endif

.PHONY: all clean run benchmark report

all: editor pipeline_cli plain_writer

editor: $(EDITOR_SRC)
	$(CC) $(CFLAGS) -o $@ $(EDITOR_SRC) $(LDFLAGS)

pipeline_cli: $(PIPELINE_SRC)
	$(CC) $(CFLAGS) -o $@ $(PIPELINE_SRC) $(LDFLAGS)

plain_writer: $(PLAIN_SRC)
	$(CC) $(CFLAGS) -o $@ $(PLAIN_SRC)

run: editor
	./editor

benchmark: all
	bash scripts/benchmark_fast.sh

report:
	python3 scripts/generate_report.py

clean:
	rm -f editor pipeline_cli plain_writer
	rm -rf out/*
