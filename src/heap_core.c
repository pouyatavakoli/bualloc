#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "heap.h"
#include "heap_config.h"
#include "heap_errors.h"
#include "heap_internal.h"

typedef struct {
  Header base;         /* empty list anchor */
  Header *freep;       /* start of free list (circular) */
  void *start_addr;    /* mmap start */
  size_t heap_size;    /* bytes reserved with mmap */
  int initialized;     /* boolean */
} HeapState;

/* Single global instance */
static HeapState _heap = {0};

/* Align size up to nearest page boundary */
static size_t align_to_pages(size_t size) {
  long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) page_size = 4096;
  return (size + (size_t)page_size - 1) & ~((size_t)page_size - 1);
}

HeapErrorCode hinit(size_t initial_bytes) {
  if (_heap.initialized) {
    return HEAP_SUCCESS;
  }

  /* Determine requested size */
  size_t requested = initial_bytes ? initial_bytes : DEFAULT_HEAP_SIZE;

  if (requested < MIN_HEAP_SIZE) requested = MIN_HEAP_SIZE;
  if (requested > MAX_HEAP_SIZE) requested = MAX_HEAP_SIZE;

  /* Round up to page size so we map whole pages and can trust the byte count. */
  size_t heap_size = align_to_pages(requested);
  if (heap_size < MIN_HEAP_SIZE) heap_size = align_to_pages(MIN_HEAP_SIZE);

  /* Determine the number of Header units that fit in this region */
  unsigned total_units = BYTES_TO_UNITS(heap_size);
  if (total_units < MIN_HEAP_UNITS) {
    return HEAP_INIT_FAILED;
  }

  void *mem = mmap(NULL, heap_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED) {
    return HEAP_INIT_FAILED;
  }

  /* Initialize heap control structures */
  _heap.base.s.ptr = &_heap.base;
  _heap.base.s.size = 0;
  _heap.freep = &_heap.base;
  _heap.start_addr = mem;
  _heap.heap_size = heap_size;
  _heap.initialized = 1;

  /* Create a single free block that covers the whole mapped region */
  Header *first_block = (Header *)mem;
  first_block->s.size = total_units;
  first_block->s.ptr = &_heap.base;

  /* Insert into free list */
  _heap.base.s.ptr = first_block;
  _heap.freep = &_heap.base;

  return HEAP_SUCCESS;
}



// void *halloc(size_t size) {
//   (void)size; /* placeholder */
//   return NULL; /* TODO: */
// }

// void hfree(void *ptr) {
//   (void)ptr; /* placeholder */
//   /* TODO: */
// }




void heap_dump(void) {
  if (!_heap.initialized) {
    printf("Heap not initialized\n");
    return;
  }

  printf("=== Heap Dump ===\n");
  printf("Heap start: %p\n", _heap.start_addr);
  printf("Heap size: %zu bytes\n", _heap.heap_size);
  printf("freep (anchor): %p\n", (void*)_heap.freep);

  Header *p = _heap.base.s.ptr;
  unsigned idx = 0;
  while (p != &_heap.base) {
    printf(" Free block %u: addr=%p, units=%u, bytes=%zu\n",
           idx, (void*)p, p->s.size, (size_t)p->s.size * sizeof(Header));
    p = p->s.ptr;
    if (++idx > 10000) { /* safety */
      printf(" ... free list suspiciously long, aborting dump\n");
      break;
    }
  }
  printf("=================\n");
}
