#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <unistd.h>

#include "internals.h"
#include "libc.h"
#include "syscalls.h"
#include "tinyld.h"

static size_t page_size = -1;
struct Elf_handle_t *self = NULL;

static int t_read_ehdr(struct Elf_handle_t *handle) {
    return t_pread(handle->fd, &handle->header, sizeof(Elf_Ehdr), 0) != sizeof(Elf_Ehdr);
}

static int t_read_shdr(struct Elf_handle_t *handle) {
    Elf_Ehdr *header = &handle->header;
    size_t total = header->e_shentsize * header->e_shnum;
    handle->shdr = (Elf_Shdr *)malloc(total);
    total -= t_pread(handle->fd, handle->shdr, total, header->e_shoff);
    return total;
}

static int t_read_phdr(struct Elf_handle_t *handle) {
    Elf_Ehdr *header = &handle->header;
    size_t total = header->e_phentsize * header->e_phnum;
    handle->phdr = (Elf_Phdr *)malloc(total);
    total -= t_pread(handle->fd, handle->phdr, total, header->e_phoff);
    return total;
}

static int t_premap_segments(struct Elf_handle_t *handle, int flags) {
    unsigned mem_protect;
    size_t cnt_seg_load = 0;
    Elf_Phdr *phdr = handle->phdr;
    if (handle->mapping_info != NULL)
        free(handle->mapping_info);
    handle->mapping_info = (struct Elf_mapping_info_t *)calloc(1, sizeof(struct Elf_mapping_info_t));
    handle->dl_info = (struct Elf_dynamic_info_t *)calloc(1, sizeof(struct Elf_dynamic_info_t));
    handle->reloc_info = (struct Elf_reloc_info_t *)calloc(1, sizeof(struct Elf_reloc_info_t));
    handle->mapping_info->addr_min = SIZE_MAX;

    for (int i = 0; i < handle->header.e_phnum; i++, phdr++) {
        switch (phdr->p_type) {
        case PT_DYNAMIC:
            handle->dl_info->vaddr = phdr->p_vaddr;
            break;
        case PT_LOAD:
            cnt_seg_load++;
            if (phdr->p_vaddr < handle->mapping_info->addr_min) {
                handle->mapping_info->addr_min = phdr->p_vaddr;
                handle->mapping_info->off_start = phdr->p_offset;
            }
            if (phdr->p_vaddr + phdr->p_memsz > handle->mapping_info->addr_max) {
                handle->mapping_info->addr_max = phdr->p_vaddr + phdr->p_memsz;
            }
            break;
        case PT_GNU_RELRO:
            handle->reloc_info->relro_start = phdr->p_vaddr & -page_size;
            handle->reloc_info->relro_end = (phdr->p_vaddr + phdr->p_memsz) & -page_size;
            break;
        }
    }

    if (NULL == (handle->mapping_info->loadmap = (struct Elf_loadmap *)malloc(
                     sizeof(struct Elf_loadmap) +
                     cnt_seg_load * sizeof(struct Elf_loadsegs))))
        return -1;
    handle->mapping_info->loadmap->nsegs = cnt_seg_load;
    return 0;
}

static int t_map_library(struct Elf_handle_t *handle, int flags) {
    if (handle->dl_info == NULL)
        return -1; // no dynamic segment found
    size_t addr_max = handle->mapping_info->addr_max;
    size_t addr_min = handle->mapping_info->addr_min;
    off_t off_start = handle->mapping_info->off_start;
    size_t map_len = addr_max - addr_min + off_start;

    addr_max += page_size - 1;
    addr_max &= -page_size;
    addr_min &= -page_size;
    off_start &= -page_size;

    void *base = t_mmap(NULL, map_len,
                        PROT_EXEC | PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED)
        return -1;
    /* from musl libc, doesn't know why:
     * If the loaded file is not relocatable and the requested address is
	 * not available, then the load operation must fail. */
    if (handle->header.e_type != ET_DYN && addr_min && base != (void *)addr_min)
        return -1;
    // enough space for following mapping process
    handle->mapping_info->base = base - addr_min; // successfully mapped

    Elf_Phdr *phdr = handle->phdr;
    // start mapping each segment in place
    for (int i = 0; i < handle->header.e_phnum; i++, phdr++) {
        if (phdr->p_type != PT_LOAD)
            continue;
        size_t addr_low = phdr->p_vaddr & -page_size;
        size_t addr_high = phdr->p_vaddr + phdr->p_memsz + page_size - 1 & -page_size;
        off_t off_start = phdr->p_offset & -page_size;
        unsigned prot = (((phdr->p_flags & PF_R) ? PROT_READ : 0) |
                         ((phdr->p_flags & PF_W) ? PROT_WRITE : 0) |
                         ((phdr->p_flags & PF_X) ? PROT_EXEC : 0));
        if (t_mmap(base + addr_low, addr_high - addr_low,
                   prot, MAP_PRIVATE | MAP_FIXED,
                   handle->fd, off_start) == MAP_FAILED) {
            return -1;
        }
        handle->mapping_info->loadmap->segs[i].addr = base + addr_low;
        handle->mapping_info->loadmap->segs[i].phdr_ndx = i;
    }
    return 0;
}

