# tiny elf loader

## usage

```sh
make tinyld.a
# or
make ARCH=i386 tinyld.a
```

take the generated `tinyld.a` and the header file `tinyld.h`

include the header in source.c:

```c
#include "tinyld.h"

void main (){
    void *blob = malloc(BLOB_SZ);
    read(blob, somewhere, size);
    void *handle = t_dlmopen(blob, size);
    void *symbol = t_dlsym(handle, "symbol");
}
```

and compile it with

```sh
gcc source.c tinyld.a
./a.out
```

try it out!

```sh
make test
```

## the idea

https://github.com/m1m1x/memdlopen replaces the functions used for `dlopen`ing, but addresses are fixed for specific ld versions. the code is not reusable. also, it relies on system ld to work.  
the idea is to re-implement the needed code for loading a dylib, and get rid of system linker.

## pitfalls

libc is now firmly bounded to the loader. the loader needs to find the `link_map` to load a library, which is hidden in the libc, so in order to build a loader, we must re-implement the libc entirely, which is quite impractical.

an idea suggests that if we write a loader that is capable of loading the `libc.so`, we might build the `link_map` on our own. but the `libc.so` for glibc also has tons of GNU specified relocations, thread stuff, etc. it's like hell implementing a loader for glibc.

glibc is a total disaster.

## workarounds

a workaround is to implement only the functions that may be used in the loader, and manually write them to the GOT of the loaded library. see `libc.h`, `libc/*.c`. this is currently implemented.

another one is quite dirty, but might be somewhat usable. since the loader itself is compiled non-statically, the loader itself has the `link_map` and `dl_runtime_resolve()` stored in it's GOT already. as long as we have an elegant way of finding our own GOT, we can put them into the GOT of the loaded library. it's just... we don't have an elegant way of doing this. is there any?
