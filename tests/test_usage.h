#ifndef TEST_USAGE_H
#define TEST_USAGE_H

#include <assert.h>
#include <stdio.h>

#include "heap.h"
#include "heap_errors.h"

static inline void test_simple_usage(void) {
    printf("\n==== linear heap walk test start ====\n");

    HeapErrorCode rc = hinit(32 * 1024);
    assert(rc == HEAP_SUCCESS);

    printf("\nInitial heap dump:\n");
    heap_walk_dump();

    void *A = halloc(1024);
    void *B = halloc(2048);
    void *C = halloc(1024);
    void *D = halloc(1024);
    assert(A && B && C && D);

    printf("\nHeap dump after allocations A, B, C, D:\n");
    heap_walk_dump();

    hfree(B);
    printf("\nHeap dump after freeing B:\n");
    heap_walk_dump();

    hfree(C);
    printf("\nHeap dump after freeing C:\n");
    heap_walk_dump();

    hfree(D);
    hfree(A);
    printf("\nHeap dump after freeing all blocks:\n");
    heap_walk_dump();

    printf("\n==== linear heap walk test end ====\n");
}

#endif /* TEST_USAGE_H */
