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

## the idea

https://github.com/m1m1x/memdlopen replaces the functions used for `dlopen`ing, but addresses are fixed for specific ld versions. the code is not reusable. also, it relies on system ld to work.  
the idea is to re-implement the needed code for loading a dylib, and get rid of system linker.
