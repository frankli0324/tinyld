#include <stdio.h>
#include <string.h>

#include "tinyld.h"
#include "types.h"

static uint32_t sysv_hash(const char *s0) {
    const unsigned char *s = (void *)s0;
    uint_fast32_t h = 0;
    while (*s) {
        h = 16 * h + *s++;
        h ^= h >> 24 & 0xf0;
    }
    return h & 0xfffffff;
}

static Elf_Sym *sysv_lookup(struct Elf_handle_t *handle, const char *s, uint32_t h) {
    Elf_Sym *syms = handle->dl_info->symtab;
    Elf_Hashtab *hashtab = handle->dl_info->hashtab;
    int16_t *gversym = handle->dl_info->gversym;
    char *strtab = handle->dl_info->strtab;
    for (size_t i = hashtab[2 + h % hashtab[0]]; i; i = hashtab[2 + hashtab[0] + i]) {
        if ((!gversym || gversym[i] >= 0) && (!strcmp(s, strtab + syms[i].st_name)))
            return syms + i;
    }
    return 0;
}

static uint32_t gnu_hash(const char *s0) {
    const unsigned char *s = (void *)s0;
    uint_fast32_t h = 5381;
    for (; *s; s++)
        h += h * 32 + *s;
    return h;
}

static int t_is_invalid_handle(struct Elf_handle_t *handle) {
    return 0;
}

static size_t t_count_sym(struct Elf_handle_t *handle) {
    if (handle->dl_info->hashtab != NULL)
        return handle->dl_info->hashtab[1];
    Elf_GHashtab *ghashtab = handle->dl_info->ghashtab;
    size_t nsym, i;
    uint32_t *buckets = ghashtab + 4 + (ghashtab[2] * sizeof(size_t) / 4);
    uint32_t *hashval;
    for (i = nsym = 0; i < ghashtab[0]; i++) {
        if (buckets[i] > nsym)
            nsym = buckets[i];
    }
    if (nsym) {
        hashval = buckets + ghashtab[0] + (nsym - ghashtab[1]);
        do
            nsym++;
        while (!(*hashval++ & 1));
    }
    return nsym;
}

static void *t_find_sym(struct Elf_handle_t *handle, const char *symbol) {
    printf("%d\n", t_count_sym(handle));
    uint32_t hash = sysv_hash(symbol);
    return sysv_lookup(handle, symbol, hash);
}

void *t_dlsym(void *handle, const char *symbol) {
    if (handle == RTLD_DEFAULT) {
    }
    if (((struct Elf_handle_t *)handle)->dl_info->vaddr == 0)
        return NULL; // not dynamic library
    if (t_is_invalid_handle(handle)) {
    }

    if (0) { // https://github.com/mickael-guene/fdpic_doc/blob/master/abi.txt
    }
    return t_find_sym(handle, symbol);
}