#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "syscalls.h"
#include "tinyld.h"
#include "types.h"

int main(int argc, char **argv) {
    t_dlopen("./cron.so", 0);
}
