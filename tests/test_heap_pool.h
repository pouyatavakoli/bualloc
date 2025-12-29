#ifndef TEST_HEAP_POOL_H
#define TEST_HEAP_POOL_H

#include "heap_pool.h"
#include "test_utils.h"

static void test_heap_pool(void) {
  LOG_TEST("Testing halloc pools ...");

  hinit(10*1024);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  printf("\nBefore allocation:\n");
  pool_print_stats();

  void* ptr1 = halloc(25);
  ASSERT_HEAP_SUCCESS(ptr1);
  void* ptr2 = halloc(50);
  ASSERT_HEAP_SUCCESS(ptr2);
  void* ptr3 = halloc(100);
  ASSERT_HEAP_SUCCESS(ptr3);
  void* ptr4 = halloc(800);
  ASSERT_HEAP_SUCCESS(ptr4);


  /* Verify it was actually allocated from pool */
  int freed = pool_free(ptr1);
  if (freed) {
    printf("[PASS] Allocation came from memory pool\n");
  } else {
    printf("[FAIL] Allocation did NOT come from memory pool\n");
    /* If not from pool, free with hfree */
    hfree(ptr1);
  }

  int freed2 = pool_free(ptr2);
  if (freed2) {
    printf("[PASS] Allocation came from memory pool\n");
  } else {
    printf("[FAIL] Allocation did NOT come from memory pool\n");
    /* If not from pool, free with hfree */
    hfree(ptr2);
  }
  int freed3 = pool_free(ptr3);
  if (freed3) {
    printf("[PASS] Allocation came from memory pool\n");
  } else {
    printf("[FAIL] Allocation did NOT come from memory pool\n");
    /* If not from pool, free with hfree */
    hfree(ptr3);
  }

  int freed4 = pool_free(ptr4);
  if (freed4) {
    printf("[PASS] Allocation came from memory pool\n");
  } else {
    printf("[FAIL] Allocation did NOT come from memory pool\n");
    /* If not from pool, free with hfree */
    hfree(ptr4);
  }

  void *ptr5 = halloc(1024*2);

  int freed5 = pool_free(ptr5);
  if (freed5) {
    printf("[FAIL] Allocation came from memory pool\n");
  } else {
    printf("[PASS] Allocation did NOT come from memory pool\n");
    /* If not from pool, free with hfree */
    hfree(ptr5);
  }

  pool_print_stats();

  LOG_TEST("Test completed.");
}

#endif