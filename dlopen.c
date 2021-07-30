#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "syscalls.h"
#include "tinyld.h"
#include "types.h"

static size_t page_size = -1;

static int t_read_ehdr(struct Elf_handle_t *handle) {
    return t_pread(handle->fd, &handle->header, sizeof(Elf_Ehdr), 0) != sizeof(Elf_Ehdr);
}

static int t_read_shdr(struct Elf_handle_t *handle) {
    Elf_Ehdr *header = &handle->header;
    size_t total = header->e_shentsize * header->e_shnum;
    handle->shdr = (Elf_Shdr *)malloc(total);
    total -= t_pread(handle->fd, handle->shdr, total, header->e_shoff);

    Elf_Shdr *sh_str = handle->shdr + header->e_shstrndx;
    char name[64];
    printf("%20s   %-8s   %s\n", "section name", "size", "offset");
    for (int i = 0; i < header->e_shnum; i++) {
        t_pread(handle->fd, name, 64, sh_str->sh_offset + handle->shdr[i].sh_name);
        printf("%20s 0x%-8lx 0x%lx\n",
               name, handle->shdr[i].sh_size, handle->shdr[i].sh_offset);
    }
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
    handle->mapping_info = (struct Elf_mapping_info_t *)malloc(sizeof(struct Elf_mapping_info_t));
    handle->mapping_info->addr_max = 0;
    handle->mapping_info->addr_min = SIZE_MAX;
    handle->mapping_info->off_start = 0;

    for (int i = 0; i < handle->header.e_phnum; i++, phdr++) {
        switch (phdr->p_type) {
        case PT_DYNAMIC:
            handle->dlinfo = phdr->p_vaddr;
            printf("program dynamic vaddr %x\tfilesiz %lx\toffset %lx\n",
                   phdr->p_vaddr, phdr->p_filesz, phdr->p_offset);
            break;
        case PT_LOAD:
            cnt_seg_load++;
            if (phdr->p_vaddr < handle->mapping_info->addr_min) {
                handle->mapping_info->addr_min = phdr->p_vaddr;
                handle->mapping_info->off_start = phdr->p_offset;
                mem_protect = (((phdr->p_flags & PF_R) ? PROT_READ : 0) |
                               ((phdr->p_flags & PF_W) ? PROT_WRITE : 0) |
                               ((phdr->p_flags & PF_X) ? PROT_EXEC : 0));
            }
            if (phdr->p_vaddr + phdr->p_memsz > handle->mapping_info->addr_max) {
                handle->mapping_info->addr_max = phdr->p_vaddr + phdr->p_memsz;
            }
            break;
        case PT_GNU_RELRO:
            printf("gnu_relro filesiz %lx\toffset %lx\n",
                   phdr->p_filesz, phdr->p_offset);
            break;
        }
    }

    if (NULL == (handle->mapping_info->loadmap = (struct Elf_loadmap *)malloc(
                     sizeof(struct Elf_loadmap) +
                     cnt_seg_load * sizeof(struct Elf_loadsegs))))
        return -1;
    handle->mapping_info->loadmap->nsegs = cnt_seg_load;
    handle->mapping_info->mem_protect = mem_protect;
    return 0;
}

static int t_map_library(struct Elf_handle_t *handle, int flags) {
    if (handle->dlinfo == 0)
        return -1; // no dynamic segment found
    size_t addr_max = handle->mapping_info->addr_max;
    size_t addr_min = handle->mapping_info->addr_min;
    off_t off_start = handle->mapping_info->off_start;
    size_t map_len = addr_max - addr_min + off_start;

    addr_max += page_size - 1;
    addr_max &= -page_size;
    addr_min &= -page_size;
    off_start &= -page_size;

    void *map = t_mmap((void *)addr_min, map_len, handle->mapping_info->mem_protect,
                       MAP_PRIVATE, handle->fd, off_start);
    printf("%p\n", map);
    if (map == MAP_FAILED)
        return -1;
}

static void *t_fdlopen(int fd, int flags) {
    if (page_size == -1)
        page_size = getpagesize();
    if (fd < 0)
        return NULL;
    struct Elf_handle_t *handle = (struct Elf_handle_t *)malloc(sizeof(struct Elf_handle_t));
    handle->fd = fd;
    t_read_ehdr(handle);
    t_read_phdr(handle);
    t_read_shdr(handle);
    if (t_premap_segments(handle, flags) != 0)
        goto error;

    if (t_map_library(handle, flags) != 0)
        goto error;

    return handle;
error:
    t_dlclose(handle);
    return NULL;
}

void *t_dlopen(const char *path, int flags) {
    int fd = open(path, O_RDONLY);
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
