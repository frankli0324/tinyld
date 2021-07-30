// wrapper for syscalls

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stddef.h>
#include <sys/types.h>

#define BD_BASE 64
struct bd_t { // blob descriptor
    const void *data;
    size_t len;
    void *ptr;
} * blob_fds[64] __attribute__((visibility("default")));

off_t __attribute__((visibility("hidden"))) t_lseek(int fd, off_t offset, int whence);
ssize_t __attribute__((visibility("hidden"))) t_read(int fd, void *buf, size_t nbyte);
ssize_t __attribute__((visibility("hidden"))) t_pread(int fd, void *buf, size_t nbyte, off_t offset);
int __attribute__((visibility("hidden"))) t_blob_open(const void *blob, int len);
int __attribute__((visibility("hidden"))) t_close(int fd);
void __attribute__((visibility("hidden"))) * t_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int __attribute__((visibility("hidden"))) t_munmap(void *addr, size_t length);

#endif
