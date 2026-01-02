#ifndef HEAP_INTERNAL_H
#define HEAP_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

/* Block magic values */
#define HEAP_MAGIC_ALLOC 0xDEADBEEF
#define HEAP_MAGIC_FREE 0xBAADF00D

/* Payload fence */
#define FENCE_SIZE 16
#define FENCE_PATTERN 0xFE

/* Worst-case alignment */
typedef long Align;

typedef union header {
  struct {
    union header* next_ptr; /* circular free list */
    size_t size;            /* size incl. header + flags */
    uint32_t magic;         /* corruption check */
    uint32_t _pad;          /* 8-byte alignment */
  } Info;

  Align x;
  char _force_size[32]; /* force power-of-two size */
} Header;

/* Header properties */
#define HEADER_SIZE_BYTES (sizeof(Header))

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert((HEADER_SIZE_BYTES & (HEADER_SIZE_BYTES - 1)) == 0,
               "Header size must be power-of-two");
#else
typedef char __header_size_must_be_power_of_two
    [(HEADER_SIZE_BYTES & (HEADER_SIZE_BYTES - 1)) == 0 ? 1 : -1];
#endif

/* Size / flag masks */
#define SIZE_ALIGN_MASK ((size_t)(HEADER_SIZE_BYTES - 1))
#define HEAP_FLAG_INUSE ((size_t)1)
#define HEAP_SIZE_MASK (~SIZE_ALIGN_MASK)

/* GC mark bit (use highest available bit) */
#define HEAP_FLAG_MARK ((size_t)0x2)

/* Helpers */

/* Calculate total block size in bytes */
#define BLOCK_BYTES(p) ((p)->Info.size & HEAP_SIZE_MASK)

#define IS_INUSE(p) (((p)->Info.size & HEAP_FLAG_INUSE) != 0)
#define SET_INUSE(p) ((p)->Info.size |= HEAP_FLAG_INUSE)
#define CLEAR_INUSE(p) ((p)->Info.size &= ~HEAP_FLAG_INUSE)

#define IS_MARKED(p)   (((p)->Info.size & HEAP_FLAG_MARK) != 0)
#define SET_MARK(p)    ((p)->Info.size |= HEAP_FLAG_MARK)
#define CLEAR_MARK(p)  ((p)->Info.size &= ~HEAP_FLAG_MARK)


#endif /* HEAP_INTERNAL_H */
