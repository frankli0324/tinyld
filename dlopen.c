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
    char *nametab = (char *)malloc(sh_str->sh_size);
    t_pread(handle->fd, nametab, sh_str->sh_size, sh_str->sh_offset);
    printf("%20s   %-8s   %s\n", "section name", "size", "offset");
    for (int i = 0; i < header->e_shnum; i++) {
        printf("%20s 0x%-8lx 0x%lx\n",
               nametab + handle->shdr[i].sh_name,
               handle->shdr[i].sh_size,
               handle->shdr[i].sh_offset);
    }
    free(nametab);
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
    handle->dl_info = (struct Elf_dynamic_info_t *)malloc(sizeof(struct Elf_dynamic_info_t));
    handle->mapping_info->base = NULL;
    handle->mapping_info->addr_max = 0;
    handle->mapping_info->addr_min = SIZE_MAX;
    handle->mapping_info->off_start = 0;

    for (int i = 0; i < handle->header.e_phnum; i++, phdr++) {
        switch (phdr->p_type) {
        case PT_DYNAMIC:
            handle->dl_info->vaddr = phdr->p_vaddr;
            printf("program dynamic offset %x\tfilesiz %lx\toffset %lx\n",
                   phdr->p_offset, phdr->p_filesz, phdr->p_offset);
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
    }
    return 0;
}

static void *t_fdlopen(int fd, int flags) {
    if (page_size == -1)
        page_size = getpagesize();
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
