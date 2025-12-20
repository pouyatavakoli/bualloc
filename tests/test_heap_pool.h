#ifndef TEST_HEAP_POOL_H
#define TEST_HEAP_POOL_H

#include "heap_pool.h"
#include "test_utils.h"

static void test_memory_pool_basic(void) {
  LOG_TEST("Testing memory pool basic allocation/free...");

  init_pools();
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  void* p1 = pool_alloc(32);
  ASSERT_HEAP_SUCCESS(p1);

  void* p2 = pool_alloc(48);
  ASSERT_HEAP_SUCCESS(p2);

  int freed1 = pool_free(p1);
  assert(freed1 == 1);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  int freed2 = pool_free(p2);
  assert(freed2 == 1);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  void* blocks[POOL_BLOCKS_PER_SIZE];
  for (int i = 0; i < POOL_BLOCKS_PER_SIZE; i++) {
    blocks[i] = pool_alloc(32);
    ASSERT_HEAP_SUCCESS(blocks[i]);
  }

  void* fail = pool_alloc(32);
  assert(fail == NULL);
  ASSERT_HEAP_ERROR(HEAP_OUT_OF_MEMORY);

  for (int i = 0; i < POOL_BLOCKS_PER_SIZE; i++) {
    int ok = pool_free(blocks[i]);
    assert(ok == 1);
  }

  LOG_TEST("Memory pool basic test completed.");
}

#endif 