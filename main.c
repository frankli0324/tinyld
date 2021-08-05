#include <stdio.h>
#include <stdlib.h>
#include <sys/auxv.h>
#include <unistd.h>

#include "arch/target/start.h"
#include "tinyld.h"

void _start_c(size_t *sp) {
    int argc = *(sp++);
    char **argv = (char **)sp++;
    sp += argc;
    char **envp = (char **)sp;
    int (*mainp)() = mainp;
    syscall(60, mainp(argc, argv, envp));
}

int main(int argc, char **argv) {
    if (argc < 2)
        return 0;
    void *handle = t_dlopen(argv[1], RTLD_NOW);
    int (*execute)(int, char **) = t_dlsym(handle, "execute");
    int ret = execute(argc, argv);
    t_dlclose(handle);
    return ret;
}
