#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

extern char **environ;

#define cast(t, p) ((t)(p))
#define align_up(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define align_dn(x, y) ((y) * (x / y))

#define N sizeof(long)

void *build_frame(char *argv[], char *envp[], long *auxv) {
  static char frame[1 << 14];
  void *sp = cast(void *, frame + sizeof(frame));
  int argc = 0, envc = 0;

  while (argv && argv[argc])
    argc++;
  while (envp && envp[envc])
    envc++;

  unsigned long argv_addr[argc], envp_addr[envc];
  for (int i = envc - 1; i >= 0; i--) {
    size_t n = strlen(envp[i]) + 1;
    sp -= n;
    memcpy(sp, envp[i], n);
    envp_addr[i] = cast(unsigned long, sp);
  }
  for (int i = argc - 1; i >= 0; i--) {
    size_t n = strlen(argv[i]) + 1;
    sp -= n;
    memcpy(sp, argv[i], n);
    argv_addr[i] = cast(unsigned long, sp);
  }
  sp = cast(void *, cast(unsigned long, sp) & ~15UL);
  if (!((envc + argc) & 1))
    sp -= N;

  int naux = 0;
  if (auxv) {
    while (auxv[naux] != AT_NULL)
      naux += 2;
    naux += 2;
  }
  sp -= naux * N;
  memcpy(sp, auxv, (naux - 2) * N);
  cast(long *, sp)[naux - 2] = AT_NULL;
  cast(long *, sp)[naux - 1] = 0;

  sp -= N;
  *cast(void **, sp) = NULL;
  sp -= envc * N;
  for (int i = 0; i < envc; i++)
    cast(void **, sp)[i] = (void *)envp_addr[i];

  sp -= N;
  *cast(void **, sp) = NULL;
  sp -= argc * N;
  for (int i = 0; i < argc; i++)
    cast(void **, sp)[i] = (void *)argv_addr[i];

  sp -= N;
  *cast(long *, sp) = argc;

  return sp;
}

int main(int argc, char *argv[]) {
  assert(argc == 2);
  int fd = open(argv[1], O_RDONLY);
  assert(fd > 0);

  size_t len = lseek(fd, 0, SEEK_END);
  void *base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(base != 0);

  Elf64_Ehdr *ehdr = cast(Elf64_Ehdr *, base);
  assert(ehdr->e_ident[0] == ELFMAG0);
  assert(ehdr->e_ident[1] == ELFMAG1);
  assert(ehdr->e_ident[2] == ELFMAG2);
  assert(ehdr->e_ident[3] == ELFMAG3);
  assert(ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN);
  assert(ehdr->e_machine == EM_X86_64);

  intptr_t lp_max = 0;
  intptr_t lp_min = -1;
  void *phdrs = cast(Elf64_Phdr *, base + ehdr->e_phoff);
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *phdr = cast(Elf64_Phdr *, phdrs + i * ehdr->e_phentsize);
    if (phdr->p_type == PT_LOAD) {
      uintptr_t dptr = align_dn(phdr->p_vaddr, phdr->p_align);
      uintptr_t uptr = align_up(phdr->p_vaddr + phdr->p_memsz, phdr->p_align);
      if (dptr < lp_min)
        lp_min = dptr;
      if (uptr > lp_max)
        lp_max = uptr;
    }
    if (phdr->p_type == PT_INTERP)
      assert(!"ldso not supported!!!");
  }
  void *virt = 0;
  uintptr_t lp_size = lp_max - lp_min;
  if (ehdr->e_type == ET_DYN) {
    virt = mmap(NULL, lp_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
    assert(virt != MAP_FAILED);
  }

  int is_overlapped = 0;
  size_t prev_mend = 0;
  size_t prev_prot = 0;
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
    if (prev_mend > moff) {
      // handle overlap
      size_t overlapped = prev_mend - moff;
      msize -= overlapped, moff += overlapped;
      fsize -= overlapped, foff += overlapped;
      ssize_t readsz = foff - phdr->p_offset;
      ssize_t readoff = moff - phdr->p_vaddr;
      if (readsz > phdr->p_filesz)
        readsz = phdr->p_filesz;
      assert(readsz > 0 && readoff > 0); // I'm sure

      if (!(prev_prot & PROT_WRITE))
        mprotect(cast(void *, prev_mend), overlapped, prev_prot | PROT_WRITE);
      lseek(fd, phdr->p_offset, SEEK_SET);
      read(fd, virt + phdr->p_vaddr, readsz);
      if (!(prev_prot & PROT_WRITE) && !(phdr->p_flags & PF_W))
        mprotect(cast(void *, prev_mend), overlapped, prev_prot);
      is_overlapped = 1;
    } else {
      is_overlapped = 0;
    }
    prev_mend = mend;
    prev_prot = prot;

    void *exp = virt + moff;
    printf("  map %#zx -> %p, sz = %#zx, msz = %#zx\n", foff, exp, fsize,
           msize);
    if (is_overlapped)
      printf("  map handled overlapped\n");
    void *area = mmap(exp, fsize, prot, MAP_PRIVATE | MAP_FIXED, fd, foff);
    assert(area == exp);

    if (prot & PROT_WRITE) {
      if (msize > fsize) {
        // create remained mapping for zero-padded pages
        size_t exsize = msize - fsize;
        void *expages = mmap(exp + fsize, exsize, prot,
                             MAP_PRIVATE | MAP_FIXED | MAP_ANON, -1, 0);
        assert(expages != MAP_FAILED);
      }
      // clear bss
      void *bss = virt + phdr->p_vaddr + phdr->p_filesz;
      size_t bss_size = virt + mend - bss;
      printf("  clearing bss %p - %p\n", bss, bss + bss_size);
      memset(bss, 0, bss_size);
    }
  }
  void *start = virt + ehdr->e_entry;
  void *sp = build_frame(argv + 1, environ, (long[]){AT_NULL});
  printf("start is at %p, sp = %p\n", start, sp);
  asm volatile( //
      "mov %1, %%rsp\n"
      "xor %%rdx, %%rdx\n"
      "jmp *%0\n" ::"r"(start),
      "r"(sp)
      : "memory");
  __builtin_unreachable();
}
