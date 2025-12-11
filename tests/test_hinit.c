#include "heap.h"
#include "heap_errors.h"
#include <assert.h>
#include <stddef.h>

int main(void) {
  HeapErrorCode rc = hinit(100 * 1024);
  assert(rc == HEAP_SUCCESS);
  heap_dump();
  return 0;
}
