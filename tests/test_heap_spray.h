#ifndef TEST_HEAP_SPRAY_H
#define TEST_HEAP_SPRAY_H

#include "heap.h"
#include "test_utils.h"

static void test_heap_spray_detection(void) {
  LOG_TEST("Testing heap spray detection...");

  HeapErrorCode res = hinit(64 * 1024);
  assert(res == HEAP_SUCCESS);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  void* p1 = halloc(16);
  ASSERT_HEAP_SUCCESS(p1);

  void* p2 = halloc(32);
  ASSERT_HEAP_SUCCESS(p2);

  /* varied-size allocations PASS */
  for (int i = 0; i < 16; i++) {
    size_t sz = 8 * (i + 1); /* different sizes */
    void* pslow = halloc(sz);
    ASSERT_HEAP_SUCCESS(pslow);
    ASSERT_HEAP_ERROR(HEAP_SUCCESS);
  }

  /* rapid same-size allocations FAIL */
  void* p = NULL;
  for (int i = 0; i < 64; i++) {
    p = halloc(64);
    if (!p) {
      ASSERT_HEAP_ERROR(HEAP_SPRAY_ATTACK);
      ASSERT_ERRNO(EACCES);
      break;
    }
  }

  assert(p == NULL);

  DUMP_HEAP_PROMPT();
}

#endif /* TEST_HEAP_SPRAY_H */
