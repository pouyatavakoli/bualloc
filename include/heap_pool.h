#ifndef HEAP_POOL_H
#define HEAP_POOL_H

#include <stddef.h>
#include <stdint.h>

#include "heap_errors.h"

#define NUM_POOLS 4
#define POOL_BLOCKS_PER_SIZE 1

/* Single block inside a memory pool */
typedef struct PoolBlock {
  struct PoolBlock* next;
} PoolBlock;

/* Fixed-size memory pool metadata */
typedef struct {
  size_t block_size;        /* size of each block */
  size_t total_blocks;      /* total blocks in pool */
  PoolBlock* free_list;     /* free block list */
  void* pool_mem;           /* raw pool memory */

  size_t used_blocks;       /* currently used blocks */
  size_t free_blocks;       /* currently free blocks */
  size_t peak_used;         /* max used blocks */

  size_t alloc_requests;    /* allocation calls */
  size_t free_requests;     /* free calls */
  size_t alloc_failures;    /* failed allocations */
} MemoryPool;


void* pool_alloc(size_t size);
void init_pools(void);
int pool_free(void* ptr);

/* print pool statistics */
void pool_print_stats(void);

#endif /* HEAP_POOL_H */