static int t_decode_dynamic(struct Elf_handle_t *handle) {
    void *base = handle->mapping_info->base;
    size_t *dynv = base + handle->dl_info->vaddr;
    for (int i = 0; dynv[i]; i += 2) {
#define value (base + dynv[i + 1])
#define case_assign(c, assign) \
    case c:                    \
        assign = value;        \
        break;

        switch (dynv[i]) {
            case_assign(DT_PLTGOT, handle->dl_info->got);
            case_assign(DT_HASH, handle->dl_info->hashtab);
            case_assign(DT_STRTAB, handle->dl_info->strtab);
            case_assign(DT_SYMTAB, handle->dl_info->symtab);
        case DT_RUNPATH:
            // printf("library search path offset: %s\n", dynv[i + 1]);
            break;
            case_assign(DT_GNU_HASH, handle->dl_info->ghashtab);
            case_assign(DT_VERSYM, handle->dl_info->gversym); // .gnu.version section addr
        }

#undef case_assign
#undef value
    }
}

static void t_do_relocate(struct Elf_handle_t *handle, size_t *rel, size_t rel_size, size_t step) {
    void *base = handle->mapping_info->base;
    size_t *got = handle->dl_info->got;
    Elf_Sym *symtab = handle->dl_info->symtab;
    char *strtab = handle->dl_info->strtab;
    for (; rel_size; rel += step, rel_size -= step * sizeof(size_t)) {
        int type = ELF_R_TYPE(rel[1]);
        if (type == R_(NONE))
            continue;
        size_t *reloc_addr = handle->mapping_info->base + rel[0];
        int symndx = ELF_R_SYM(rel[1]);
        if (symndx != 0) { // symbol relocation
            switch (type) {
            case R_(JUMP_SLOT):
                if (t_strcmp(symtab[symndx].st_name + strtab, "syscall") == 0)
                    *reloc_addr = (size_t)syscall;
                else
                    *reloc_addr += (size_t)base;
                continue;
            }
        }

        size_t addend = step == 3 ? rel[2] : *reloc_addr;
        switch (type) {
        case R_(PC32):
            addend -= (size_t)reloc_addr;
        case R_(SYMBOLIC):  //direct
        case R_(GLOB_DAT):  // got
        case R_(JUMP_SLOT): // plt
            *reloc_addr = symtab[symndx].st_value + addend;
            break;
        case R_(RELATIVE):
            *reloc_addr = (size_t)base + addend;
            break;
        case R_(COPY):
            t_memcpy(reloc_addr, base + symtab[symndx].st_value, symtab[symndx].st_size);
            break;
        default:
            // printf("unable to relocate type %d\n", type);
            syscall(SYS_exit, -1);
        }
    }
}

