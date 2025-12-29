#ifndef HEAP_GARBAGE_H
#define HEAP_GARBAGE_H

#include <stddef.h>
#include <stdint.h>
#include "heap_internal.h"

void gc_init(void);
int gc_is_initialized(void);
int is_possible_heap_ptr(void* ptr);
void gc_mark_stack(void);
Header* heap_block_from_payload(void* ptr);
void gc_sweep(void);
void gc_collect(void);

#endif /* HEAP_GARBAGE_H */
