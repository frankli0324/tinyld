// wrapper for syscalls

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stddef.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define hidden __attribute__((visibility("hidden")))

#define BD_BASE 64
const struct bd_t { // blob descriptor
    const void *data;
    size_t len;
    void *ptr;
};

long hidden syscall(long n, ...);
off_t hidden t_lseek(int fd, off_t offset, int whence);
ssize_t hidden t_read(int fd, void *buf, size_t nbyte);
ssize_t hidden t_pread(int fd, void *buf, size_t nbyte, off_t offset);
int hidden t_blob_open(const void *blob, int len);
int hidden t_close(int fd);
void hidden *t_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int hidden t_munmap(void *addr, size_t length);
int hidden t_mprotect(void *addr, size_t len, int prot);

#endif
