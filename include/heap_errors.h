#ifndef HEAP_ERRORS_H
#define HEAP_ERRORS_H

#include <stddef.h>

typedef enum {
  HEAP_SUCCESS = 0,          // Operation successful
  HEAP_INIT_FAILED,          // Heap initialization failed
  HEAP_ALLOC_FAILED,         // Memory allocation failed
  HEAP_FREE_FAILED,          // Memory free failed
  HEAP_OUT_OF_MEMORY,        // Out of memory
  HEAP_INVALID_POINTER,      // Invalid pointer
  HEAP_DOUBLE_FREE,          // Attempt to free already freed memory
  HEAP_CORRUPTION_DETECTED,  // Heap corruption detected
  HEAP_INVALID_SIZE,         // Invalid size requested
  HEAP_NOT_INITIALIZED,      // Heap not initialized
  HEAP_OVERFLOW,             // Heap overflow detected
  HEAP_UNDERFLOW,            // Heap underflow detected
  HEAP_ALIGNMENT_ERROR,      // Memory alignment error
  HEAP_BOUNDARY_ERROR,       // Memory boundary violation
  HEAP_UNKNOWN_ERROR         // Unknown error
} HeapErrorCode;

const char* heap_error_what(HeapErrorCode code);

#endif  // HEAP_ERRORS_H