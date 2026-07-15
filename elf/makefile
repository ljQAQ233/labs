CC = gcc

all: elf.out static.out

elf.out: elf.c
	$(CC) $< -o $@

static.out: ../static.c
	$(CC) $< -o $@ -static

.PHONY:all
