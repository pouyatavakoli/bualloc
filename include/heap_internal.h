#ifndef HEAP_INTERNAL_H
#define HEAP_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

/* Standard magic values for heap blocks */
#define HEAP_MAGIC_ALLOC 0xDEADBEEF
#define HEAP_MAGIC_FREE 0xBAADF00D

/* Force header alignment to worst-case primitive */
typedef long Align;

/* Header structure */
union header {
  struct {
    union header* next_ptr; /* next free block if on free list (circular) */
    size_t size;            /* block size including header + flags */
    uint32_t magic;         /* magic number for corruption check */
    uint32_t _pad;          /* pad to align struct to multiple of 8 */
  } Info;

  Align x;              /* ensure proper alignment */
  char _force_size[32]; /* force sizeof(Header) = 32 bytes (power-of-two) */
};

typedef union header Header;

/* Header size in bytes */
#define HEADER_SIZE_BYTES (sizeof(Header))

/* Static assert: HEADER_SIZE_BYTES must be a power of two for low-bit flag
 * masking */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert((HEADER_SIZE_BYTES & (HEADER_SIZE_BYTES - 1)) == 0,
               "HEADER_SIZE_BYTES must be power-of-two for flag masking");
#else
typedef char __header_size_must_be_power_of_two
    [(HEADER_SIZE_BYTES & (HEADER_SIZE_BYTES - 1)) == 0 ? 1 : -1];
#endif

/* Mask for alignment bits: HEADER_SIZE_BYTES must be a power-of-two on
 * mainstream ABIs */
#define SIZE_ALIGN_MASK ((size_t)(HEADER_SIZE_BYTES - 1))

/* Flags stored in the low bits of Info.size (safe because sizes are multiples
 * of HEADER_SIZE_BYTES) */
#define HEAP_FLAG_INUSE ((size_t)1)

/* Mask to clear flags and get the pure size in bytes */
#define HEAP_SIZE_MASK (~(SIZE_ALIGN_MASK))

/* Helper macros */
#define BLOCK_BYTES(p) ((p)->Info.size & HEAP_SIZE_MASK)
#define IS_INUSE(p) (((p)->Info.size & HEAP_FLAG_INUSE) != 0)
#define SET_INUSE(p) ((p)->Info.size |= HEAP_FLAG_INUSE)
#define CLEAR_INUSE(p) ((p)->Info.size &= ~HEAP_FLAG_INUSE)

#endif /* HEAP_INTERNAL_H */
