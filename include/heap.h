#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

#include "heap_errors.h"

HeapErrorCode hinit(size_t initial_bytes);

/* Allocation */
void* halloc(size_t size);
void hfree(void* ptr);

/* Diagnostics */
HeapErrorCode heap_last_error(void);
void heap_walk_dump(void);
void heap_raw_dump(void);

#endif /* HEAP_H */
