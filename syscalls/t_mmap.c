#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../libc.h"
#include "../syscalls.h"

void *t_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    const size_t page_size = getpagesize();
    if (fd < 64 || blob_fds[fd - BD_BASE] == NULL) {
        if (offset & (page_size - 1)) {
            errno = EINVAL;
            return MAP_FAILED;
        }
#if defined(SYS_mmap2)
        return (void *)syscall(SYS_mmap2, addr, length, prot, flags, fd, offset / 4096);
#elif defined(SYS_mmap)
        return (void *)syscall(SYS_mmap, addr, length, prot, flags, fd, offset);
#else
#error "no mmap syscall available... really?"
#endif
    }
    struct bd_t *bd = blob_fds[fd - BD_BASE];
    //  0x802 : MAP_PRIVATE,MAP_DENYWRITE
    //  0x812 : MAP_PRIVATE,MAP_FIXED,MAP_DENYWRITE
    int mflags = MAP_PRIVATE | MAP_ANONYMOUS;
    if ((flags & MAP_FIXED) != 0) {
        mflags |= MAP_FIXED;
    }
    void *ret = t_mmap(addr, length, PROT_READ | PROT_WRITE | PROT_EXEC, mflags, -1, 0);
    if (ret == MAP_FAILED)
        return MAP_FAILED;
    t_memcpy(ret, bd->data + offset, length > bd->len - offset ? bd->len - offset : length);

    void *start = (void *)((uintptr_t)ret & (((size_t)-1) ^ (page_size - 1)));
    while (start < ret) {
        syscall(SYS_mprotect, start, page_size, prot);
        start += page_size;
    }
    return ret;
}

int t_munmap(void *addr, size_t length) {
    return syscall(SYS_munmap, addr, length);
}

int t_mprotect(void *addr, size_t len, int prot) {
    const size_t page_size = getpagesize();
    size_t start, end;
    start = (size_t)addr & -page_size;
    end = (size_t)((char *)addr + len + page_size - 1) & -page_size;
    return syscall(SYS_mprotect, start, end - start, prot);
}
