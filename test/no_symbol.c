int execute(int argc, char **argv) {
    __asm__(
        "mov rdi, 1;"
        "mov rax, 0x6161616162626262;"
        "push rax;"
        "mov rsi, rsp;"
        "mov rdx, 8;"
        "mov rax, 1;"
        "syscall;"
        "pop rax;");
    return 2333;
}
