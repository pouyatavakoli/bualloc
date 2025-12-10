#ifndef HEAP_INTERNAL_H
#define HEAP_INTERNAL_H

#include <stddef.h>

/* force header alignment to worst-case primitive */
typedef long Align;

union header {
  struct {
    union header* ptr; /* next block if on free list */
    unsigned size;     /* size of this block (in Header units) */
  } s;
  Align x; /* force alignment */
};

typedef union header Header;

/* convert bytes to Header units (rounded down since we will
   ensure heap size is a multiple of sizeof(Header) externally). */
#define BYTES_TO_UNITS(bytes) ((unsigned)((bytes) / sizeof(Header)))

#endif /* HEAP_INTERNAL_H */
