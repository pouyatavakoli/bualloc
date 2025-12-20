#include "heap_errors.h"

const char* heap_error_what(HeapErrorCode code) {
  switch (code) {
    case HEAP_SUCCESS:
      return "success";

    case HEAP_INIT_FAILED:
      return "heap initialization failed";

    case HEAP_ALLOC_FAILED:
      return "memory allocation failed";

    case HEAP_FREE_FAILED:
      return "memory free failed";

    case HEAP_OUT_OF_MEMORY:
      return "out of memory";

    case HEAP_INVALID_POINTER:
      return "invalid pointer";

    case HEAP_DOUBLE_FREE:
      return "double free detected";

    case HEAP_CORRUPTION_DETECTED:
      return "heap corruption detected";

    case HEAP_INVALID_SIZE:
      return "invalid size requested";

    case HEAP_NOT_INITIALIZED:
      return "heap not initialized";

    case HEAP_SPRAY_ATTACK:
      return "heap spray detected";

    default:
      return "unknown error";
  }
}
