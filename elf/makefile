CC = gcc

all: elf.out static.out asm.out

elf.out: elf.c
	$(CC) $< -o $@ -static -Wl,-Ttext-segment=0x10000000

static.out: ../static.c
	$(CC) $< -o $@ -static

asm.out: ../asm.S
	$(CC) $< -o $@ -static -nostdlib -ffreestanding

.PHONY:all
