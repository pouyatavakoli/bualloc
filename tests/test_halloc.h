#ifndef TEST_HALLOC_H
#define TEST_HALLOC_H

#include "heap.h"
#include "test_utils.h"

static void test_halloc(void) {
  LOG_TEST("Testing halloc allocations...");

  HeapErrorCode res = hinit(32 * 1024);
  assert(res == HEAP_SUCCESS);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  void* p1 = halloc(2000);
  ASSERT_HEAP_SUCCESS(p1);

  void* p2 = halloc(2400);
  ASSERT_HEAP_SUCCESS(p2);

  // large allocation to trigger out-of-memory
  void* p3 = halloc(1024UL * 1024 * 1024);
  if (!p3) {
    ASSERT_HEAP_ERROR(HEAP_OUT_OF_MEMORY);
    ASSERT_ERRNO(ENOMEM);
  }

  DUMP_HEAP_PROMPT();
}
#endif
