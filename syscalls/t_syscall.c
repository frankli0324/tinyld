#include <stdarg.h>

#include "../arch/target/syscall.h"
#include "../libc.h"
#include "../syscall.h"

static long __syscall_ret(unsigned long r) {
    if (r > -4096UL) {
        errno = -r;
        return -1;
    }
    return r;
}

#define __SYSCALL_NARGS_X(a, b, c, d, e, f, g, h, n, ...) n
#define __SYSCALL_NARGS(...) __SYSCALL_NARGS_X(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0, )

#define __SYSCALL_CONCAT_X(a, b) a##b
#define __SYSCALL_CONCAT(a, b) __SYSCALL_CONCAT_X(a, b)

#define __SYSCALL_CALL(...)                                   \
    __SYSCALL_CONCAT(__syscall, __SYSCALL_NARGS(__VA_ARGS__)) \
    (__VA_ARGS__)

long syscall(long n, ...) {
    va_list ap;
    long a, b, c, d, e, f;
    va_start(ap, n);
    a = va_arg(ap, long);
    b = va_arg(ap, long);
    c = va_arg(ap, long);
    d = va_arg(ap, long);
    e = va_arg(ap, long);
    f = va_arg(ap, long);
    va_end(ap);
    return __syscall_ret(__SYSCALL_CALL(n, a, b, c, d, e, f));
}
