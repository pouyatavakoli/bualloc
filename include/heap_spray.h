#ifndef HEAP_SPRAY_H
#define HEAP_SPRAY_H

#include <stddef.h>

/* heap spray detection status */
#define HEAP_SPRAY_OK        0
#define HEAP_SPRAY_DETECTED  1

/* initialize heap spray detection */
void heap_spray_init(void);

/* check allocation pattern for heap spray */
int heap_spray_check(size_t size);

#endif /* HEAP_SPRAY_H */
