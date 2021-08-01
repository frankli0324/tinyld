#include <stdlib.h>

#include "syscalls.h"
#include "tinyld.h"
#include "types.h"

int __attribute__((visibility("hidden"))) _t_dlclose(struct Elf_handle_t *handle) {
    if (handle == NULL)
        return 0;
    t_close(handle->fd);
    if (handle->mapping_info != NULL) {
        // unmap elf
        free(handle->mapping_info);
    }
    if (handle->phdr != NULL)
        free(handle->phdr);
    if (handle->shdr != NULL)
        free(handle->shdr);
    free(handle);
    return 0;
}

int t_dlclose(void *handle) {
    return _t_dlclose(handle);
}
