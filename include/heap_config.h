#ifndef HEAP_CONFIG_H
#define HEAP_CONFIG_H

/* Size constraints (bytes) */
#define MIN_HEAP_SIZE     (32u)                      /* size of one block */
#define DEFAULT_HEAP_SIZE (64u * 1024u)              /* 64 KB */
#define MAX_HEAP_SIZE     (16u * 1024u * 1024u)      /* 16 MB */

/* Allocation constraints */
#define MIN_HEAP_UNITS    2u                         /* minimum block count */

#endif /* HEAP_CONFIG_H */
