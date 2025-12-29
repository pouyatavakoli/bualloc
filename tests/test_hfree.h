#ifndef TEST_HFREE_H
#define TEST_HFREE_H

#include "heap.h"
#include "test_utils.h"

static void test_hfree(void) {
  LOG_TEST("Testing hfree...");

  HeapErrorCode res = hinit(10*1024);
  assert(res == HEAP_SUCCESS);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  void* p1 = halloc(1600);
  void* p2 = halloc(1600);
  void* p3 = halloc(1600);
  void* p4 = halloc(1600);
  DUMP_HEAP_PROMPT();
  
  hfree(p2);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);
  
  hfree(p1);
  hfree(p3);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  printf("adjacent blocks freed, view dump to check coalescing\n");
  //view dump to see coalescing
  DUMP_HEAP_PROMPT();

  // double free
  hfree(p1);
  ASSERT_HEAP_ERROR(HEAP_DOUBLE_FREE);
  ASSERT_ERRNO(EINVAL);

  // invalid pointer
  int dummy;
  hfree(&dummy);
  ASSERT_HEAP_ERROR(HEAP_INVALID_POINTER);
  ASSERT_ERRNO(EINVAL);

  hfree(p4);


  DUMP_HEAP_PROMPT();
}

#endif
