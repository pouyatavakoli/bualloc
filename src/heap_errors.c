#include "heap_errors.h"

const char* heap_error_what(HeapErrorCode code) {
  switch (code) {
    case HEAP_SUCCESS:
      return "Success";
    default:
      return "Unknown heap error";
  }
}