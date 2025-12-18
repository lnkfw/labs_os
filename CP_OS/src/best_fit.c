#include "allocator.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define ALIGNMENT 8
#define MIN_BLOCK_SIZE 32
#define ALLOCATED_FLAG 0x1

typedef struct BlockHeader {
    size_t size;
    struct BlockHeader* next;
    struct BlockHeader* prev;
} BlockHeader;

typedef struct {
    BlockHeader* free_list;
    size_t free_blocks_count;
    size_t allocated_blocks_count;
    size_t total_free_size;
} BestFitData;

static inline size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static inline int is_allocated(BlockHeader* block) {
    return block->size & ALLOCATED_FLAG;
}

static inline size_t get_block_size(BlockHeader* block) {
    return block->size & ~ALLOCATED_FLAG;
}

static inline void set_block_size(BlockHeader* block, size_t size, int allocated) {
    block->size = size | (allocated ? ALLOCATED_FLAG : 0);
}

Allocator* createBestFitAllocator(size_t size) {
    if (size < sizeof(Allocator) + sizeof(BestFitData) + MIN_BLOCK_SIZE) {
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
    allocator->alloc = bestFitAlloc;
    allocator->free = bestFitFree;
    BestFitData* data = (BestFitData*)((char*)memory + sizeof(Allocator));
    size_t data_start = sizeof(Allocator) + sizeof(BestFitData);
    size_t available_size = size - data_start;
    BlockHeader* initial_block = (BlockHeader*)((char*)memory + data_start);
    set_block_size(initial_block, available_size, 0);
    initial_block->next = NULL;
    initial_block->prev = NULL;
    data->free_list = initial_block;
    data->free_blocks_count = 1;
    data->allocated_blocks_count = 0;
    data->total_free_size = available_size;
    
    return allocator;
}

static BlockHeader* find_best_fit(BestFitData* data, size_t required_size) {
    BlockHeader* best_block = NULL;
    BlockHeader* current = data->free_list;
    
    while (current) {
        size_t block_size = get_block_size(current);
        
        if (block_size >= required_size) {
            if (!best_block || block_size < get_block_size(best_block)) {
                best_block = current;
            }
        }
        current = current->next;
    }
    
    return best_block;
}

static void remove_from_free_list(BestFitData* data, BlockHeader* block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        data->free_list = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
    data->free_blocks_count--;
    data->total_free_size -= get_block_size(block);
}

static void add_to_free_list(BestFitData* data, BlockHeader* block) {
    block->next = data->free_list;
    block->prev = NULL;
    if (data->free_list) {
        data->free_list->prev = block;
    }
    data->free_list = block;
    data->free_blocks_count++;
    data->total_free_size += get_block_size(block);
}

static void merge_with_neighbors(BestFitData* data, BlockHeader* block) {
    BlockHeader* current = data->free_list;
    
    while (current) {
        uintptr_t current_addr = (uintptr_t)current;
        uintptr_t current_end = current_addr + get_block_size(current);
        uintptr_t block_addr = (uintptr_t)block;
        uintptr_t block_end = block_addr + get_block_size(block);
        if (current_end == block_addr) {
            remove_from_free_list(data, current);
            size_t new_size = get_block_size(current) + get_block_size(block);
            set_block_size(current, new_size, 0);
            block = current;
            break;
        }
        if (block_end == current_addr) {
            remove_from_free_list(data, current);
            size_t new_size = get_block_size(block) + get_block_size(current);
            set_block_size(block, new_size, 0);
            break;
        }
        
        current = current->next;
    }
    add_to_free_list(data, block);
}

void* bestFitAlloc(Allocator* allocator, size_t size) {
    if (!allocator || size == 0) return NULL;
    
    BestFitData* data = (BestFitData*)((char*)allocator->memory + sizeof(Allocator));
    size_t required_size = align_up(size + sizeof(BlockHeader), ALIGNMENT);
    
    if (required_size < MIN_BLOCK_SIZE) {
        required_size = MIN_BLOCK_SIZE;
    }
    BlockHeader* best_block = find_best_fit(data, required_size);
    
    if (!best_block) {
        return NULL;
    }
    remove_from_free_list(data, best_block);
    size_t best_block_size = get_block_size(best_block);
    if (best_block_size >= required_size + MIN_BLOCK_SIZE) {
        BlockHeader* new_free = (BlockHeader*)((char*)best_block + required_size);
        size_t new_size = best_block_size - required_size;
        
        set_block_size(new_free, new_size, 0);
        new_free->next = NULL;
        new_free->prev = NULL;
        
        add_to_free_list(data, new_free);
    } else {
        required_size = best_block_size;
    }
    set_block_size(best_block, required_size, 1);    
    allocator->used_size += required_size;
    data->allocated_blocks_count++;    
    return (void*)((char*)best_block + sizeof(BlockHeader));
}

void bestFitFree(Allocator* allocator, void* ptr) {
    if (!allocator || !ptr) return;
    
    BestFitData* data = (BestFitData*)((char*)allocator->memory + sizeof(Allocator));
    
    BlockHeader* block = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
    
    if (!is_allocated(block)) {
        return;
    }
    
    size_t block_size = get_block_size(block);
    
    set_block_size(block, block_size, 0);
    block->next = NULL;
    block->prev = NULL;
    
    merge_with_neighbors(data, block);
    
    allocator->used_size -= block_size;
    if (data->allocated_blocks_count > 0) {
        data->allocated_blocks_count--;
    }
}