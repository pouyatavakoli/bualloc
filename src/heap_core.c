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
  heap_size &= ~(HEADER_SIZE_BYTES - 1);
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
  _heap.base.Info.next_ptr = &_heap.base;
  _heap.base.Info.size = 0; /* sentinel size 0, flags cleared */
  _heap.freep = &_heap.base;
  _heap.start_addr = mem;
  _heap.heap_size = heap_size;
  _heap.initialized = 1;

  /* Create a single free block that covers the whole mapped region.
     Store block length in bytes and ensure flag bits are cleared. */
  Header* first_block = (Header*)mem;
  first_block->Info.size = heap_size & HEAP_SIZE_MASK;
  first_block->Info.next_ptr = &_heap.base;
  first_block->Info.magic = HEAP_MAGIC_FREE;

  /* Insert into free list: base -> first_block -> base (circular) */
  _heap.base.Info.next_ptr = first_block;
  _heap.freep = &_heap.base;

  return HEAP_SUCCESS;
}

static int is_valid_heap_ptr(void* ptr) {
  if (!_heap.initialized || ptr == NULL) return 0;

  char* heap_start = (char*)_heap.start_addr;
  char* heap_end = heap_start + _heap.heap_size;
  Header* bp = (Header*)ptr - 1;

  // Must be within heap memory
  if ((char*)bp < heap_start || (char*)bp >= heap_end) return 0;

  // Must be aligned to header size
  if (((uintptr_t)bp & (HEADER_SIZE_BYTES - 1)) != 0) return 0;

  // sanity check block size
  size_t size = BLOCK_BYTES(bp);
  if (size < sizeof(Header) || size > _heap.heap_size) return 0;

  if (bp->Info.magic != HEAP_MAGIC_FREE && bp->Info.magic != HEAP_MAGIC_ALLOC) {
    return 0;
  }
  return 1;
}

void* halloc(size_t size) {
  if (!_heap.initialized || size == 0) {
    errno = EINVAL;
    return NULL;
  }

  Header* prevp = _heap.freep;
  Header* p = prevp->Info.next_ptr;

  /* round payload up to HEADER_SIZE_BYTES and add header */
  size_t total_size =
      ((size + SIZE_ALIGN_MASK) & ~SIZE_ALIGN_MASK) + HEADER_SIZE_BYTES;

  do {
    if (!IS_INUSE(p) && BLOCK_BYTES(p) >= total_size) {
      size_t remaining = BLOCK_BYTES(p) - total_size;

      if (remaining >= HEADER_SIZE_BYTES * 2) {
        /* split */
        Header* tail = (Header*)((char*)p + total_size);
        tail->Info.size = remaining & HEAP_SIZE_MASK;
        tail->Info.next_ptr = p->Info.next_ptr;
        tail->Info.magic = HEAP_MAGIC_FREE;

        /* replace p with tail in free list */
        prevp->Info.next_ptr = tail;

        /* shrink allocated block */
        p->Info.size = total_size & HEAP_SIZE_MASK;
      } else {
        /* allocate whole block */
        prevp->Info.next_ptr = p->Info.next_ptr;
      }

      /* mark allocated block */
      SET_INUSE(p);
      p->Info.magic = HEAP_MAGIC_ALLOC;

      _heap.freep = prevp;
      return (void*)(p + 1);
    }

    prevp = p;
    p = p->Info.next_ptr;

  } while (p != _heap.freep);

  errno = ENOMEM;
  return NULL;
}

void hfree(void* ptr) {
  if (!_heap.initialized || ptr == NULL) return;

  if (!is_valid_heap_ptr(ptr)) {
    fprintf(stderr, "Heap free error: invalid pointer %p\n", ptr);
    return;
  }

  Header* bp = (Header*)ptr - 1;

  if (!IS_INUSE(bp)) {
    fprintf(stderr, "Double free or free of non-allocated pointer at %p\n", bp);
    return;
  }

  if (bp->Info.magic != HEAP_MAGIC_ALLOC) {
    fprintf(stderr, "Heap corruption or double free detected at %p\n", bp);
    return;
  }

  // Mark block as free
  CLEAR_INUSE(bp);
  bp->Info.magic = HEAP_MAGIC_FREE;

  Header* p = _heap.freep;

  // Find correct position in circular free list (p < bp < p->next)
  for (; !(bp > p && bp < p->Info.next_ptr); p = p->Info.next_ptr) {
    if (p >= p->Info.next_ptr && (bp > p || bp < p->Info.next_ptr)) {
      break;  // wrap around
    }
  }

  Header* next = p->Info.next_ptr;
  Header* prev = p;

  // Try to merge with upper neighbor
  if ((char*)bp + BLOCK_BYTES(bp) == (char*)next) {
    bp->Info.size = (BLOCK_BYTES(bp) + BLOCK_BYTES(next)) & HEAP_SIZE_MASK;
    bp->Info.next_ptr = next->Info.next_ptr;
    bp->Info.magic = HEAP_MAGIC_FREE;
  } else {
    bp->Info.next_ptr = next;
  }

  // Try to merge with lower neighbor
  if ((char*)prev + BLOCK_BYTES(prev) == (char*)bp) {
    prev->Info.size = (BLOCK_BYTES(prev) + BLOCK_BYTES(bp)) & HEAP_SIZE_MASK;
    prev->Info.next_ptr = bp->Info.next_ptr;
    prev->Info.magic = HEAP_MAGIC_FREE;
  } else {
    prev->Info.next_ptr = bp;
  }

  // Update free pointer
  _heap.freep = prev;
}

/* Debug: full heap dump */
void heap_free_dump(void) {
  if (!_heap.initialized) {
    printf("Heap not initialized.\n");
    return;
  }

  printf("Heap dump: start=%p, total_size=%zu bytes\n", _heap.start_addr,
         _heap.heap_size);

  Header* p = _heap.base.Info.next_ptr;
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
             (void*)p, (void*)p->Info.next_ptr);
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
          total_size, inuse ? "yes" : "no", (void*)p->Info.next_ptr);
    }

    p = p->Info.next_ptr;
  } while (p != start);

  printf("End of free list\n");
}

void heap_walk_dump(void) {
  if (!_heap.initialized) {
    printf("Heap not initialized.\n");
    return;
  }

  printf("Heap dump: start=%p, total_size=%zu bytes\n", _heap.start_addr,
         _heap.heap_size);

  char* heap_start = (char*)_heap.start_addr;
  char* heap_end = heap_start + _heap.heap_size;
  size_t block_num = 0;

  Header* p = (Header*)heap_start;

  while ((char*)p < heap_end) {
    size_t total_size = BLOCK_BYTES(p);
    int inuse = IS_INUSE(p);
    void* payload = (void*)(p + 1);
    size_t payload_size =
        (total_size >= sizeof(Header)) ? total_size - sizeof(Header) : 0;

    printf(
        "Block %zu:\n"
        "  Header: %p\n"
        "  Payload: %p\n"
        "  Total size: %zu\n"
        "  Payload size: %zu\n"
        "  In-use: %s\n"
        "  Magic: 0x%08x\n",
        block_num, (void*)p, payload, total_size, payload_size,
        inuse ? "yes" : "no", p->Info.magic);

    if (total_size == 0) {
      printf("  Warning: block with size 0 detected, stopping dump.\n");
      break;  // safety check to avoid infinite loop
    }

    p = (Header*)((char*)p + total_size);
    block_num++;
  }

  printf("End of heap\n");
}
