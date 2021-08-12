extern long syscall(long t, ...);

int execute(int argc, char **argv) {
    syscall(1, 2, "test2\n", 6);
    return 0;
}
