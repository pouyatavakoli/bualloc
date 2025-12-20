#define _GNU_SOURCE
#include "heap_pool.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "heap_errors.h"

static const size_t pool_sizes[NUM_POOLS] = {32, 64, 128, 256};
static MemoryPool _pools[NUM_POOLS];

void init_pools(void) {
  for (int i = 0; i < NUM_POOLS; i++) {
    size_t bsize = pool_sizes[i];
    size_t total_size = bsize * POOL_BLOCKS_PER_SIZE;

    void* mem = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED) {
      heap_set_error(HEAP_OUT_OF_MEMORY, ENOMEM);
      fprintf(stderr, "pool[%d] size=%zu: %s\n", i, bsize,
              heap_error_what(heap_last_error()));

      fprintf(stderr, "pool[%d] size=%zu: out of memory (errno=%d: %s)\n", i,
              bsize, errno, strerror(errno));

      _pools[i].pool_mem = NULL;
      _pools[i].free_list = NULL;
      _pools[i].total_blocks = 0;

      _pools[i].used_blocks = 0;
      _pools[i].free_blocks = 0;
      _pools[i].peak_used = 0;
      _pools[i].alloc_requests = 0;
      _pools[i].free_requests = 0;
      _pools[i].alloc_failures = 0;

      continue;
    }

    _pools[i].block_size = bsize;
    _pools[i].pool_mem = mem;
    _pools[i].total_blocks = POOL_BLOCKS_PER_SIZE;

    PoolBlock* head = (PoolBlock*)mem;
    _pools[i].free_list = head;
    PoolBlock* current = head;

    for (size_t j = 1; j < POOL_BLOCKS_PER_SIZE; j++) {
      PoolBlock* next = (PoolBlock*)((char*)mem + j * bsize);
      current->next = next;
      current = next;
    }
    current->next = NULL;

    _pools[i].used_blocks = 0;
    _pools[i].free_blocks = POOL_BLOCKS_PER_SIZE;
    _pools[i].peak_used = 0;
    _pools[i].alloc_requests = 0;
    _pools[i].free_requests = 0;
    _pools[i].alloc_failures = 0;
  }

  heap_set_error(HEAP_SUCCESS, 0);
}

void* pool_alloc(size_t size) {
  for (int i = 0; i < NUM_POOLS; i++) {
    if (size <= _pools[i].block_size) {
      _pools[i].alloc_requests++;
      if (_pools[i].free_list == NULL) {
        _pools[i].alloc_failures++;
        heap_set_error(HEAP_OUT_OF_MEMORY, ENOMEM);
        return NULL;
      }

      PoolBlock* block = _pools[i].free_list;
      _pools[i].free_list = block->next;

      _pools[i].used_blocks++;
      _pools[i].free_blocks--;

      if (_pools[i].used_blocks > _pools[i].peak_used) {
        _pools[i].peak_used = _pools[i].used_blocks;
      }

      heap_set_error(HEAP_SUCCESS, 0);
      return (void*)((char*)block + sizeof(PoolBlock));
    }
  }

  return NULL;
}

int pool_free(void* ptr) {
  if (!ptr) {
    heap_set_error(HEAP_INVALID_POINTER, EINVAL);
    return 0;
  }

  for (int i = 0; i < NUM_POOLS; i++) {
    /* Skip pools that failed initialization */
    if (_pools[i].pool_mem == NULL || _pools[i].total_blocks == 0) {
      continue;
    }

    char* start = (char*)_pools[i].pool_mem;
    char* end = start + _pools[i].block_size * _pools[i].total_blocks;

    /* Adjust pointer back to PoolBlock header for range check */
    char* block_start = (char*)ptr - sizeof(PoolBlock);

    /* Check if block header is within this pool's memory range */
    if (block_start >= start && block_start < end) {
      size_t offset = block_start - start;
      if (offset % _pools[i].block_size != 0) {
        heap_set_error(HEAP_INVALID_POINTER, EINVAL);
        return 0;
      }

      /* Double-free detection: check if block is already in free list */
      PoolBlock* block_to_free = (PoolBlock*)block_start;
      PoolBlock* current = _pools[i].free_list;
      while (current != NULL) {
        if (current == block_to_free) {
          heap_set_error(HEAP_DOUBLE_FREE, EINVAL);
          return 0;
        }
        current = current->next;
      }

      /* Add block to free list */
      block_to_free->next = _pools[i].free_list;
      _pools[i].free_list = block_to_free;

      /* Update statistics */
      _pools[i].used_blocks--;
      _pools[i].free_blocks++;
      _pools[i].free_requests++;

      heap_set_error(HEAP_SUCCESS, 0);
      return 1;
    }
  }

  /* Pointer doesn't belong to any pool */
  return 0;
}

