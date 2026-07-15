# statically-linked elf loader

- [x] static-pie elf supported
- [x] faked, forged initial frame (SysV AMD64 ABI)
- [x] original auxv supported
- [x] release used resources
- [x] **can load some simple static glibc-compiled programs**
  - use `vecs` to test it
  - static-pie `lua` (dev version v5.5)
    - `make MYCFLAGS="-DLUA_USE_POSIX" MYLIBS="-ldl" MYLDFLAGS="-static-pie" clean all`

