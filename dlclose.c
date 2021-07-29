#include <stdlib.h>

#include "syscalls.h"
#include "tinyld.h"
#include "types.h"

#define free_arr(arr, count)                \
    {                                       \
        for (int i = 0; i < (count); i++) { \
            free(arr[i]);                   \
            arr[i] = NULL;                  \
        }                                   \
        free(arr);                          \
        arr = NULL;                         \
    }

int __attribute__((visibility("hidden"))) _t_dlclose(struct Elf_handle_t *handle) {
    t_close(handle->fd);
    if (handle->phdr != NULL)
        free_arr(handle->phdr, handle->header->e_phnum);
    if (handle->shdr != NULL)
        free_arr(handle->shdr, handle->header->e_shnum);
    free(handle->header);
    return 0;
}

int t_dlclose(void *handle) {
    return _t_dlclose(handle);
}
