#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "heap.h"
#include "heap_errors.h"
#include "heap_config.h"


static void print_result(const char *case_name, HeapErrorCode rc) {
    printf("%-30s : %2d -> %s\n", case_name, (int)rc, heap_error_what(rc));
}

int main(void) {
    printf("=== Test: hinit() ===\n\n");

    /* Case 1: default (0) */
    printf("Case 1: initialize with 0 (use default)\n");
    HeapErrorCode r1 = hinit(0);
    print_result("hinit(0)", r1);
    if (r1 != HEAP_SUCCESS) {
        fprintf(stderr, "hinit(0) failed - aborting test\n");
        return 1;
    }

    /* call heap_dump to show current layout */
    printf("\nHeap dump after hinit(0):\n");
    heap_dump();
    printf("\n");

    /* Case 2: idempotence - calling again should succeed and not change outcome */
    printf("Case 2: call hinit(0) again (idempotence)\n");
    HeapErrorCode r1b = hinit(0);
    print_result("hinit(0) again", r1b);
    if (r1b != HEAP_SUCCESS) {
        fprintf(stderr, "second hinit(0) failed - aborting test\n");
        return 2;
    }
    printf("\n");

    /* Case 3: request smaller than MIN_HEAP_SIZE -> should clamp to MIN and succeed */
    printf("Case 3: request smaller than MIN_HEAP_SIZE\n");
    size_t small = (MIN_HEAP_SIZE > 1) ? (MIN_HEAP_SIZE - 1) : 0;
    HeapErrorCode r2 = hinit(small);
    print_result("hinit(MIN_HEAP_SIZE-1)", r2);
    if (r2 != HEAP_SUCCESS) {
        fprintf(stderr, "hinit(small) failed - aborting test\n");
        return 3;
    }
    printf("\n");

    /* Case 4: request larger than MAX_HEAP_SIZE -> should clamp to MAX and succeed */
    printf("Case 4: request larger than MAX_HEAP_SIZE\n");
    size_t huge = MAX_HEAP_SIZE + 1024;
    HeapErrorCode r3 = hinit(huge);
    print_result("hinit(MAX_HEAP_SIZE+1024)", r3);
    if (r3 != HEAP_SUCCESS) {
        fprintf(stderr, "hinit(huge) failed - aborting test\n");
        return 4;
    }
    printf("\n");

    /* Final heap dump for visual verification */
    printf("Final heap dump:\n");
    heap_dump();
    printf("\n");

    printf("All hinit checks passed (visual verification required for dumps).\n");
    return 0;
}
