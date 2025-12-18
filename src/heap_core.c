#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

  if (size > (SIZE_MAX - page_size)) {
    return SIZE_MAX - (SIZE_MAX % page_size);
  }

  return ((size + page_size - 1) / page_size) * page_size;
}

/* Public API: initialize heap with requested bytes (0 means default) */
HeapErrorCode hinit(size_t initial_bytes) {
  if (_heap.initialized) return HEAP_SUCCESS;

  size_t requested = initial_bytes ? initial_bytes : DEFAULT_HEAP_SIZE;
  if (requested < MIN_HEAP_SIZE) requested = MIN_HEAP_SIZE;
  if (requested > MAX_HEAP_SIZE) requested = MAX_HEAP_SIZE;

  size_t heap_size = align_to_pages(requested);

  if (heap_size < HEADER_SIZE_BYTES) return HEAP_INIT_FAILED;
  heap_size &= ~(HEADER_SIZE_BYTES - 1);
  if ((heap_size / HEADER_SIZE_BYTES) < (size_t)MIN_HEAP_UNITS)
    return HEAP_INIT_FAILED;

  void* mem = mmap(NULL, heap_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED) return HEAP_INIT_FAILED;
  if (((uintptr_t)mem & (HEADER_SIZE_BYTES - 1)) != 0) {
    (void)munmap(mem, heap_size);
    return HEAP_INIT_FAILED;
  }

  _heap.base.Info.next_ptr = &_heap.base;
  _heap.base.Info.size = 0;
  _heap.freep = &_heap.base;
  _heap.start_addr = mem;
  _heap.heap_size = heap_size;
  _heap.initialized = 1;

  Header* first_block = (Header*)mem;
  first_block->Info.size = heap_size & HEAP_SIZE_MASK;
  first_block->Info.next_ptr = &_heap.base;
  first_block->Info.magic = HEAP_MAGIC_FREE;
  _heap.base.Info.next_ptr = first_block;

  return HEAP_SUCCESS;
}

/* Fence helpers */
static void set_fence(uint8_t* ptr) { memset(ptr, FENCE_PATTERN, FENCE_SIZE); }

static int check_fence(uint8_t* ptr) {
  for (size_t i = 0; i < FENCE_SIZE; i++)
    if (ptr[i] != FENCE_PATTERN) return 0;
  return 1;
}

/* Validate pointer within heap */
static int is_valid_heap_ptr(void* ptr) {
  if (!_heap.initialized || ptr == NULL) return 0;

  char* heap_start = (char*)_heap.start_addr;
  char* heap_end = heap_start + _heap.heap_size;
  Header* bp = (Header*)((uint8_t*)ptr - FENCE_SIZE) - 1;

  if ((char*)bp < heap_start || (char*)bp >= heap_end) return 0;
  if (((uintptr_t)bp & (HEADER_SIZE_BYTES - 1)) != 0) return 0;
  size_t size = BLOCK_BYTES(bp);
  if (size < sizeof(Header) || size > _heap.heap_size) return 0;
  if (bp->Info.magic != HEAP_MAGIC_FREE && bp->Info.magic != HEAP_MAGIC_ALLOC)
    return 0;
  return 1;
}

/* Allocate memory with fence around payload */
void* halloc(size_t size) {
  if (!_heap.initialized || size == 0) {
    errno = EINVAL;
    return NULL;
  }

  size_t payload_size = (size + SIZE_ALIGN_MASK) & ~SIZE_ALIGN_MASK;
  size_t total_size =
      HEADER_SIZE_BYTES + FENCE_SIZE + payload_size + FENCE_SIZE;

  Header* prevp = _heap.freep;
  Header* p = prevp->Info.next_ptr;

  do {
    if (!IS_INUSE(p) && BLOCK_BYTES(p) >= total_size) {
      size_t remaining = BLOCK_BYTES(p) - total_size;

      if (remaining >= HEADER_SIZE_BYTES + 2 * FENCE_SIZE) {
        /* split */
        Header* tail = (Header*)((char*)p + total_size);
        tail->Info.size = remaining & HEAP_SIZE_MASK;
        tail->Info.next_ptr = p->Info.next_ptr;
        tail->Info.magic = HEAP_MAGIC_FREE;
        prevp->Info.next_ptr = tail;
        p->Info.size = total_size & HEAP_SIZE_MASK;
      } else {
        prevp->Info.next_ptr = p->Info.next_ptr;
      }

      SET_INUSE(p);
      p->Info.magic = HEAP_MAGIC_ALLOC;

      /* Initialize fences */
      uint8_t* pre_fence = (uint8_t*)(p + 1);
      uint8_t* payload = pre_fence + FENCE_SIZE;
      uint8_t* post_fence = payload + payload_size;
      set_fence(pre_fence);
      set_fence(post_fence);

      _heap.freep = prevp;
      memset(payload, 0, payload_size);
      return payload;
    }

    prevp = p;
    p = p->Info.next_ptr;
  } while (p != _heap.freep);

  errno = ENOMEM;
  return NULL;
}

