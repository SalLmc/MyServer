#ifndef COCTX_H
#define COCTX_H

#include <stdlib.h>

typedef void *(*coctx_pfn_t)(void *s, void *s1);

struct coctx_t
{
    void *regs[14];
    size_t ss_size;
    char *ss_sp;
};

extern "C"
{
    extern void coctx_swap(coctx_t *, coctx_t *) asm("coctx_swap");
};

int coctx_init(coctx_t *ctx);
int coctx_make(coctx_t *ctx, coctx_pfn_t pfn, const void *s, const void *s1);

#endif