#include "core/memory.h"
#include "core/errors.h"
#include <stdlib.h>

Eps_Mem*
EpsMem_Alloc(size_t size)
{
    Eps_Mem* memptr = malloc(size);

    if (memptr == NULL) {
        EpsErr_Fatal("memory allocation failed");
    }

    return memptr;
}

Eps_Mem*
EpsMem_Calloc(size_t size, size_t n)
{
    Eps_Mem* memptr = calloc(size, n);

    if (memptr == NULL) {
        EpsErr_Fatal("memory allocation failed");
    }

    return memptr;
}

Eps_Mem*
EpsMem_Realloc(Eps_Mem* mem, size_t size)
{
    Eps_Mem* newmem = realloc(mem, size);

    if (newmem == NULL) {
        EpsErr_Fatal("memory reallocation failed");
    }

    return newmem;
}

void
EpsMem_Free(Eps_Mem* mem)
{
    free(mem);
}
