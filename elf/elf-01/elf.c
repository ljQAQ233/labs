#include <assert.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define cast(t, p) ((t)(p))
#define align_up(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define align_dn(x, y) ((y) * (x / y))

int main(int argc, char *argv[]) {
  int fd = open("static.out", O_RDONLY);
  assert(fd > 0);

  size_t len = lseek(fd, 0, SEEK_END);
  void *base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(base != 0);

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
    if (phdr->p_type != PT_LOAD)
      continue;
    int prot = 0;
    if (phdr->p_flags & PF_R)
      prot |= PROT_READ;
    if (phdr->p_flags & PF_W)
      prot |= PROT_WRITE;
    if (phdr->p_flags & PF_X)
      prot |= PROT_EXEC;
    size_t sz = align_up(phdr->p_memsz, sz);
    size_t bss_sz = sz - phdr->p_filesz;
    void *exp = cast(void *, phdr->p_vaddr);
    printf("  map %#zx -> %p, sz = %#zx, bss_sz = %#zx, align = %#zx\n",
           phdr->p_offset, exp, sz, bss_sz, phdr->p_align);
  }
  void *start = cast(void *, ehdr->e_entry);

  return 0;
}
