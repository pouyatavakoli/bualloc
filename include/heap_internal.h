#ifndef HEAP_INTERNAL_H
#define HEAP_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

/* force header alignment to worst-case primitive */
typedef long Align;

union header {
  struct {
    union header* ptr; /* next block if on free list (circular) */
    size_t size; /* size of this block in BYTES (including header). Low bits
                    used for flags */
  } s;
  Align x; /* force alignment to at least sizeof(long) */
};

typedef union header Header;

/* header size in bytes */
#define HEADER_SIZE_BYTES (sizeof(Header))

/* static assert: HEADER_SIZE_BYTES must be a power of two for low-bit flag masking */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
  _Static_assert((HEADER_SIZE_BYTES & (HEADER_SIZE_BYTES - 1)) == 0,
                 "HEADER_SIZE_BYTES must be power-of-two for flag masking");
#else
  typedef char __header_size_must_be_power_of_two[
    (HEADER_SIZE_BYTES & (HEADER_SIZE_BYTES - 1)) == 0 ? 1 : -1];
#endif


/* mask for alignment bits: HEADER_SIZE_BYTES must be a power-of-two on
 * mainstream ABIs */
#define SIZE_ALIGN_MASK ((size_t)(HEADER_SIZE_BYTES - 1))

/* Flags stored in the low bits of s.size (safe because sizes are multiples of
   HEADER_SIZE_BYTES) You can add more flags here as needed; use distinct bits
   only. */
#define HEAP_FLAG_INUSE ((size_t)1)

/* Mask to clear flags and get the pure size in bytes */
#define HEAP_SIZE_MASK (~(SIZE_ALIGN_MASK))

/* Helpers */
#define BLOCK_BYTES(p) ((p)->s.size & HEAP_SIZE_MASK)
#define IS_INUSE(p) (((p)->s.size & HEAP_FLAG_INUSE) != 0)
#define SET_INUSE(p) ((p)->s.size |= HEAP_FLAG_INUSE)
#define CLEAR_INUSE(p) ((p)->s.size &= ~HEAP_FLAG_INUSE)

#endif /* HEAP_INTERNAL_H */