/* Free memory and check fences */
void hfree(void* ptr) {
  if (!_heap.initialized || ptr == NULL) return;

  if (!is_valid_heap_ptr(ptr)) {
    fprintf(stderr, "Heap free error: invalid pointer %p\n", ptr);
    return;
  }

  Header* bp = (Header*)((uint8_t*)ptr - FENCE_SIZE) - 1;

  if (!IS_INUSE(bp)) {
    fprintf(stderr, "Double free or free of non-allocated pointer at %p\n", bp);
    return;
  }

  if (bp->Info.magic != HEAP_MAGIC_ALLOC) {
    fprintf(stderr, "Heap corruption or double free detected at %p\n", bp);
    return;
  }

  size_t payload_size = BLOCK_BYTES(bp) - HEADER_SIZE_BYTES - 2 * FENCE_SIZE;
  uint8_t* pre_fence = (uint8_t*)(bp + 1);
  uint8_t* payload = pre_fence + FENCE_SIZE;
  uint8_t* post_fence = payload + payload_size;

  if (!check_fence(pre_fence) || !check_fence(post_fence)) {
    fprintf(stderr, "Heap corruption detected: fence overwritten at %p\n", bp);
    return;
  }

  /* Poison payload to detect use-after-free */
  memset(payload, 0xDE, payload_size);

  CLEAR_INUSE(bp);
  bp->Info.magic = HEAP_MAGIC_FREE;

  Header* p = _heap.freep;
  for (; !(bp > p && bp < p->Info.next_ptr); p = p->Info.next_ptr) {
    if (p >= p->Info.next_ptr && (bp > p || bp < p->Info.next_ptr)) break;
  }

  Header* next = p->Info.next_ptr;
  Header* prev = p;

  if ((char*)bp + BLOCK_BYTES(bp) == (char*)next) {
    bp->Info.size = (BLOCK_BYTES(bp) + BLOCK_BYTES(next)) & HEAP_SIZE_MASK;
    bp->Info.next_ptr = next->Info.next_ptr;
    bp->Info.magic = HEAP_MAGIC_FREE;
  } else {
    bp->Info.next_ptr = next;
  }

  if ((char*)prev + BLOCK_BYTES(prev) == (char*)bp) {
    prev->Info.size = (BLOCK_BYTES(prev) + BLOCK_BYTES(bp)) & HEAP_SIZE_MASK;
    prev->Info.next_ptr = bp->Info.next_ptr;
    prev->Info.magic = HEAP_MAGIC_FREE;
  } else {
    prev->Info.next_ptr = bp;
  }

  _heap.freep = prev;
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
    uint8_t* pre_fence = (uint8_t*)(p + 1);
    uint8_t* payload = pre_fence + FENCE_SIZE;
    size_t payload_size = total_size - HEADER_SIZE_BYTES - 2 * FENCE_SIZE;
    uint8_t* post_fence = payload + payload_size;

    printf(
        "Block %zu:\n"
        "  Header: %p\n"
        "  Payload: %p\n"
        "  Total size: %zu\n"
        "  Payload size: %zu\n"
        "  In-use: %s\n"
        "  Magic: 0x%08x\n"
        "  Fence pre: %s\n"
        "  Fence post: %s\n",
        block_num, (void*)p, payload, total_size, payload_size,
        inuse ? "yes" : "no", p->Info.magic,
        check_fence(pre_fence) ? "OK" : "CORRUPTED",
        check_fence(post_fence) ? "OK" : "CORRUPTED");

    if (total_size == 0) break;
    p = (Header*)((char*)p + total_size);
    block_num++;
  }

  printf("End of heap\n");
}

void heap_raw_dump(void) {
    if (!_heap.initialized) return;

    uint8_t* start = (uint8_t*)_heap.start_addr;
    uint8_t* end = start + _heap.heap_size;

    size_t count = 0;
    printf("\n            ");
    for (uint8_t* p = start; p < end; p++, count++) {
        if (count % 32 == 0 && count != 0) printf("\n            ");
        printf("%02x ", *p);
    }
    printf("\n");
}
