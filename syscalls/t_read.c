#include <errno.h>
#include <unistd.h>

#include "../syscalls.h"

extern struct bd_t *blob_fds[64];

ssize_t t_read(int fd, void *buf, size_t count) {
    if (fd < 64 || blob_fds[fd - BD_BASE] == NULL)
        return syscall(SYS_read, fd, buf, count);
    struct bd_t *bd = blob_fds[fd - BD_BASE];
    if (bd->ptr - bd->data == bd->len)
        return -1; // EOF
    unsigned char *p = buf;
    for (ssize_t i = 0; i < count; i++) {
        *(p + i) = *((unsigned char *)bd->ptr++);
        if (bd->ptr - bd->data == bd->len)
            return i;
    }
    return count;
}

ssize_t t_pread(int fd, void *buf, size_t nbyte, off_t offset) {
    if (fd < 64 || blob_fds[fd - BD_BASE] == NULL) {
        // https://man7.org/linux/man-pages/man2/syscall.2.html#:~:text=Welcome
        return syscall(SYS_pread64, fd, buf, nbyte, offset);
    }
    struct bd_t *bd = blob_fds[fd - BD_BASE];
    unsigned char *p = buf;
    for (ssize_t i = 0; i < nbyte; i++) {
        *(p + i) = ((unsigned char *)bd->data)[offset + i];
        if (offset + i == bd->len)
            return i;
    }
    return nbyte;
}
