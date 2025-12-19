#ifndef TEST_HINIT_H
#define TEST_HINIT_H

#include "heap.h"
#include "test_utils.h"

static void test_hinit(void) {
  LOG_TEST("Testing heap initialization...");

  // normal init
  HeapErrorCode res = hinit(32 * 1024);
  assert(res == HEAP_SUCCESS);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  // double init
  res = hinit(64 * 1024);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);


  DUMP_HEAP_PROMPT();
}

#endif
