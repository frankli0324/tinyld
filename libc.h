#ifndef LIBC_H
#define LIBC_H

#include <errno.h>
#include <unistd.h>

#define hidden __attribute__((visibility("hidden")))
static int rtld_errno;
int hidden *__errno_location(void);

#ifdef errno
#undef errno
#endif
#define errno (*__errno_location())

void hidden *t_memcpy(void *dest, const void *src, size_t n);
char hidden *t_strcpy(char *dest, const char *src);
char hidden *t_strncpy(char *dest, const char *src, size_t n);

int hidden t_memcmp(const void *s1, const void *s2, size_t n);
int hidden t_strcmp(const char *s1, const char *s2);
int hidden t_strncmp(const char *s1, const char *s2, size_t n);

#undef hidden

#endif // LIBC_H
