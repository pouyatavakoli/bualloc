#ifndef HEAP_POOL_H
#define HEAP_POOL_H

#include <stddef.h>
#include <stdint.h>

#include "heap_errors.h"

#define NUM_POOLS 4
#define POOL_BLOCKS_PER_SIZE 128

typedef struct PoolBlock { 
  struct PoolBlock* next;
  int in_use; 
} PoolBlock; 

typedef struct {
    
    size_t block_size; 
    size_t total_blocks; 
    PoolBlock* free_list; 
    void* pool_mem;

    size_t used_blocks;
    size_t free_blocks;
    size_t peak_used;

    size_t alloc_requests;
    size_t free_requests;
    size_t alloc_failures;
} MemoryPool;


void* pool_alloc(size_t size);
HeapErrorCode init_pools(void);
int pool_free(void* ptr);


#endif /* HEAP_POOL_H */