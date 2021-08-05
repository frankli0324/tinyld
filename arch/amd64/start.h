#ifndef START_H
#define START_H

void _start() {
    __asm__(
        "pop %rax;"
        "mov %rsp,%rdi;"
        "call _start_c;");
}

#endif // START_H
