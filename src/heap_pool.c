#define _GNU_SOURCE

#include <errno.h>
#include <stdalign.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "heap_pool.h"
#include "heap_errors.h"



/* Alignment helpers */
#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define PAYLOAD_OFFSET ALIGN_UP(sizeof(PoolBlock), alignof(max_align_t))

static const size_t pool_sizes[NUM_POOLS] = {64, 128, 256, 1024};
static MemoryPool _pools[NUM_POOLS];

/* Initialize all memory pools */
void init_pools(void) {
  for (int i = 0; i < NUM_POOLS; i++) {
    size_t bsize = pool_sizes[i];

    /* Block must fit header + aligned payload */
    if (bsize < PAYLOAD_OFFSET) {
      heap_set_error(HEAP_INVALID_SIZE, EINVAL);
      _pools[i].pool_mem = NULL;
      continue;
    }

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

    /* Build free list */
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

/* Allocate from suitable pool */
void* pool_alloc(size_t size) {
  for (int i = 0; i < NUM_POOLS; i++) {
    MemoryPool* pool = &_pools[i];

    if (!pool->pool_mem) continue;

    /* Payload size check */
    if (size > pool->block_size - PAYLOAD_OFFSET) continue;

    pool->alloc_requests++;

    if (pool->free_list == NULL) {
      pool->alloc_failures++;
      continue;
    }

    PoolBlock* block = pool->free_list;
    pool->free_list = block->next;

    pool->used_blocks++;
    pool->free_blocks--;

    if (pool->used_blocks > pool->peak_used)
      pool->peak_used = pool->used_blocks;

    heap_set_error(HEAP_SUCCESS, 0);

    /* Return aligned payload */
    return (void*)((char*)block + PAYLOAD_OFFSET);
  }

  return NULL;
}

/* Free pooled block */
int pool_free(void* ptr) {
  if (!ptr) {
    heap_set_error(HEAP_INVALID_POINTER, EINVAL);
    return 0;
  }

  for (int i = 0; i < NUM_POOLS; i++) {
    MemoryPool* pool = &_pools[i];

    if (!pool->pool_mem || pool->total_blocks == 0) continue;

    char* start = (char*)pool->pool_mem;
    char* end = start + pool->block_size * pool->total_blocks;

    /* Recover header from payload */
    char* block_start = (char*)ptr - PAYLOAD_OFFSET;

    if (block_start < start || block_start >= end) continue;

    size_t offset = (size_t)(block_start - start);

    /* Must land exactly on block boundary */
    if (offset % pool->block_size != 0) {
      heap_set_error(HEAP_INVALID_POINTER, EINVAL);
      return 0;
    }

    PoolBlock* block = (PoolBlock*)block_start;

    /* Double-free detection */
    for (PoolBlock* cur = pool->free_list; cur; cur = cur->next) {
      if (cur == block) {
        heap_set_error(HEAP_DOUBLE_FREE, EINVAL);
        return 0;
      }
    }

    block->next = pool->free_list;
    pool->free_list = block;

    pool->used_blocks--;
    pool->free_blocks++;
    pool->free_requests++;

    heap_set_error(HEAP_SUCCESS, 0);
    return 1;
  }

  heap_set_error(HEAP_INVALID_POINTER, EINVAL);
  return 0;
}


/* Print pool statistics */
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

    if (!pool->pool_mem) {
      printf("  Status: FAILED TO INITIALIZE\n");
      continue;
    }

    printf("  Status: ACTIVE\n");
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

    size_t free_count = 0;
    for (PoolBlock* cur = pool->free_list; cur; cur = cur->next) free_count++;

    if (free_count != pool->free_blocks) {
      printf("  WARNING: Free count mismatch! list=%zu stats=%zu\n", free_count,
             pool->free_blocks);
    }

    if (pool->used_blocks + pool->free_blocks != pool->total_blocks) {
      printf("  WARNING: Block count inconsistent!\n");
    }

    printf("  Utilization: %.1f%%\n",
           100.0 * pool->used_blocks / pool->total_blocks);

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
         100.0 * total_used_blocks / total_capacity);
  printf("Failure rate: %.1f%%\n",
         100.0 * total_alloc_failures / total_alloc_requests);
  printf("===============================\n");
}
