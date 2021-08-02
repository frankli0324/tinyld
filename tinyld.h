#ifndef TINYLD_H
#define TINYLD_H
#include <stddef.h>

/* The MODE argument to `dlopen' contains one of the following: */
#define RTLD_LAZY 0x00001     /* Lazy function call binding.  */
#define RTLD_NOW 0x00002      /* Immediate function call binding.  */
#define RTLD_BINDING_MASK 0x3 /* Mask of binding time value.  */
#define RTLD_NOLOAD 0x00004   /* Do not load the object.  */
#define RTLD_DEEPBIND 0x00008 /* Use deep binding.  */

#define RTLD_NEXT ((void *)-1)      /* Search subsequent objects. */
#define RTLD_DEFAULT ((void *)-2)   /* Use default search algorithm. */
#define RTLD_SELF ((void *)-3)      /* Search this and subsequent objects (Mac OS X 10.5 and later) */
#define RTLD_MAIN_ONLY ((void *)-5) /* Search main executable only (Mac OS X 10.5 and later) */

/* If the following bit is set in the MODE argument to `dlopen',
   the symbols of the loaded object and its dependencies are made
   visible as if the object were linked directly into the program.  */
#define RTLD_GLOBAL 0x00100
/* Unix98 demands the following flag which is the inverse to RTLD_GLOBAL.
   The implementation does this by default and so we can define the
   value to zero.  */
#define RTLD_LOCAL 0
/* Do not delete object when closed.  */
#define RTLD_NODELETE 0x01000

void *t_dlmopen(const void *blob, size_t len, int flags);
void *t_dlopen(const char *path, int flags);
void *t_dlsym(void *handle, const char *symbol);
int t_dlclose(void *handle);

#endif
