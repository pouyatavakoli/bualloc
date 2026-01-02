#ifndef HEAP_ERRORS_H
#define HEAP_ERRORS_H


typedef enum {
  HEAP_SUCCESS = 0,

  HEAP_INIT_FAILED,        /* heap initialization failed */
  HEAP_ALLOC_FAILED,       /* allocation failed */
  HEAP_FREE_FAILED,        /* free failed */
  HEAP_OUT_OF_MEMORY,      /* not enough memory */

  HEAP_INVALID_POINTER,    /* pointer not in heap */
  HEAP_DOUBLE_FREE,        /* double free detected */
  HEAP_INVALID_SIZE,       /* invalid size */
  HEAP_NOT_INITIALIZED,    /* heap not initialized */

  HEAP_OVERFLOW,           /* buffer overflow */
  HEAP_UNDERFLOW,          /* buffer underflow */
  HEAP_ALIGNMENT_ERROR,    /* alignment error */
  HEAP_BOUNDARY_ERROR,     /* boundary corruption */

  HEAP_CORRUPTION_DETECTED,/* heap corruption */
  HEAP_SPRAY_ATTACK,       /* heap spray detected */
  HEAP_UNKNOWN_ERROR       /* unknown error */
} HeapErrorCode;

HeapErrorCode heap_last_error(void);

void heap_set_error(HeapErrorCode code, int err);

const char* heap_error_what(HeapErrorCode code);

#endif /* HEAP_ERRORS_H */
