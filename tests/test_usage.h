#ifndef TEST_USAGE_H
#define TEST_USAGE_H

#include "heap.h"
#include "test_utils.h"

static void test_simple_usage(void) {
  LOG_TEST("Testing simple heap usage as malloc/free...");

  if (hinit(10 * 1024) != HEAP_SUCCESS) {
    ASSERT_HEAP_ERROR(HEAP_SUCCESS);
    return;
  }

  int* arr = (int*)halloc(500 * sizeof(int));
  ASSERT_HEAP_SUCCESS(arr);

  for (int i = 0; i < 10; i++) arr[i] = i;

  printf("Array contents: ");
  for (int i = 0; i < 10; i++) printf("%d ", arr[i]);
  printf("\n");

  DUMP_HEAP_PROMPT();

  hfree(arr);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);
  printf("freed the array...\n");
  DUMP_HEAP_PROMPT();

  // Fence corruption test
  char* corrupt = (char*)halloc(1600);
  corrupt[-1] ^= 0xFF;  // corrupt pre-fence
  hfree(corrupt);
  ASSERT_HEAP_ERROR(HEAP_BOUNDARY_ERROR);
  ASSERT_ERRNO(EFAULT);

}
#endif
