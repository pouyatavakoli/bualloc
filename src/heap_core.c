#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "heap.h"
#include "heap_config.h"
#include "heap_errors.h"
#include "heap_internal.h"

/* Heap state */
typedef struct {
  Header base;      /* empty list anchor (sentinel) */
  Header* freep;    /* start of free list (circular) */
  void* start_addr; /* mmap start */
  size_t heap_size; /* bytes reserved with mmap */
  int initialized;  /* boolean */
} HeapState;

/* Single global instance */
static HeapState _heap = {0};

/* Align size up to nearest page boundary (safe against overflow) */
static size_t align_to_pages(size_t size) {
  long page_size_l = sysconf(_SC_PAGESIZE);
  if (page_size_l <= 0) page_size_l = 4096;
  size_t page_size = (size_t)page_size_l;

  /* Prevent overflow: if size is too large, clamp to a page-aligned maximum. */
  if (size > (SIZE_MAX - page_size)) {
    /* Return the largest page-aligned size not exceeding SIZE_MAX */
    return SIZE_MAX - (SIZE_MAX % page_size);
  }

  return ((size + page_size - 1) / page_size) * page_size;
}

/* Public API: initialize heap with requested bytes (0 means default) */
HeapErrorCode hinit(size_t initial_bytes) {
  if (_heap.initialized) {
    return HEAP_SUCCESS;
  }

  /* determine requested size */
  size_t requested = initial_bytes ? initial_bytes : DEFAULT_HEAP_SIZE;
  if (requested < MIN_HEAP_SIZE) requested = MIN_HEAP_SIZE;
  if (requested > MAX_HEAP_SIZE) requested = MAX_HEAP_SIZE;

  /* Round up to page size (mmap requires whole pages). */
  size_t heap_size = align_to_pages(requested);

  /* Ensure heap_size is a multiple of HEADER_SIZE_BYTES by rounding DOWN.
     This is simple and safe: we never claim bytes we haven't mapped.
     It wastes at most (HEADER_SIZE_BYTES - 1) bytes. */
  if (heap_size < HEADER_SIZE_BYTES) {
    return HEAP_INIT_FAILED;
  }
  heap_size -= (heap_size % HEADER_SIZE_BYTES);

  /* Ensure we have at least MIN_HEAP_UNITS header-sized blocks */
  if ((heap_size / HEADER_SIZE_BYTES) < (size_t)MIN_HEAP_UNITS) {
    return HEAP_INIT_FAILED;
  }
  void* mem = mmap(NULL, heap_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED) {
    return HEAP_INIT_FAILED;
  }
  /* Defensive check: ensure mapping pointer is aligned to HEADER_SIZE_BYTES */
  if (((uintptr_t)mem & (HEADER_SIZE_BYTES - 1)) != 0) {
    (void)munmap(mem, heap_size);
    return HEAP_INIT_FAILED;
  }

  /* Initialize heap control structures */
  _heap.base.s.ptr = &_heap.base;
  _heap.base.s.size = 0; /* sentinel size 0, flags cleared */
  _heap.freep = &_heap.base;
  _heap.start_addr = mem;
  _heap.heap_size = heap_size;
  _heap.initialized = 1;

  /* Create a single free block that covers the whole mapped region.
     Store block length in bytes and ensure flag bits are cleared. */
  Header* first_block = (Header*)mem;
  first_block->s.size = (heap_size & HEAP_SIZE_MASK);
  first_block->s.ptr = &_heap.base;

  /* Insert into free list: base -> first_block -> base (circular) */
  _heap.base.s.ptr = first_block;
  _heap.freep = &_heap.base;

  return HEAP_SUCCESS;
}

// TODO: void *halloc(size_t size)
void *halloc(size_t size) {
    if (!_heap.initialized || size == 0) {
        errno = EINVAL;
        return NULL;
    }

    Header *prevp = _heap.freep;
    Header *p = prevp->s.ptr;

    /* Align requested size to multiple of HEADER_SIZE_BYTES */
    size_t total_size = ((size + HEADER_SIZE_BYTES - 1) & ~SIZE_ALIGN_MASK) + HEADER_SIZE_BYTES;

    /* Traverse circular free list (first-fit) */
    do {
        if (!IS_INUSE(p) && BLOCK_BYTES(p) >= total_size) {
            size_t remaining = BLOCK_BYTES(p) - total_size;

            if (remaining >= HEADER_SIZE_BYTES * 2) { // split if remainder is big enough
                /* Create a new free block for the tail */
                Header *tail = (Header *)((char *)p + total_size);
                tail->s.size = remaining;
                tail->s.ptr = p->s.ptr;

                /* Update current block */
                p->s.size = total_size;
                p->s.ptr = tail;
            }

            /* Mark block as in-use */
            SET_INUSE(p);

            /* Update freep to previous block */
            _heap.freep = prevp;

            /* Return pointer to payload (after header) */
            return (void *)(p + 1);
        }

        prevp = p;
        p = p->s.ptr;
    } while (p != _heap.freep);

    /* No suitable block found */
    errno = ENOMEM;
    return NULL;
}


// TODO: void hfree(void *ptr)

#include <stdint.h>
#include <stdio.h>

#include "heap.h"
#include "heap_internal.h"

/* Debug: full heap dump */
void heap_dump(void) {
  if (!_heap.initialized) {
    printf("Heap not initialized.\n");
    return;
  }

  printf("Heap dump: start=%p, total_size=%zu bytes\n", _heap.start_addr,
         _heap.heap_size);

  Header* p = _heap.base.s.ptr;
  if (!p) {
    printf("Free list empty.\n");
    return;
  }

  printf("Free list (circular):\n");
  Header* start = p;
  int block_num = 0;

  do {
    if (p == &_heap.base) {
      /* Sentinel node */
      printf("  Block (sentinel): header=%p, size=0, inuse=no, next=%p\n",
             (void*)p, (void*)p->s.ptr);
    } else {
      size_t total_size = BLOCK_BYTES(p);    // includes header
      void* payload_start = (void*)(p + 1);  // after header
      size_t header_size = sizeof(Header);
      size_t payload_size =
          (total_size >= header_size) ? total_size - header_size : 0;
      int inuse = IS_INUSE(p);

      printf(
          "  Block %d: header=%p, payload=%p, "
          "header_size=%zu, payload_size=%zu, total_size=%zu, "
          "inuse=%s, next=%p\n",
          block_num++, (void*)p, payload_start, header_size, payload_size,
          total_size, inuse ? "yes" : "no", (void*)p->s.ptr);
    }

    p = p->s.ptr;
  } while (p != start);

  printf("End of free list\n");
}
