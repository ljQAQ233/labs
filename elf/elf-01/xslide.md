---
theme: seriph
background: none
class: 'text-center'
transition: slide-left
title: elf-01 勘误
---

# slides.md 勘误

---

# 1. Elf32/Elf64 混淆

<br>

全篇是 x86_64, 却贴了 `Elf32_Phdr`.

Elf64 字段顺序不同: `p_flags` 紧跟 `p_type` (第二位), 字段宽度为 64 位.

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

---
transition: slide-left
---

# 2. ET_EXEC / ET_DYN 注释

- `ET_EXEC // static`: `gcc -no-pie` 的动态链接程序也是 `ET_EXEC` (带 `PT_INTERP`).

<br>

- `ET_EXEC` - 链接时地址已经确定
- `ET_DYN` - 链接时只确定相对基地址偏移

<br>

- `PT_INTERP` + `DT_NEED` - 有外部库
- `PT_INTERP` 缺失 - 不需要外部库, 直接链接进了 elf

---
transition: slide-left
---

# 3. align_dn 宏缺少括号

```c
#define align_dn(x, y) ((y) * (x / y))
```

`x` 未加括号, 传入表达式会按优先级错误展开.

```c
#define align_dn(x, y) ((y) * ((x) / (y)))
```
