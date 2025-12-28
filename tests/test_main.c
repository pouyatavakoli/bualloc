#include <stdio.h>
#include <stdlib.h>

#include "test_halloc.h"
#include "test_hfree.h"
#include "test_hinit.h"
#include "test_usage.h"
#include "test_heap_pool.h"
#include "test_heap_spray.h"
#include "test_gc.h"

int main() {
  int choice = 0;

  printf("Heap Allocator Test Menu:\n");
  printf("1. Test hinit\n");
  printf("2. Test halloc\n");
  printf("3. Test hfree\n");
  printf("4. Test simple usage\n");
  printf("5. Test memory pool\n");
  printf("6. Test heap spray detection\n");
  printf("7. Test garbage collection\n");
  printf("Enter test number to run: ");

  if (scanf("%d", &choice) != 1) {
    fprintf(stderr, "Invalid input.\n");
    return 1;
  }

  switch (choice) {
    case 1:
      test_hinit();
      break;
    case 2:
      test_halloc();
      break;
    case 3:
      test_hfree();
      break;
    case 4:
      test_simple_usage();
      break;
    case 5:
      test_heap_pool();
      break;
    case 6:
      test_heap_spray_detection();
      break;
    case 7:
      test_gc();
      break;
    default:
      printf("Invalid choice.\n");
      return 1;
  }

  printf("Test finished.\n");
  return 0;
}
