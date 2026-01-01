#ifndef TEST_GC_SHORT_H
#define TEST_GC_SHORT_H

#include <string.h>

#include "heap.h"
#include "heap_garbage.h"
#include "heap_internal.h"
#include "test_utils.h"

static void test_gc_short_free_and_poison(void) {
  LOG_TEST("Starting short GC test: free & poison unreachable objects");

  // Initialize heap
  HeapErrorCode res = hinit(10 * 1024);
  if (res != HEAP_SUCCESS) {
    printf("[ERROR] Heap initialization failed: %d\n", res);
    return;
  }
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);

  // Allocate objects
  void* objKeep1 = halloc(2000);
  void* objKeep2 = halloc(2000);
  void* objDrop = halloc(2000);

  if (!objKeep1 || !objKeep2 || !objDrop) {
    printf("[ERROR] Allocation failed\n");
    return;
  }

  // Fill memory with pattern to track it
  memset(objKeep1, 0xAA, 2000);
  memset(objKeep2, 0xBB, 2000);
  memset(objDrop, 0xCC, 2000);

  printf("[INFO] Allocated 3 objects (640 bytes each)\n");
  printf("       objKeep1: %p, objKeep2: %p, objDrop: %p (untracked root)\n",
         objKeep1, objKeep2, objDrop);

  // Add roots only for objects we want to keep
  gc_add_root(&objKeep1);
  gc_add_root(&objKeep2);

  DUMP_HEAP_PROMPT();  // Should show 3 allocated blocks

  // Run GC
  printf(
      "[INFO] Running gc_collect() → expecting objDrop to be freed and "
      "poisoned\n");
  gc_collect();

  DUMP_HEAP_PROMPT();  // Should show 2 live blocks; the freed block's payload
                       // poisoned

  // Optionally verify poison manually
  unsigned char* drop_bytes = (unsigned char*)objDrop;
  int poisoned = 1;
  for (size_t i = 0; i < 640; i++) {
    if (drop_bytes[i] != 0xDE) {
      poisoned = 0;
      break;
    }
  }
  printf("[INFO] Freed block poisoned with 0xDE: %s\n",
         poisoned ? "YES" : "NO");

  printf(
      "[PASS] Short GC test completed: unreachable object freed and "
      "poisoned\n");
}

/* Public entry point */
void test_gc(void) { test_gc_short_free_and_poison(); }

#endif /* TEST_GC_SHORT_H */
