#include <stdio.h>
#include <stdlib.h>
#include <sys/auxv.h>
#include <unistd.h>

#include "tinyld.h"

int main(int argc, char **argv) {
    if (argc < 2)
        return 0;
    FILE *fp = fopen(argv[1], "r");
    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp);
    char *blob = (char *)malloc(sz);
    fseek(fp, 0, SEEK_SET);
    fread(blob, sz, 1, fp);
    void *handle = t_dlmopen(blob, sz, RTLD_NOW);
    int (*execute)(int, char **) = t_dlsym(handle, "execute");
    int ret = execute(argc, argv);
    t_dlclose(handle);
    return ret;
}
