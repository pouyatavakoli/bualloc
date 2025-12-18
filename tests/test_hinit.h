#ifndef TEST_HINIT
#define TEST_HINIT
#include <assert.h>
#include <stddef.h>

#include "heap.h"
#include "heap_errors.h"

void test_hinit(void) {
  HeapErrorCode rc = hinit(100 * 1024);
  assert(rc == HEAP_SUCCESS);
  heap_walk_dump();
  return;
}
#endif
