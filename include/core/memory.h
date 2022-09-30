#ifndef EPS_MEMORY
#define EPS_MEMORY

#include "errors.h"
#include "stddef.h"

typedef void Eps_Mem;

/**
 * Allocates memory, if the allocation failed,
 * throws fatal.
 */
Eps_Mem *Eps_AllocMem(size_t size);

Eps_Mem*
Eps_CallocMem(size_t size, size_t n);

/**
 * Reallocates memory, as well as 'Eps_AllocMem',
 * throws fatal if the memory reallocation failed.
 */
Eps_Mem *Eps_ReallocMem(Eps_Mem* mem, size_t size);

/**
 * Frees up provided memory.
 * Just an alias for stdlib 'free' function.
 */
void Eps_FreeMem(Eps_Mem* mem);

size_t Eps_MemUsage(void);

#endif