void pool_print_stats(void) {
  printf("\n=== Memory Pool Statistics ===\n");
  printf("Total pools: %d\n", NUM_POOLS);
  printf("Blocks per pool: %d\n", POOL_BLOCKS_PER_SIZE);
  printf("\n");

  size_t total_alloc_requests = 0;
  size_t total_free_requests = 0;
  size_t total_alloc_failures = 0;
  size_t total_used_blocks = 0;
  size_t total_free_blocks = 0;
  size_t total_capacity = 0;

  for (int i = 0; i < NUM_POOLS; i++) {
    MemoryPool* pool = &_pools[i];

    printf("Pool %d [%zu bytes per block]:\n", i, pool->block_size);

    if (pool->pool_mem == NULL) {
      printf("  Status: FAILED TO INITIALIZE\n");
      continue;
    }

    printf("  Status: %s\n", (pool->total_blocks > 0) ? "ACTIVE" : "INACTIVE");
    printf("  Memory region: %p - %p\n", pool->pool_mem,
           (char*)pool->pool_mem + pool->block_size * pool->total_blocks);
    printf("  Total blocks: %zu\n", pool->total_blocks);
    printf("  Used blocks: %zu\n", pool->used_blocks);
    printf("  Free blocks: %zu\n", pool->free_blocks);
    printf("  Peak used: %zu\n", pool->peak_used);
    printf("  Allocation requests: %zu\n", pool->alloc_requests);
    printf("  Free requests: %zu\n", pool->free_requests);
    printf("  Allocation failures: %zu\n", pool->alloc_failures);
    printf("  Free list head: %p\n", (void*)pool->free_list);

    /* Check free list integrity */
    size_t free_count = 0;
    PoolBlock* current = pool->free_list;
    while (current != NULL) {
      free_count++;
      current = current->next;
    }

    if (free_count != pool->free_blocks) {
      printf("  WARNING: Free count mismatch! List has %zu, stats show %zu\n",
             free_count, pool->free_blocks);
    }

    /* Check for consistency */
    if (pool->used_blocks + pool->free_blocks != pool->total_blocks) {
      printf("  WARNING: Block count inconsistent! used+free=%zu, total=%zu\n",
             pool->used_blocks + pool->free_blocks, pool->total_blocks);
    }

    printf("  Utilization: %.1f%%\n",
           (pool->total_blocks > 0)
               ? (100.0 * pool->used_blocks / pool->total_blocks)
               : 0.0);

    total_alloc_requests += pool->alloc_requests;
    total_free_requests += pool->free_requests;
    total_alloc_failures += pool->alloc_failures;
    total_used_blocks += pool->used_blocks;
    total_free_blocks += pool->free_blocks;
    total_capacity += pool->total_blocks;

    printf("\n");
  }

  printf("=== Summary ===\n");
  printf("Total capacity: %zu blocks\n", total_capacity);
  printf("Total used: %zu blocks\n", total_used_blocks);
  printf("Total free: %zu blocks\n", total_free_blocks);
  printf("Total allocation requests: %zu\n", total_alloc_requests);
  printf("Total free requests: %zu\n", total_free_requests);
  printf("Total allocation failures: %zu\n", total_alloc_failures);
  printf("Overall utilization: %.1f%%\n",
         (total_capacity > 0) ? (100.0 * total_used_blocks / total_capacity)
                              : 0.0);
  printf("Failure rate: %.1f%%\n",
         (total_alloc_requests > 0)
             ? (100.0 * total_alloc_failures / total_alloc_requests)
             : 0.0);
  printf("===============================\n");
}
