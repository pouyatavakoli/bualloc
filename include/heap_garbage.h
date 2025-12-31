#ifndef HEAP_GARBAGE_H
#define HEAP_GARBAGE_H

#include "heap.h"
#include "heap_internal.h"

/* Initialize the garbage collector (currently a no-op) */
void gc_init(void);

/* Register a pointer-to-pointer as a root (e.g., &my_ptr) */
void gc_add_root(void** root);

/* Remove a previously registered root */
void gc_remove_root(void** root);

/* Run a full mark-and-sweep collection cycle */
void gc_collect(void);

#endif /* HEAP_GARBAGE_H */