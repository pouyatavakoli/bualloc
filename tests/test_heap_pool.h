#ifndef TEST_HEAP_POOL_H
#define TEST_HEAP_POOL_H

#include "heap_pool.h"
#include "test_utils.h"

static void test_heap_pool(void) {
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

  int double_free = pool_free(p1);
  assert(double_free == 0);
  ASSERT_HEAP_ERROR(HEAP_DOUBLE_FREE);

  int null_free = pool_free(NULL);
  assert(null_free == 0);
  ASSERT_HEAP_ERROR(HEAP_INVALID_POINTER);

  int invalid_free = pool_free((void*)0x12345);
  assert(invalid_free == 0);
  ASSERT_HEAP_ERROR(HEAP_INVALID_POINTER);

  void* misaligned = (char*)p2 + 1;
  int misaligned_free = pool_free(misaligned);
  assert(misaligned_free == 0);
  ASSERT_HEAP_ERROR(HEAP_ALIGNMENT_ERROR);

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

  LOG_TEST("Memory pool extended test completed.");
}

#endif
