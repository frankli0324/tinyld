#ifndef TYPES_H
#define TYPES_H

#ifdef __linux__
#include <elf.h>
#else
#include "elf.h"
#endif
#include <fcntl.h>

#include "arch/target/relocs.h"
#include "arch/target/types.h"

typedef uint32_t Elf_Hashtab;
// musl libc says on "S390x" systems hashtab_t is `uint64_t`, what's that?
// https://en.wikipedia.org/wiki/Linux_on_IBM_Z
typedef uint32_t Elf_GHashtab;

struct Elf_loadsegs {
    void *addr;
    size_t phdr_ndx;
};

struct Elf_loadmap {
    unsigned short version, nsegs;
    struct Elf_loadsegs segs[];
};

struct Elf_mapping_info_t {
    void *base;
    size_t addr_min, addr_max;
    off_t off_start;
    struct Elf_loadmap *loadmap;
};

struct Elf_dynamic_info_t {
    Elf_Addr vaddr;
    char *strtab;
    void *got;
    Elf_Sym *symtab;
    Elf_Hashtab *hashtab;   // sysv hashtab
    Elf_GHashtab *ghashtab; // gnu hashtab
    int16_t *gversym;       // gnu version
};

struct Elf_reloc_info_t {
    size_t relro_start, relro_end;
};

struct Elf_handle_t {
    int fd;
    Elf_Ehdr header;
    Elf_Phdr *phdr;
    Elf_Shdr *shdr;
    struct Elf_mapping_info_t *mapping_info;
    struct Elf_dynamic_info_t *dl_info;
    struct Elf_reloc_info_t *reloc_info;
};

struct link_map {
    Elf_Addr l_addr;
    char *l_name;
    Elf_Dyn *l_ld;
    struct link_map *l_next, *l_prev;
};

#endif // TYPES_H
