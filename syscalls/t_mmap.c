#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../syscalls.h"

void *t_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    if (fd < 64 || blob_fds[fd - BD_BASE] == NULL)
        return mmap(addr, length, prot, flags, fd, offset);
    const size_t page_size = getpagesize();
    struct bd_t *bd = blob_fds[fd - BD_BASE];
    //  0x802 : MAP_PRIVATE,MAP_DENYWRITE
    //  0x812 : MAP_PRIVATE,MAP_FIXED,MAP_DENYWRITE
    int mflags = MAP_PRIVATE | MAP_ANONYMOUS;
    if ((flags & MAP_FIXED) != 0) {
        mflags |= MAP_FIXED;
    }
    void *ret = mmap(addr, length, PROT_READ | PROT_WRITE | PROT_EXEC, mflags, -1, 0);
    if (ret == MAP_FAILED)
        return MAP_FAILED;
    memcpy(ret, bd->data, length > bd->len ? bd->len : length);

    void *start = (void *)((uintptr_t)ret & (((size_t)-1) ^ (page_size - 1)));
    while (start < ret) {
        mprotect(start, page_size, prot);
        start += page_size;
    }
    printf("mmap : [0x%lx,0x%lx]", (uint64_t)ret, (uint64_t)ret + length);
    return ret;
}

int t_munmap(void *addr, size_t length) {
    return -1;
}