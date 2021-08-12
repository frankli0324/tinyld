int execute(int argc, char **argv) {
    __asm__(
        "mov rdi, 1;"
        "mov rax, 0x0a3174736574;"
        "push rax;"
        "mov rsi, rsp;"
        "mov rdx, 6;"
        "mov rax, 1;"
        "syscall;"
        "pop rax;");
    return 2333;
}
