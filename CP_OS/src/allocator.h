#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

typedef struct Allocator Allocator;

struct Allocator {
    void* memory;
    size_t total_size;
    size_t used_size;
    void* (*alloc)(Allocator* allocator, size_t size);
    void (*free)(Allocator* allocator, void* ptr);
};

Allocator* createBestFitAllocator(size_t size);
void* bestFitAlloc(Allocator* allocator, size_t size);
void bestFitFree(Allocator* allocator, void* ptr);

Allocator* createPow2Allocator(size_t size);
void* pow2Alloc(Allocator* allocator, size_t size);
void pow2Free(Allocator* allocator, void* ptr);

Allocator* createMemoryAllocator(size_t memory_size, int type);

void destroyAllocator(Allocator* allocator);

#endif