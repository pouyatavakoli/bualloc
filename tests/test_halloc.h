#ifndef TEST_HALLOC
#define TEST_HALLOC
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "heap.h"
#include "heap_errors.h"

void test_halloc(void) {
  HeapErrorCode rc = hinit(100 * 1024);
  assert(rc == HEAP_SUCCESS);

  void* p1 = halloc(1024);
  assert(p1 != NULL);
  heap_walk_dump();
  printf("pointer returned to user is: %p\n", p1);

  void* p2 = halloc(1024);
  assert(p2 != NULL);
  heap_walk_dump();
  printf("pointer returned to user is: %p\n", p2);

  //   void* p2 = halloc(2048);
  //   assert(p2 != NULL);

  //   void* p3 = halloc(512);
  //   assert(p3 != NULL);

  return;
}
#endif
