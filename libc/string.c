#include "../libc.h"

#define _t_cpy(whl)                        \
    if ((dest != NULL) && (src != NULL)) { \
        char *cdest = dest;                \
        const char *csrc = src;            \
        while (whl)                        \
            *(cdest++) = *(csrc++);        \
    }                                      \
    return dest;

void *t_memcpy(void *dest, const void *src, size_t n) {
    _t_cpy(n--);
}

char *t_strcpy(char *dest, const char *src) {
    _t_cpy(*csrc != '\0');
}

char *t_strncpy(char *dest, const char *src, size_t n) {
    _t_cpy(n--);
}

int t_strcmp(const char *s1, const char *s2) {
    while (*s1 != '\0' && *s2 != '\0') {
        if (*s1 - *s2 != 0)
            return *s1 - *s2;
        s1++, s2++;
    }
    return *s1 - *s2;
}

int t_strncmp(const char *s1, const char *s2, size_t n) {
    while (n--) {
        if (*s1 - *s2 != 0)
            return *s1 - *s2;
        s1++, s2++;
    }
    return 0;
}

int t_memcmp(const void *s1, const void *s2, size_t n) {
    return t_strncmp(s1, s2, n);
}
