#ifndef TINYLD_H
#define TINYLD_H
#include <stddef.h>

void *t_dlmopen(const void *blob, size_t len, int flags);
void *t_dlopen(const char *path, int flags);
int t_dlclose(void *handle);

#endif