static int t_relocate_handle(struct Elf_handle_t *handle) {
    void *base = handle->mapping_info->base;
    size_t *dynv = base + handle->dl_info->vaddr;
    size_t dynv_vec[35] = {0}; // C99 [$6.7.8/21]
    for (int i = 0; dynv[i]; i += 2)
        if (dynv[i] >= 0 && dynv[i] < 35)
            dynv_vec[dynv[i]] = dynv[i + 1];
    t_do_relocate(handle, handle->mapping_info->base + dynv_vec[DT_JMPREL], dynv_vec[DT_PLTRELSZ], 2 + (dynv_vec[DT_PLTREL] == DT_RELA));
    t_do_relocate(handle, handle->mapping_info->base + dynv_vec[DT_REL], dynv_vec[DT_RELSZ], 2);
    t_do_relocate(handle, handle->mapping_info->base + dynv_vec[DT_RELA], dynv_vec[DT_RELASZ], 3);
    size_t *got = handle->dl_info->got, *self_got = self->dl_info->got;
    got[1] = self_got[1];
    got[2] = self_got[2];
    if (handle->reloc_info->relro_start != handle->reloc_info->relro_end) {
        if (t_mprotect(
                handle->mapping_info->base + handle->reloc_info->relro_start,
                handle->reloc_info->relro_end - handle->reloc_info->relro_start,
                PROT_READ)) {
            if (0 || errno != ENOSYS) {
                // Error relocating: RELRO protection failed
            }
        }
    }
    return 0;
}

static int t_parse_auxv() {
    // 2.6.0-test7
    int fd = syscall(SYS_open, "/proc/self/auxv", O_RDONLY);
    size_t auxv[32];
    syscall(SYS_read, fd, auxv, 32 * sizeof(size_t));
    size_t nphdr = 0;
    void *entry;
    for (int i = 0; i < 32; i += 2) {
        switch (auxv[i]) {
        case AT_PHDR:
            self->phdr = (void *)auxv[i + 1];
            break;
        case AT_PHNUM:
            nphdr = auxv[i + 1];
            break;
        }
    }

    for (size_t i = 0; i < nphdr; i++) {
        if (self->phdr[i].p_type == PT_DYNAMIC) {
            self->dl_info->vaddr = self->phdr[i].p_vaddr;
            break;
        }
    }
    if (self->dl_info->vaddr == 0) {
        // no dynamic segment found
        syscall(SYS_exit, -1);
    }
}

static int t_collect_env() {
    // t_parse_maps();
    self = (struct Elf_handle_t *)calloc(1, sizeof(struct Elf_handle_t));
    self->mapping_info = (struct Elf_mapping_info_t *)calloc(1, sizeof(struct Elf_mapping_info_t));
    self->dl_info = (struct Elf_dynamic_info_t *)calloc(1, sizeof(struct Elf_dynamic_info_t));
    int ret = t_parse_auxv();
    for (size_t mem = ((size_t)self->phdr & (-page_size)), i = 0;
         i < 5; mem -= page_size, i++) {

        if (t_memcmp((void *)mem, ELFMAG, 4) == 0) {
            self->mapping_info->base = (void *)mem;
            break;
        }
    }
    size_t *dynv = self->mapping_info->base + self->dl_info->vaddr;
    for (size_t i = 0; i < 32; i += 2)
        if (dynv[i] == DT_PLTGOT)
            self->dl_info->got = (void *)dynv[i + 1];
    return ret;
}

static void *t_fdlopen(int fd, int flags) {
    if (page_size == -1)
        page_size = getpagesize();
    if (self == NULL)
        t_collect_env();
    if (fd < 0)
        return NULL;
    struct Elf_handle_t *handle = (struct Elf_handle_t *)calloc(1, sizeof(struct Elf_handle_t));
    if (handle == NULL)
        goto error;
    handle->fd = fd;
    t_read_ehdr(handle);
    t_read_phdr(handle);
    t_read_shdr(handle);
    if (t_premap_segments(handle, flags) != 0)
        goto error;

    if (t_map_library(handle, flags) != 0)
        goto error;

    if (t_decode_dynamic(handle) != 0)
        goto error;

    // TODO: load DT_NEEDED
    // only static libraies allowed without this

    if (t_relocate_handle(handle) != 0)
        goto error;

    return handle;
error:
    t_dlclose(handle);
    return NULL;
}

void *t_dlopen(const char *path, int flags) {
    int fd = syscall(SYS_open, path, O_RDONLY);
    if (fd == -1)
        return NULL;
    return t_fdlopen(fd, flags);
}

void *t_dlmopen(const void *blob, size_t len, int flags) {
    int fd = t_blob_open(blob, len);
    if (fd == -1)
        return NULL;
    return t_fdlopen(fd, flags);
}
