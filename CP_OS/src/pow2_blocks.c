#include "allocator.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MIN_BLOCK_SIZE 32
#define MAX_LEVELS 20
#define ALIGNMENT 8

typedef struct Pow2BlockHeader {
    size_t size;
    struct Pow2BlockHeader* next;
} Pow2BlockHeader;

typedef struct {
    Pow2BlockHeader* free_lists[MAX_LEVELS];
    size_t level_count;
    size_t min_size;
    size_t max_size;
    size_t allocated_blocks_count;
} Pow2Data;

static inline size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static size_t get_level(size_t size) {
    size_t level = 0;
    size_t current_size = MIN_BLOCK_SIZE;
    
    while (current_size < size && level < MAX_LEVELS - 1) {
        current_size <<= 1;
        level++;
    }
    
    return level;
}

static size_t get_size_for_level(size_t level) {
    return MIN_BLOCK_SIZE << level;
}

static void add_to_free_list(Pow2Data* data, size_t level, Pow2BlockHeader* block) {
    block->next = data->free_lists[level];
    data->free_lists[level] = block;
}

static Pow2BlockHeader* remove_from_free_list(Pow2Data* data, size_t level) {
    if (!data->free_lists[level]) {
        return NULL;
    }
    
    Pow2BlockHeader* block = data->free_lists[level];
    data->free_lists[level] = block->next;
    block->next = NULL;
    
    return block;
}

Allocator* createPow2Allocator(size_t size) {
    if (size < MIN_BLOCK_SIZE * 2) {
        return NULL;
    }
    
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        page_size = 4096;
    }
    size = align_up(size, page_size);
    
    void* memory = mmap(NULL, size,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1, 0);
    
    if (memory == MAP_FAILED) {
        return NULL;
    }
    
    Allocator* allocator = (Allocator*)memory;
    allocator->memory = memory;
    allocator->total_size = size;
    allocator->used_size = 0;
    allocator->alloc = pow2Alloc;
    allocator->free = pow2Free;
    
    Pow2Data* data = (Pow2Data*)((char*)memory + sizeof(Allocator));
    
    for (size_t i = 0; i < MAX_LEVELS; i++) {
        data->free_lists[i] = NULL;
    }
    
    data->min_size = MIN_BLOCK_SIZE;
    data->max_size = MIN_BLOCK_SIZE;
    data->level_count = 0;
    
    size_t available_size = size - sizeof(Allocator) - sizeof(Pow2Data);
    while (data->max_size < available_size && data->level_count < MAX_LEVELS) {
        data->max_size <<= 1;
        data->level_count++;
    }
    
    size_t top_level = data->level_count - 1;
    Pow2BlockHeader* initial_block = (Pow2BlockHeader*)((char*)data + sizeof(Pow2Data));
    initial_block->size = data->max_size;
    initial_block->next = NULL;
    
    data->free_lists[top_level] = initial_block;
    data->allocated_blocks_count = 0;
    
    return allocator;
}

void* pow2Alloc(Allocator* allocator, size_t size) {
    if (!allocator || size == 0) return NULL;
    
    Pow2Data* data = (Pow2Data*)((char*)allocator->memory + sizeof(Allocator));
    
    size_t required_size = align_up(size + sizeof(Pow2BlockHeader), ALIGNMENT);
    
    size_t level = get_level(required_size);
    if (level >= data->level_count) {
        return NULL;
    }
    
    size_t current_level = level;
    while (current_level < data->level_count && !data->free_lists[current_level]) {
        current_level++;
    }
    
    if (current_level >= data->level_count) {
        return NULL;
    }
    
    Pow2BlockHeader* block = NULL;
    while (current_level > level) {
        block = remove_from_free_list(data, current_level);
        if (!block) break;
        
        size_t block_size = get_size_for_level(current_level);
        size_t half_size = block_size >> 1;
        size_t smaller_level = current_level - 1;
        
        Pow2BlockHeader* left = block;
        left->size = half_size;
        
        Pow2BlockHeader* right = (Pow2BlockHeader*)((char*)block + half_size);
        right->size = half_size;
        
        add_to_free_list(data, smaller_level, right);
        add_to_free_list(data, smaller_level, left);
        
        current_level--;
    }
    
    block = remove_from_free_list(data, level);
    if (!block) {
        return NULL;
    }
    
    size_t block_size = get_size_for_level(level);
    allocator->used_size += block_size;
    data->allocated_blocks_count++;
    
    return (void*)((char*)block + sizeof(Pow2BlockHeader));
}

void pow2Free(Allocator* allocator, void* ptr) {
    if (!allocator || !ptr) return;
    
    Pow2Data* data = (Pow2Data*)((char*)allocator->memory + sizeof(Allocator));
    Pow2BlockHeader* block = (Pow2BlockHeader*)((char*)ptr - sizeof(Pow2BlockHeader));
    
    size_t block_size = block->size;
    
    size_t level = get_level(block_size);
    
    add_to_free_list(data, level, block);
    
    allocator->used_size -= block_size;
    data->allocated_blocks_count--;
}

// type: 0 - наиболее подходящий, 1 - степени двойки.
Allocator* createMemoryAllocator(size_t memory_size, int type) {
    if (type == 0) {
        return createBestFitAllocator(memory_size);
    } else {
        return createPow2Allocator(memory_size);
    }
}

void destroyAllocator(Allocator* allocator) {
    if (!allocator) return;
    
    munmap(allocator->memory, allocator->total_size);
}