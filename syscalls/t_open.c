#include <stdlib.h>
#include <unistd.h>

#include "../syscalls.h"

int t_blob_open(const void *blob, int len) {
    struct bd_t *bd = (struct bd_t *)malloc(sizeof(struct bd_t));
    bd->data = blob;
    bd->len = len;
    bd->ptr = (void *)bd->data;
    for (int i = 0; i < 64; i++) {
        if (blob_fds[i] != NULL)
            continue;
        blob_fds[i] = bd;
        return i + BD_BASE;
    }
    return -1;
}

int t_close(int fd) {
    if (fd < 64 || blob_fds[fd - BD_BASE] == NULL)
        return close(fd);
    struct bd_t *bd = blob_fds[fd - BD_BASE];
    free((void *)bd->data);
    bd->data = bd->ptr = NULL;
    free(bd);
    blob_fds[fd - BD_BASE] = NULL;
    return 0;
}
