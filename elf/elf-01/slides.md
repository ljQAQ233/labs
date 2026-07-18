---
theme: seriph
background: none
class: 'text-center'
transition: slide-left
title: elf-01 静态 ELF 文件加载

fonts:
  sans: 'Fira Code,Noto Sans CJK SC'
  serif: 'Fira Code,Noto Sans CJK SC'
  mono: 'Fira Code,Noto Sans CJK SC'
---

# elf-01 静态 ELF 文件加载

---
layout: center
---

- <https://github.com/ljqaq233/labs>

项目名称叫作 `labs`, 就有自己探索的意味, 这一期是 在 linux 上尝试加载一个 <span v-mark.red> **静态链接** </span> 的 elf 文件

当然, 常见的 elf 加载发生在 <span v-mark.blue>内核态</span>, <span v-mark.blue>包括 OS Loader / Boot Loader</span>, 但是这不妨碍 做一个小玩具吧 (<strike>可能玩具都不是</strike>)

---
transition: fade-out
layout: two-cols
layoutClass: gap-2
---

# ELF 文件组成

查看[文档](https://refspecs.linuxfoundation.org/elf/elf.pdf)(文档有亿点点旧, 主要以头文件为主), 也可以直接看 [头文件](https://github.com/torvalds/linux/blob/master/include/uapi/linux/elf.h), 或者手册 `man elf`

<div v-click="1">

elf 中最重要的两个结构是 <span v-mark.red="3">**段 (segment)**</span> 与 <span v-mark.red="4">**节 (section)** </span>

- 并且, segment 由 section 组成

>  An object file segment contains one or more sections (p40)

</div>

<div v-click="3">

- 加载时 只需要看 segment (our loader), 告知映射信息

</div>

<div v-click="4">

- 而 section 是给链接器看的
  - 而且<span v-mark.green="5">主要是 工具链</span>, 部分会通过 **dynamic 段** 暴露给 <span v-mark.green="6"> **动态链接器**</span>

</div>

::right::

<div v-click="6">

```c
void *dmap = dl->dynamic;
Elf64_Dyn *dyn = (Elf64_Dyn *)dmap;
memset(dl->array, 0, sizeof(dl->array));
for ( ; dyn->d_tag != DT_NULL ; dyn++)
{
    if (dyn->d_tag < DT_NUM)
        dl->array[dyn->d_tag] = dyn->d_un.d_val;
    if (dyn->d_tag == DT_PLTREL)
        dllog("pltrel %d used\n", dyn->d_un.d_val);
    if (dyn->d_tag == DT_DEBUG)
        dyn->d_un.d_ptr = (uintptr_t)&dbg;
    if (dyn->d_tag == DT_GNU_HASH)
        dl->ghash = dl->virt + dyn->d_un.d_ptr;
}
```

</div>


---
transition: fade-out
layout: two-cols
---

## ELF Header

众多的数据结构, 在文件的开头都具有 **magic number**

往往, 他们位于一个 **header** 以内 <v-click> 比如 BMP 格式 </v-click>

<div v-click="1">

<!-- 这些东西是文件的元数据, 标示这个是什么文件, 大小, 以及具体的结构 -->

```c {all|2-3|4|6-17}
typedef struct {
  CHAR8         CharB;
  CHAR8         CharM;
  UINT32        Size;
  UINT16        Reserved[2];
  UINT32        ImageOffset;
  UINT32        HeaderSize;
  UINT32        PixelWidth;
  UINT32        PixelHeight;
  UINT16        Planes;          ///< Must be 1
  UINT16        BitPerPixel;     ///< 1, 4, 8, or 24
  UINT32        CompressionType;
  UINT32        ImageSize;       ///< Compressed image size in bytes
  UINT32        XPixelsPerMeter;
  UINT32        YPixelsPerMeter;
  UINT32        NumberOfColors;
  UINT32        ImportantColors;
} BMP_IMAGE_HEADER;
```

</div>

---
transition: fade-out
layout: two-cols
layoutClass: gap-8
---

<!--
  这是一份从 manual 中摘抄的 ELF Header 的定义
  e_ident 包含了 magic number, 以及其他一些东西比如 ABI 版本
-->

```c {all|2|2|3-4|all}{at:1}
typedef struct {
  unsigned char e_ident[EI_NIDENT];
  uint16_t      e_type;
  uint16_t      e_machine;
  uint32_t      e_version;
  ElfN_Addr     e_entry;
  ElfN_Off      e_phoff;
  ElfN_Off      e_shoff;
  uint32_t      e_flags;
  uint16_t      e_ehsize;
  uint16_t      e_phentsize;
  uint16_t      e_phnum;
  uint16_t      e_shentsize;
  uint16_t      e_shnum;
  uint16_t      e_shstrndx;
} ElfN_Ehdr;
```

::right::

<v-switch>

<template #1>

```c
#define EI_MAG0 0
#define ELFMAG0 0x7f

#define EI_MAG1 1
#define ELFMAG1 'E'

#define EI_MAG2 2
#define ELFMAG2 'L'

#define EI_MAG3 3
#define ELFMAG3 'F'

#define ELFMAG "\177ELF"
#define SELFMAG 4
```

</template>

<template #2>

<div style="font-size:12px">

| Index | Name | Value | Purpose |
|-------|------|-------|---------|
| 0 | EI_MAG0 | 0 | File identification |
| 1 | EI_MAG1 | 1 | File identification |
| 2 | EI_MAG2 | 2 | File identification |
| 3 | EI_MAG3 | 3 | File identification |
| 4 | EI_CLASS | 4 | File class |
| 5 | EI_DATA | 5 | Data encoding |
| 6 | EI_VERSION | 6 | File version |
| 7 | EI_PAD | 7 | Start of padding bytes |
| — | EI_NIDENT | 16 | Size of e_ident[] |
</div>

</template>

<template #3>

```c
#define ET_EXEC 2 // static / -no-pie
#define ET_DYN 3  // static-pie / pie / .so

#define EM_NONE 0
#define EM_M32 1
#define EM_SPARC 2
#define EM_386 3
#define EM_68K 4
#define EM_88K 5
#define EM_860 7
#define EM_MIPS 8
```

- `ET_DYN` 如果没有指定 `interpreter` 就是 **static-pie** (可执行文件)
- 动态链接器 (ldso) 就是 **static-pie 居多**

</template>

</v-switch>

---
layout: two-cols
layoutClass: gap-1
zoom: 0.8
---

````md magic-move
```c
typedef struct {
  unsigned char e_ident[EI_NIDENT];
  uint16_t      e_type;
  uint16_t      e_machine;
  uint32_t      e_version;
  ElfN_Addr     e_entry;   // elf entrypoint
  ElfN_Off      e_phoff;   // program header (segment header) offset
  ElfN_Off      e_shoff;   // section header offset (in the file)
  uint32_t      e_flags;
  uint16_t      e_ehsize;    // size of elf header
  uint16_t      e_phentsize; // size of each program header entry
  uint16_t      e_phnum;     // number of program header
  uint16_t      e_shentsize; // section ...
  uint16_t      e_shnum;
  uint16_t      e_shstrndx;
} ElfN_Ehdr;
```
````

<v-click>

```shell
readelf -h /bin/sh
```

</v-click>

::right::

<v-click>

```
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN (Position-Independent Executable file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x28fb0
  Start of program headers:          64 (bytes into file)
  Start of section headers:          1215688 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         14
  Size of section headers:           64 (bytes)
  Number of section headers:         32
  Section header string table index: 31
```

</v-click>

<v-click>

```
Entry point address:               0x28fb0
```

由于这个是 `ET_DYN`, 所以这个地址是相对于 一个基地址的偏移位置

</v-click>

---
layout: default
zoom: 0.9
---

**Program Header**, 也就是 **Segment Header**, 为 加载器 提供了映射信息

```shell
readelf --segments kernel.elf
```

<v-click>

<<< ../snippets/kernel-elf-segs.txt {all|6-17}

</v-click>

---
layout: default
---
  
```txt
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz             Flags  Align
```

```c
typedef struct {
  uint32_t   p_type;
  uint32_t   p_flags;
  Elf64_Off  p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  uint64_t   p_filesz;
  uint64_t   p_memsz;
  uint64_t   p_align;
} Elf64_Phdr;
```



NOTE:

> Values 0 and 1 mean that no alignment is required. Otherwise,
> p_align should be a positive, integral power of 2 (p40)

<v-click>

除此之外, 就是 `p_paddr` 是物理地址, 在 现代操作系统 上一般忽略

</v-click>

---
layout: default
zoom: 0.9
---

如图所示 kernel 是静态编译, 它有一堆在编译时就已经确定的地址, 类型为 `ET_EXEC`

OS Loader 需要做的工作就是建立页表, 完成 文件地址 -> 虚拟地址 的映射. 用户态 : `mmap`

<<< ../snippets/kernel-elf-segs.txt

---
layout: default
layoutClass: gap-2
---

## However

有一些程序, 比如后面要用到的测试程序 `asm.out`

````md magic-move
```txt {7-8}
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000001000 0x0000000000401000 0x0000000000401000
                 0x000000000000002a 0x000000000000002a  R E    0x1000
  LOAD           0x0000000000002000 0x0000000000402000 0x0000000000402000
                 0x0000000000000030 0x0000000000000030  R      0x1000
  LOAD           0x0000000000002030 0x0000000000403030 0x0000000000403030
                 0x000000000000000d 0x000000000000000d  RW     0x1000
```
```txt {3-4}
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000002030 0x0000000000403030 0x0000000000403030
                 0x000000000000000d 0x000000000000000d  RW     0x1000
```
````

<v-click>

对于 `mmap`, 要求页对齐的情况下, 这个 `LOAD` 段显然是不尽人意的.

这时候需要 **地址向下对齐**, 使用 **p_align** 作为 alignment

</v-click>

---
transition: slide-left
layout: default
layoutClass: gap-2
---

```txt {3-4}
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000002030 0x0000000000403030 0x0000000000403030
                 0x000000000000000d 0x000000000000000d  RW     0x1000
```

```txt

0x00403000 ┌──────────────────┐
           │                  │
           │    (48 bytes)    │
           │                  │
0x00403030 ├──────────────────┤
           │                  │
           │    LOAD(RW)      │
           │    13 bytes      │ <- p_filesz = p_memsz = 0xd
           │                  │
0x0040303D ├──────────────────┤
           │                  │
           │     unused       │ <- 4035 bytes till next page
           │                  │
0x00404000 └──────────────────┘

```

---
transition: slide-left
layout: two-cols
layoutClass: gap-2
---

现在讨论, 如何加载一个 `ET_EXEC`, 也就是 `statically-linked` 的 elf 可执行程序 (`x86_64`)

<v-clicks>

1. 加载文件, 检查类型 (`mmap`)
2. 读取 `program header`
3. 遍历 `program header`, 映射 (`mmap`)
4. 跳转 (`x86_64 jmp instr`)
5. 测试

</v-clicks>

<v-click>

```makefile {class:'!children:text-xs'}
asm.out: ../asm.S
	$(CC) $< -o $@ -static -nostdlib -ffreestanding
```

</v-click>

<div v-click="12">

```shell
$ objdump -s --start-address=0x403030 \
  --stop-address=0x40303d asm.out 

asm.out:     file format elf64-x86-64

Contents of section .data:
 403030 48656c6c 6f20776f 726c6421 0a  Hello world!.
```

</div>

::right::

<v-click>

`elf/asm.S`
```asm {all|2-3|4-8|10-12}
  .text
  .globl _start
_start:
  movq $1, %rax
  movq $1, %rdi
  movq $msg, %rsi
  movq $len, %rdx
  syscall /* write(1, msg, ) */

  movq $60, %rax
  xorq %rdi, %rdi
  syscall /* exit(0) */

  .data
msg:
  .ascii "Hello world!\n"
len = . - msg
```

</v-click>

<v-click>

```txt
0x00403030 ┌──────────────────┐
           │    LOAD(RW)      │
           │    13 bytes      │
0x0040303D └──────────────────┘
```

</v-click>

---
transition: fade-out
layout: center
---

# Coding

---
transition: fade-out
layout: default
---

````md magic-move
```c
#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  return 0;
}
```

```c
#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define cast(t, p) ((t)(p))
#define align_up(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define align_dn(x, y) ((y) * (x / y))

int main(int argc, char *argv[]) {
  return 0;
}
```

```c {*|14-16|18|19-20}
#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define cast(t, p) ((t)(p))
#define align_up(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define align_dn(x, y) ((y) * (x / y))

int main(int argc, char *argv[]) {
  assert(argc == 2);
  int fd = open(argv[1], O_RDONLY);
  assert(fd >= 0);

  size_t len = lseek(fd, 0, SEEK_END);
  void *base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(base != MAP_FAILED);

  return 0;
}
```

```c
int main(int argc, char *argv[]) {
  assert(argc == 2);
  int fd = open(argv[1], O_RDONLY);
  assert(fd >= 0);

  size_t len = lseek(fd, 0, SEEK_END);
  void *base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(base != MAP_FAILED);

  Elf64_Ehdr *ehdr = cast(Elf64_Ehdr *, base);
  assert(ehdr->e_ident[0] == ELFMAG0);
  assert(ehdr->e_ident[1] == ELFMAG1);
  assert(ehdr->e_ident[2] == ELFMAG2);
  assert(ehdr->e_ident[3] == ELFMAG3);
  assert(ehdr->e_type == ET_EXEC);
  assert(ehdr->e_machine == EM_X86_64);

  return 0;
}
```

```c
int main(int argc, char *argv[]) {
  /* ... */
  size_t len = lseek(fd, 0, SEEK_END);
  void *base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(base != MAP_FAILED);

  Elf64_Ehdr *ehdr = cast(Elf64_Ehdr *, base);
  assert(ehdr->e_ident[0] == ELFMAG0);
  assert(ehdr->e_ident[1] == ELFMAG1);
  assert(ehdr->e_ident[2] == ELFMAG2);
  assert(ehdr->e_ident[3] == ELFMAG3);
  assert(ehdr->e_type == ET_EXEC);
  assert(ehdr->e_machine == EM_X86_64);

  void *phdrs = cast(Elf64_Phdr *, base + ehdr->e_phoff);

  return 0;
}
```
```c
int main(int argc, char *argv[]) {
  /* ... */
  size_t len = lseek(fd, 0, SEEK_END);
  void *base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(base != MAP_FAILED);

  Elf64_Ehdr *ehdr = cast(Elf64_Ehdr *, base);
  assert(ehdr->e_ident[0] == ELFMAG0);
  assert(ehdr->e_ident[1] == ELFMAG1);
  assert(ehdr->e_ident[2] == ELFMAG2);
  assert(ehdr->e_ident[3] == ELFMAG3);
  assert(ehdr->e_type == ET_EXEC);
  assert(ehdr->e_machine == EM_X86_64);

  void *phdrs = cast(Elf64_Phdr *, base + ehdr->e_phoff);
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *phdr = cast(Elf64_Phdr *, phdrs + i * ehdr->e_phentsize);
  }

  return 0;
}
```
```c
#define cast(t, p) ((t)(p))
#define align_up(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define align_dn(x, y) ((y) * (x / y))

int main(int argc, char *argv[]) {
  /* ... */
  void *phdrs = cast(Elf64_Phdr *, base + ehdr->e_phoff);
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *phdr = cast(Elf64_Phdr *, phdrs + i * ehdr->e_phentsize);
  }

  return 0;
}
```
```c {*|12|13-18}
#define cast(t, p) ((t)(p))
#define align_up(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define align_dn(x, y) ((y) * (x / y))

int main(int argc, char *argv[]) {
  /* ... */
  void *phdrs = cast(Elf64_Phdr *, base + ehdr->e_phoff);
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *phdr = cast(Elf64_Phdr *, phdrs + i * ehdr->e_phentsize);
    if (phdr->p_type != PT_LOAD)
      continue;
    size_t A = phdr->p_align;
    size_t foff = align_dn(phdr->p_offset, A);
    size_t fend = align_up(phdr->p_offset + phdr->p_filesz, A);
    size_t fsize = fend - foff;
    size_t moff = align_dn(phdr->p_vaddr, A);
    size_t mend = align_up(phdr->p_vaddr + phdr->p_memsz, A);
    size_t msize = mend - moff;
  }

  return 0;
}
```
```c
int main(int argc, char *argv[]) {
  /* ... */
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *phdr = cast(Elf64_Phdr *, phdrs + i * ehdr->e_phentsize);
    if (phdr->p_type != PT_LOAD)
      continue;
    size_t A = phdr->p_align;
    size_t foff = align_dn(phdr->p_offset, A);
    size_t fend = align_up(phdr->p_offset + phdr->p_filesz, A);
    size_t fsize = fend - foff;
    size_t moff = align_dn(phdr->p_vaddr, A);
    size_t mend = align_up(phdr->p_vaddr + phdr->p_memsz, A);
    size_t msize = mend - moff;

    void *exp = cast(void *, moff);
    printf("  map %#zx -> %p, sz = %#zx, msz = %#zx\n", foff, exp, fsize,
           msize);
    void *area = mmap(exp, fsize, prot, MAP_PRIVATE | MAP_FIXED, fd, foff);
    assert(area == exp);
  }

  return 0;
}
```
```c
int main(int argc, char *argv[]) {
  /* ... */
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *phdr = cast(Elf64_Phdr *, phdrs + i * ehdr->e_phentsize);
    if (phdr->p_type != PT_LOAD)
      continue;
    int prot = 0;
    if (phdr->p_flags & PF_R)
      prot |= PROT_READ;
    if (phdr->p_flags & PF_W)
      prot |= PROT_WRITE;
    if (phdr->p_flags & PF_X)
      prot |= PROT_EXEC;
    size_t A = phdr->p_align;
    size_t foff = align_dn(phdr->p_offset, A);
    size_t fend = align_up(phdr->p_offset + phdr->p_filesz, A);
    size_t fsize = fend - foff;
    size_t moff = align_dn(phdr->p_vaddr, A);
    size_t mend = align_up(phdr->p_vaddr + phdr->p_memsz, A);
    size_t msize = mend - moff;

    void *exp = cast(void *, moff);
    printf("  map %#zx -> %p, sz = %#zx, msz = %#zx\n", foff, exp, fsize,
           msize);
    void *area = mmap(exp, fsize, prot, MAP_PRIVATE | MAP_FIXED, fd, foff);
    assert(area == exp);
  }

  return 0;
}
```
````

---
transition: fade-out
layout: default
---

4. 跳转 (`x86_64 jmp instr`)

````md magic-move
```c
int main(int argc, char *argv[]) {
  /* ... */
  Elf64_Ehdr *ehdr = cast(Elf64_Ehdr *, base);
  /* ... */
  return 0;
}
```
```c
int main(int argc, char *argv[]) {
  /* ... */
  Elf64_Ehdr *ehdr = cast(Elf64_Ehdr *, base);
  /* ... */
  void *start = cast(void *, ehdr->e_entry);
  printf("start is at %p\n", start);
  return 0;
}
```
```c
int main(int argc, char *argv[]) {
  /* ... */
  Elf64_Ehdr *ehdr = cast(Elf64_Ehdr *, base);
  /* ... */
  void *start = cast(void *, ehdr->e_entry);
  printf("start is at %p\n", start);
  asm volatile(
      "jmp *%0\n"
      ::"r"(start)
      : "memory"
  );
  return 0;
}
```
```c
int main(int argc, char *argv[]) {
  /* ... */
  Elf64_Ehdr *ehdr = cast(Elf64_Ehdr *, base);
  /* ... */
  void *start = cast(void *, ehdr->e_entry);
  printf("start is at %p\n", start);
  asm volatile(
      "jmp *%0\n"
      ::"r"(start)
      : "memory"
  );
  __builtin_unreachable();
}
```
````

---
transition: slide-up
layout: center
---

# Run

---
layout: default
---

```shell
cd labs/elf/elf-01
make -f ../makefile clean all
./elf.out ./asm.out
```

<v-click>

```txt
  map 0 -> 0x400000, sz = 0x1000, msz = 0x1000
  map 0x1000 -> 0x401000, sz = 0x1000, msz = 0x1000
  map 0x2000 -> 0x402000, sz = 0x1000, msz = 0x1000
  map 0x2000 -> 0x403000, sz = 0x1000, msz = 0x1000
start is at 0x401000
Hello world!
```

</v-click>

---
transition: slide-up
layout: center
---

# めでたし めでたし

---
transition: slide-up
layout: center
---

# P.S.

---
layout: default
---

在遍历 Program Header 时:

```c
void *phdrs = cast(Elf64_Phdr *, base + ehdr->e_phoff);
for (int i = 0; i < ehdr->e_phnum; i++) {
  Elf64_Phdr *phdr = cast(Elf64_Phdr *, phdrs + i * ehdr->e_phentsize);
}
```

而不是

```c
Elf64_Phdr *phdrs = cast(Elf64_Phdr *, base + ehdr->e_phoff);
for (int i = 0; i < ehdr->e_phnum; i++) {
  Elf64_Phdr *phdr = phdrs + i;
}
```

既然 `Ehdr` 提供了 一个 `segment header size`, 不能假设: `hdr 间地址差值 = sizeof(Elf64_Phdr)`

---
transition: slide-up
layout: default
---

此事在 `UEFI` 中亦有记载:

```c
EFI_STATUS
EFIAPI
CoreGetMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  OUT UINTN                     *MapKey,
  OUT UINTN                     *DescriptorSize,
  OUT UINT32                    *DescriptorVersion
  )
```

而 OVMF 刚刚好就是不按套路出牌:

`MdeModulePkg/Core/Dxe/Mem/Page.c`

```c
  Size = sizeof (EFI_MEMORY_DESCRIPTOR);

  //
  // Make sure Size != sizeof(EFI_MEMORY_DESCRIPTOR). This will
  // prevent people from having pointer math bugs in their code.
  // now you have to use *DescriptorSize to make things work.
  //
  Size += sizeof(UINT64) - (Size % sizeof (UINT64));
```

---
layout: center
---

# TODO

---
transition: fade-out
layout: default
---

1. `.bss` 没有清空, 但是由于 `asm.out` 没有 `.bss`, 所以下次一定

   - 类似这样的程序的 buf 属于需要被初始化为 0 的数据, 存放在 `.bss` 节, 这时候会得到 <span v-mark.circle.red>一群乱码</span>
   - 当然: bss 可能没有映射, 这时候会 SIGSEGV

```c
char buf[233];
int main() {
  static char buf1[123];
  return 0;
}
```

<v-click>

2. `overlap`

    - 在 `elf-02` 解决

</v-click>

---
transition: fade-out
layout: center
---

# Thanks for watching

- <https://github.com/ljqaq233/labs>
- slide show powered by `slidev`
