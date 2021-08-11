extern long syscall(long t, ...);
extern int strcmp(char *, char *);

int execute(int argc, char **argv) {
    if (strcmp("test2", "test2") == 0)
        syscall(1, 2, "test\n", 5);
    return 0;
}
