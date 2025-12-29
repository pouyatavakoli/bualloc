#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

#include "heap_errors.h"
#include "heap_internal.h" 

HeapErrorCode hinit(size_t initial_bytes);

/* Allocation */
void* halloc(size_t size);
void hfree(void* ptr);

/* Diagnostics */
HeapErrorCode heap_last_error(void);
void heap_walk_dump(void);
void heap_raw_dump(void);

/* GC helpers / heap traversal */
void* heap_start_addr(void);
size_t heap_total_size(void);
Header* heap_first_block(void);
Header* heap_next_block(Header* current);


#endif /* HEAP_H */
