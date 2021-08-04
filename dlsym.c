#include <string.h>

#include "internals.h"
#include "tinyld.h"

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

static Elf_Sym *gnu_lookup(struct Elf_handle_t *handle, const char *s, uint32_t h) {
    Elf_Sym *syms = handle->dl_info->symtab;
    char *strtab = handle->dl_info->strtab;
    int16_t *gversym = handle->dl_info->gversym;
    Elf_GHashtab *hashtab = handle->dl_info->ghashtab;
    const size_t *bloomwords = (const void *)(hashtab + 4);
    size_t f = bloomwords[(h / (8 * sizeof(size_t))) & (hashtab[2] - 1)];
    if (!(f & (1ul << h % (8 * sizeof(size_t)))))
        return 0;

    f >>= (h >> hashtab[3]) % (8 * sizeof f);
    if (!(f & 1))
        return 0;

    uint32_t nbuckets = hashtab[0];
    uint32_t *buckets = hashtab + 4 + hashtab[2] * (sizeof(size_t) / 4);
    uint32_t i = buckets[h % nbuckets];
    if (!i)
        return 0;
    uint32_t *hashval = buckets + nbuckets + (i - hashtab[1]);
    for (h |= 1;; i++) {
        uint32_t h2 = *hashval++;
        if ((h == (h2 | 1)) && (!gversym || gversym[i] >= 0) && !strcmp(s, strtab + syms[i].st_name))
            return syms + i;
        if (h2 & 1)
            break;
    }
    return NULL;
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
    if (handle->dl_info->ghashtab != NULL)
        return gnu_lookup(handle, symbol, gnu_hash(symbol));
    else
        return sysv_lookup(handle, symbol, sysv_hash(symbol));
}

static inline void *_t_dlsym(struct Elf_handle_t *handle, const char *symbol) {
    Elf_Sym *sym = t_find_sym(handle, symbol);
    return handle->mapping_info->base + sym->st_value;
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
    return _t_dlsym(handle, symbol);
}