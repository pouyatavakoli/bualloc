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
      return (void*)block;
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
    char* p = (char*)ptr;

    /* Check if pointer is within this pool's memory range */
    if (p >= start && p < end) {
      /* Verify pointer is aligned to block boundary */
      size_t offset = p - start;
      if (offset % _pools[i].block_size != 0) {
        heap_set_error(HEAP_INVALID_POINTER, EINVAL);
        return 0;
      }

      /* Double-free detection: check if block is already in free list */
      PoolBlock* current = _pools[i].free_list;
      while (current != NULL) {
        if (current == (PoolBlock*)ptr) {
          heap_set_error(HEAP_DOUBLE_FREE, EINVAL);
          return 0;
        }
        current = current->next;
      }

      /* Add block to free list */
      PoolBlock* block = (PoolBlock*)ptr;
      block->next = _pools[i].free_list;
      _pools[i].free_list = block;

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
