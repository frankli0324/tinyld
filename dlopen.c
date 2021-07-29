#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "syscalls.h"
#include "tinyld.h"
#include "types.h"

Elf_Ehdr __attribute__((visibility("hidden"))) * t_read_ehdr(struct Elf_handle_t *handle) {
    Elf_Ehdr *header = (Elf_Ehdr *)malloc(sizeof(Elf_Ehdr));
    t_lseek(handle->fd, 0, SEEK_SET);
    t_read(handle->fd, header, sizeof(Elf_Ehdr));
    return header;
}

Elf_Shdr __attribute__((visibility("hidden"))) * *t_read_shdr(struct Elf_handle_t *handle) {
    Elf_Ehdr *header = handle->header;
    t_lseek(handle->fd, handle->header->e_shoff, SEEK_SET);
    handle->shdr = (Elf_Shdr **)malloc(header->e_shentsize * header->e_shnum);
    for (int i = 0; i < header->e_phnum; i++) {
        Elf_Shdr *shdr = (Elf_Shdr *)malloc(header->e_shentsize);
        t_read(handle->fd, shdr, header->e_shentsize);
        printf("section name 0x%x\tsize 0x%lx\toffset 0x%lx\n",
               shdr->sh_name, shdr->sh_size, shdr->sh_offset);
        handle->shdr[i] = shdr;
    }
    t_lseek(handle->fd, header->e_shstrndx, SEEK_SET);
    return handle->shdr;
}

Elf_Phdr __attribute__((visibility("hidden"))) * *t_read_phdr(struct Elf_handle_t *handle) {
    Elf_Ehdr *header = handle->header;
    t_lseek(handle->fd, header->e_phoff, SEEK_SET);
    handle->phdr = (Elf_Phdr **)malloc(header->e_phentsize * header->e_phnum);
    for (int i = 0; i < header->e_phnum; i++) {
        Elf_Phdr *phdr = (Elf_Phdr *)malloc(header->e_phentsize);
        t_read(handle->fd, phdr, header->e_phentsize);
        handle->phdr[i] = phdr;
    }
    return handle->phdr;
}

int __attribute__((visibility("hidden"))) t_load_program(struct Elf_handle_t *handle, int flags) {
    for (int i = 0; i < handle->header->e_phnum; i++) {
        if (handle->phdr[i]->p_type == PT_LOAD) {
            Elf_Phdr *phdr = handle->phdr[i];
            printf("program type %x\tfilesiz %lx\toffset %lx\n",
                   phdr->p_type, phdr->p_filesz, phdr->p_offset);
        }
    }
    return 0;
}

void __attribute__((visibility("hidden"))) * t_fdlopen(int fd, int flags) {
    if (fd < 0)
        return NULL;
    struct Elf_handle_t *handle = (struct Elf_handle_t *)malloc(sizeof(struct Elf_handle_t));
    handle->fd = fd;
    handle->header = t_read_ehdr(handle);
    printf("program header offset: %ld\n", handle->header->e_phoff);
    printf("program header count: %d\n", handle->header->e_phnum);
    t_read_phdr(handle);
    t_read_shdr(handle);
    if (t_load_program(handle, flags) != 0) {
        // error
    }
    return handle;
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
