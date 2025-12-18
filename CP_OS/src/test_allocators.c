#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MEMORY_SIZE (10 * 1024 * 1024)
#define NUM_ALLOCS 5000

static double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void benchmark_allocator(int type, const char* name) {
    Allocator* allocator = createMemoryAllocator(MEMORY_SIZE, type);
    if (!allocator) {
        printf("Ошибка создания аллокатора!\n");
        return;
    }
    
    void* ptrs[NUM_ALLOCS];
    size_t sizes[NUM_ALLOCS];
    
    srand(42);
    for (int i = 0; i < NUM_ALLOCS; i++) {
        sizes[i] = 16 + (rand() % 2048);
    }
    
    double start = get_time_sec();
    
    size_t total_requested = 0;
    int allocated = 0;
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = allocator->alloc(allocator, sizes[i]);
        if (ptrs[i]) {
            total_requested += sizes[i];
            allocated++;
        }
    }
    
    double alloc_time = get_time_sec() - start;
    
    size_t used_memory = allocator->used_size;
    double utilization = (double)total_requested / used_memory * 100.0;
    
    start = get_time_sec();
    
    for (int i = 0; i < allocated; i++) {
        if (ptrs[i]) {
            allocator->free(allocator, ptrs[i]);
        }
    }
    
    double free_time = get_time_sec() - start;
    
    printf("%s\n", name);
    printf("alloc: %.6f с\n", alloc_time);
    printf("free: %.6f с\n", free_time);
    printf("utilization: %.2f %%\n", utilization);
    printf("\n");
    
    destroyAllocator(allocator);
}

int main() {
    benchmark_allocator(0, "Best Fit");
    benchmark_allocator(1, "Power-of-Two");
    
    return 0;
}