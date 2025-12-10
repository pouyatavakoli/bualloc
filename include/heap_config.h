#ifndef HEAP_CONFIG_H
#define HEAP_CONFIG_H

#define DEFAULT_HEAP_SIZE (64 * 1024)      /* 64 KB */
#define MIN_HEAP_SIZE (4 * 1024)           /* 4 KB */
#define MAX_HEAP_SIZE (16UL * 1024 * 1024) /* 16 MB */

#define MIN_HEAP_UNITS 2u /* conservative minimum unit count */

#endif /* HEAP_CONFIG_H */
