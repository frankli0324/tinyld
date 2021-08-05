#include "../libc.h"

int *__errno_location(void) {
    return &rtld_errno;
}
