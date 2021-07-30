#ifndef TYPES_H
#define TYPES_H

#ifdef __linux__
#include <elf.h>
#else
#include "elf.h"
#endif

#if ELFCLASS == ELFCLASS32
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Phdr Elf32_Phdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Off Elf32_Off
#define Elf_Half Elf32_Half
#define Elf_Addr Elf32_Addr
#define Elf_auxv_t Elf32_auxv_t
#else
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Phdr Elf64_Phdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Off Elf64_Off
#define Elf_Half Elf64_Half
#define Elf_Addr Elf64_Addr
#define Elf_auxv_t Elf64_auxv_t
#endif

struct Elf_loadsegs {
    uintptr_t addr, p_vaddr, p_memsz;
};

struct Elf_loadmap {
    unsigned short version, nsegs;
    struct Elf_loadsegs segs[];
};

struct Elf_mapping_info_t {
    size_t addr_min, addr_max;
    off_t off_start;
    unsigned mem_protect;
    struct Elf_loadmap *loadmap;
};

struct Elf_handle_t {
    int fd;
    Elf_Ehdr header;
    Elf_Phdr *phdr;
    Elf_Shdr *shdr;
    Elf_Addr dlinfo;
    struct Elf_mapping_info_t *mapping_info;
};

#endif // TYPES_H
