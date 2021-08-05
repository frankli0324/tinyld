#include <unistd.h>

#include "../syscalls.h"

off_t t_lseek(int fd, off_t offset, int whence) {
    if (fd < 64 || blob_fds[fd - BD_BASE] == NULL)
        return syscall(SYS_lseek, fd, offset, whence);
    struct bd_t *bd = blob_fds[fd - BD_BASE];
    if (whence == SEEK_SET) {
        if (bd->len > offset)
            bd->ptr = (void *)bd->data + offset;
        else
            return -1;
    }
    return bd->ptr - bd->data;
}
