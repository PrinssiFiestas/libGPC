#include "../include/gpc/memory.h"

typedef struct GPArena
{
    GPAllocator allocator;
    size_t pos;
    size_t cap;
    unsigned char primary_mem[];
} GPArena;
