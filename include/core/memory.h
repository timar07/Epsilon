#ifndef EPS_MEMORY
#define EPS_MEMORY

#include "errors.h"
#include "stddef.h"

typedef void Eps_Mem;

/**
 * Allocates memory, if the allocation failed,
 * throws fatal.
 */
Eps_Mem *EpsMem_Alloc(size_t size);

Eps_Mem*
EpsMem_Calloc(size_t size, size_t n);

/**
 * Reallocates memory, as well as 'EpsMem_Alloc',
 * throws fatal if the memory reallocation failed.
 */
Eps_Mem *EpsMem_Realloc(Eps_Mem* mem, size_t size);

/**
 * Frees up provided memory.
 * Just an alias for stdlib 'free' function.
 */
void EpsMem_Free(Eps_Mem* mem);

size_t Eps_MemUsage(void);

#endif
