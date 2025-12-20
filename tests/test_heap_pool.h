#ifndef TEST_HEAP_POOL_H
#define TEST_HEAP_POOL_H

#include "heap_pool.h"
#include "test_utils.h"

static void test_heap_pool(void) {
  LOG_TEST("Testing halloc pools ...");

  hinit(0);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  printf("\nBefore allocation:\n");
  pool_print_stats();

  void* ptr = halloc(100);
  ASSERT_HEAP_SUCCESS(ptr);

  void* ptr2 = halloc(30);
  ASSERT_HEAP_SUCCESS(ptr2);

  void* ptr3 = halloc(30);
  ASSERT_HEAP_SUCCESS(ptr3);

  void* ptr4 = halloc(60);
  ASSERT_HEAP_SUCCESS(ptr4);

  void* ptr5 = halloc(60);
  ASSERT_HEAP_SUCCESS(ptr5);

  void* ptr6 = halloc(200);
  ASSERT_HEAP_SUCCESS(ptr6);

  void* ptr7 = halloc(200);
  ASSERT_HEAP_SUCCESS(ptr7);

  printf("\nAfter allocation:\n");
  printf("Allocated pointer: %p\n", ptr);
  pool_print_stats();

  /* Verify it was actually allocated from pool */
  int freed = pool_free(ptr);
  if (freed) {
    printf("[PASS] Allocation came from memory pool\n");
  } else {
    printf("[FAIL] Allocation did NOT come from memory pool\n");
    /* If not from pool, free with hfree */
    hfree(ptr);
  }

  /* hfree should be able to free from pool */
  hfree(ptr2);
  hfree(ptr3);
  hfree(ptr4);
  hfree(ptr5);
  hfree(ptr6);
  hfree(ptr7);

  /* normal alloc from heap */
  void* ptr8 = halloc(1024);
  ASSERT_HEAP_SUCCESS(ptr8);
  hfree(ptr8);

  printf("\nAfter freeing:\n");
  pool_print_stats();

  LOG_TEST("Test completed.");
}

#endif