#include "heap_errors.h"

const char* heap_error_what(HeapErrorCode code) {
  switch (code) {
    case HEAP_SUCCESS:
      return "Success";
    case HEAP_INIT_FAILED:
      return "Heap initialization failed";
    case HEAP_ALLOC_FAILED:
      return "Memory allocation failed";
    case HEAP_FREE_FAILED:
      return "Memory free failed";
    case HEAP_OUT_OF_MEMORY:
      return "Out of memory";
    case HEAP_INVALID_POINTER:
      return "Invalid pointer";
    case HEAP_DOUBLE_FREE:
      return "Double free detected";
    case HEAP_CORRUPTION_DETECTED:
      return "Heap corruption detected";
    case HEAP_INVALID_SIZE:
      return "Invalid size requested";
    case HEAP_NOT_INITIALIZED:
      return "Heap not initialized";
    default:
      return "Unknown error";
  }
}