#ifndef HEAP_SPRAY_H
#define HEAP_SPRAY_H

#include <stddef.h>

#define HEAP_SPRAY_OK 0
#define HEAP_SPRAY_DETECTED 1

void heap_spray_init(void);
int heap_spray_check(size_t size);

#endif
