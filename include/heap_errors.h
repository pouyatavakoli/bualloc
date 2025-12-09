#ifndef HEAP_ERRORS_H
#define HEAP_ERRORS_H

#include <stddef.h>

typedef enum {
  HEAP_SUCCESS = 0  // Operation successful
} HeapErrorCode;

const char* heap_error_what(HeapErrorCode code);

#endif  // HEAP_ERRORS_H