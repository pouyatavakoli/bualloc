#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

#include "heap_errors.h"

HeapErrorCode hinit(size_t initial_bytes);
void* halloc(size_t size);
void hfree(void* ptr);
void heap_dump();

#endif