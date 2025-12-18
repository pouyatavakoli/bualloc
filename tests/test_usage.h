#ifndef TEST_USAGE_H
#define TEST_USAGE_H

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "heap.h"
#include "heap_errors.h"

static inline void test_simple_usage(void) {
  printf("\n==== linear heap walk test start ====\n");

  HeapErrorCode rc = hinit(12 * 32);
  assert(rc == HEAP_SUCCESS);
  heap_raw_dump();
  printf("----------\n");

  printf("alloc a: \n");
  void* A = halloc(1);
  heap_raw_dump();
  printf("----------\n");

  printf("alloc b: \n");
  void* B = halloc(1);
  heap_raw_dump();
  printf("----------\n");


  printf("alloc c: \n");
  void* C = halloc(1);
  heap_raw_dump();
  printf("----------\n");

  printf("alloc d: \n");
  void* D = halloc(1);
  heap_raw_dump();
  printf("----------\n");

  heap_raw_dump();

  hfree(A);
  hfree(B);
  hfree(C);
  hfree(D);

  printf("----------\n");
  heap_raw_dump();
  void* example = halloc(32 * 12);

  printf("----------\n");
  heap_raw_dump();

  hfree(example);
  // assert(A && B && C && D);

  // printf("\nHeap dump after allocations A, B, C, D:\n");
  // heap_walk_dump();

  // hfree(B);
  // printf("\nHeap dump after freeing B:\n");
  // heap_walk_dump();

  // hfree(C);
  // printf("\nHeap dump after freeing C:\n");
  // heap_walk_dump();

  // hfree(D);
  // hfree(A);
  // printf("\nHeap dump after freeing all blocks:\n");
  // heap_walk_dump();
  // heap_raw_dump();

  // printf("\n==== linear heap walk test end ====\n");
}

#endif /* TEST_USAGE_H */
