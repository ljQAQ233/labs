CC = gcc

ifeq ($(notdir $(CURDIR)),elf-03)
ENTRY_FLAG = -Wl,-e,mystart
else
ENTRY_FLAG =
endif

all: elf.out static.out asm.out

elf.out: elf.c
	$(CC) $< -o $@ -static -Wl,-Ttext-segment=0x10000000 $(ENTRY_FLAG)

static.out: ../static.c
	$(CC) $< -o $@ -static

asm.out: ../asm.S
	$(CC) $< -o $@ -static -nostdlib -ffreestanding

asm-pie.out: ../asm.S
	$(CC) $< -o $@ -static-pie -nostdlib -ffreestanding

.PHONY:all
