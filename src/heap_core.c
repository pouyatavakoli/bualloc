#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "heap.h"
#include "heap_config.h"
#include "heap_errors.h"
#include "heap_internal.h"
#include "heap_pool.h"

/* -------------------------------------------------------------------------- */
/* Heap state                                                                 */
/* -------------------------------------------------------------------------- */

typedef struct {
  Header base;
  Header* freep;
  void* start_addr;
  size_t heap_size;
  int initialized;
} HeapState;

static HeapState _heap = {0};
static HeapErrorCode _heap_last_error = HEAP_SUCCESS;


/* -------------------------------------------------------------------------- */
/* Utilities                                                                  */
/* -------------------------------------------------------------------------- */

HeapErrorCode heap_last_error(void) { return _heap_last_error; }

static void heap_set_error(HeapErrorCode code, int err) {
  _heap_last_error = code;
  errno = err;
}

static size_t align_to_pages(size_t size) {
  long ps = sysconf(_SC_PAGESIZE);
  size_t page_size = (ps > 0) ? (size_t)ps : 4096u;

  if (size > SIZE_MAX - page_size) return SIZE_MAX - (SIZE_MAX % page_size);
  return ((size + page_size - 1) / page_size) * page_size;
}

static void set_fence(uint8_t* ptr) { memset(ptr, FENCE_PATTERN, FENCE_SIZE); }

static int check_fence(const uint8_t* ptr) {
  for (size_t i = 0; i < FENCE_SIZE; i++)
    if (ptr[i] != FENCE_PATTERN) return 0;
  return 1;
}

static int is_valid_heap_ptr(void* ptr) {
  if (!_heap.initialized || !ptr) return 0;

  char* start = (char*)_heap.start_addr;
  char* end = start + _heap.heap_size;
  Header* bp = (Header*)((uint8_t*)ptr - FENCE_SIZE) - 1;

  if ((char*)bp < start || (char*)bp >= end) return 0;
  if ((uintptr_t)bp & (HEADER_SIZE_BYTES - 1)) return 0;

  size_t size = BLOCK_BYTES(bp);
  if (size < HEADER_SIZE_BYTES || size > _heap.heap_size) return 0;
  if (bp->Info.magic != HEAP_MAGIC_FREE && bp->Info.magic != HEAP_MAGIC_ALLOC)
    return 0;

  return 1;
}

static Header* find_insertion_point(Header* freed_block) {
  Header* prev = _heap.freep;

  while (1) {
    Header* next = prev->Info.next_ptr;

    if (prev == &_heap.base) {
      if (freed_block < next || next == &_heap.base) {
        return prev;
      }
    } else if (prev < freed_block && freed_block < next) {
      return prev;
    }
    /* wrapped around the circular list
     *   - freed_block > prev: should be after the last block
     *   - freed_block < next: should be before the first block
     */
    else if (prev >= next && (freed_block > prev || freed_block < next)) {
      return prev;
    }

    prev = next;
  }
}

/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

HeapErrorCode hinit(size_t initial_bytes) {
  if (_heap.initialized) return HEAP_SUCCESS;

  size_t requested = initial_bytes ? initial_bytes : DEFAULT_HEAP_SIZE;
  if (requested < MIN_HEAP_SIZE) requested = MIN_HEAP_SIZE;
  if (requested > MAX_HEAP_SIZE) requested = MAX_HEAP_SIZE;

  size_t heap_size = align_to_pages(requested);
  if (heap_size < HEADER_SIZE_BYTES ||
      heap_size / HEADER_SIZE_BYTES < MIN_HEAP_UNITS) {
    heap_set_error(HEAP_INVALID_SIZE, EINVAL);
    return HEAP_INIT_FAILED;
  }

  void* mem = mmap(NULL, heap_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED) {
    heap_set_error(HEAP_OUT_OF_MEMORY, ENOMEM);
    return HEAP_INIT_FAILED;
  }

  if ((uintptr_t)mem & (HEADER_SIZE_BYTES - 1)) {
    munmap(mem, heap_size);
    heap_set_error(HEAP_ALIGNMENT_ERROR, EFAULT);
    return HEAP_INIT_FAILED;
  }

  _heap.base.Info.next_ptr = &_heap.base;
  _heap.base.Info.size = 0;
  _heap.freep = &_heap.base;
  _heap.start_addr = mem;
  _heap.heap_size = heap_size;
  _heap.initialized = 1;

  Header* first = (Header*)mem;
  first->Info.size = heap_size & HEAP_SIZE_MASK;
  first->Info.next_ptr = &_heap.base;
  first->Info.magic = HEAP_MAGIC_FREE;

  _heap.base.Info.next_ptr = first;

  heap_set_error(HEAP_SUCCESS, 0);
  return HEAP_SUCCESS;
}

/* -------------------------------------------------------------------------- */
/* Allocation                                                                 */
/* -------------------------------------------------------------------------- */

void* halloc(size_t size) {
  if (!_heap.initialized || size == 0) {
    heap_set_error(HEAP_NOT_INITIALIZED, EINVAL);
    return NULL;
  }

  void* pool_ptr = pool_alloc(size);
  if (pool_ptr != NULL) { 
    return pool_ptr; 
  }

  if (size > SIZE_MAX - SIZE_ALIGN_MASK) {
    heap_set_error(HEAP_OVERFLOW, ENOMEM);
    return NULL;
  }

  size_t payload_size = (size + SIZE_ALIGN_MASK) & ~SIZE_ALIGN_MASK;
  if (payload_size > SIZE_MAX - HEADER_SIZE_BYTES - 2 * FENCE_SIZE) {
    heap_set_error(HEAP_OVERFLOW, ENOMEM);
    return NULL;
  }

  size_t total_size = HEADER_SIZE_BYTES + payload_size + 2 * FENCE_SIZE;
  if (total_size > _heap.heap_size) {
    heap_set_error(HEAP_OUT_OF_MEMORY, ENOMEM);
    return NULL;
  }

  Header* prev = _heap.freep;
  Header* p = prev->Info.next_ptr;

  do {
    if (!IS_INUSE(p) && BLOCK_BYTES(p) >= total_size) {
      size_t remaining = BLOCK_BYTES(p) - total_size;

      if (remaining >= HEADER_SIZE_BYTES + 2 * FENCE_SIZE) {
        Header* tail = (Header*)((char*)p + total_size);
        tail->Info.size = remaining & HEAP_SIZE_MASK;
        tail->Info.next_ptr = p->Info.next_ptr;
        tail->Info.magic = HEAP_MAGIC_FREE;
        prev->Info.next_ptr = tail;
        p->Info.size = total_size & HEAP_SIZE_MASK;
      } else {
        prev->Info.next_ptr = p->Info.next_ptr;
      }

      SET_INUSE(p);
      p->Info.magic = HEAP_MAGIC_ALLOC;

      uint8_t* pre = (uint8_t*)(p + 1);
      uint8_t* pay = pre + FENCE_SIZE;
      uint8_t* post = pay + payload_size;

      set_fence(pre);
      set_fence(post);
      memset(pay, 0, payload_size);

      _heap.freep = prev;
      heap_set_error(HEAP_SUCCESS, 0);
      return pay;
    }

    prev = p;
    p = p->Info.next_ptr;
  } while (p != _heap.freep);

  heap_set_error(HEAP_OUT_OF_MEMORY, ENOMEM);
  return NULL;
}

/* -------------------------------------------------------------------------- */
/* Free                                                                       */
/* -------------------------------------------------------------------------- */

void hfree(void* ptr) {
  if (!_heap.initialized) {
    heap_set_error(HEAP_NOT_INITIALIZED, EINVAL);
    return;
  }

  if (!ptr || !is_valid_heap_ptr(ptr)) {
    heap_set_error(HEAP_INVALID_POINTER, EINVAL);
    return;
  }

  if (pool_free(ptr)){
    return;
  }

  Header* freed_block = (Header*)((uint8_t*)ptr - FENCE_SIZE) - 1;

  if (!IS_INUSE(freed_block)) {
    heap_set_error(HEAP_DOUBLE_FREE, EINVAL);
    return;
  }

  if (freed_block->Info.magic != HEAP_MAGIC_ALLOC) {
    heap_set_error(HEAP_CORRUPTION_DETECTED, EFAULT);
    return;
  }

  /* check fence */
  size_t payload_size =
      BLOCK_BYTES(freed_block) - HEADER_SIZE_BYTES - 2 * FENCE_SIZE;
  uint8_t* pre_fence = (uint8_t*)(freed_block + 1);
  uint8_t* payload = pre_fence + FENCE_SIZE;
  uint8_t* post_fence = payload + payload_size;

  if (!check_fence(pre_fence) || !check_fence(post_fence)) {
    heap_set_error(HEAP_BOUNDARY_ERROR, EFAULT);
    return;
  }

  /* poison payload */
  memset(payload, 0xDE, payload_size);

  CLEAR_INUSE(freed_block);
  freed_block->Info.magic = HEAP_MAGIC_FREE;

  /* --- Coalescing Logic --- */

  Header* prev = find_insertion_point(freed_block);
  Header* next = prev->Info.next_ptr;

  /* merge freed_block with next if adjacent */
  if ((char*)freed_block + BLOCK_BYTES(freed_block) == (char*)next) {
    freed_block->Info.size =
        (BLOCK_BYTES(freed_block) + BLOCK_BYTES(next)) & HEAP_SIZE_MASK;
    freed_block->Info.next_ptr = next->Info.next_ptr;
  } else {
    freed_block->Info.next_ptr = next;
  }

  /* merge prev with freed_block if adjacent */
  if ((char*)prev + BLOCK_BYTES(prev) == (char*)freed_block) {
    prev->Info.size =
        (BLOCK_BYTES(prev) + BLOCK_BYTES(freed_block)) & HEAP_SIZE_MASK;
    prev->Info.next_ptr = freed_block->Info.next_ptr;
  } else {
    prev->Info.next_ptr = freed_block;
  }

  /* Update free list pointer */
  _heap.freep = prev;
  heap_set_error(HEAP_SUCCESS, 0);
}

/* -------------------------------------------------------------------------- */
/* Diagnostics                                                                */
/* -------------------------------------------------------------------------- */

void heap_walk_dump(void) {
  if (!_heap.initialized) {
    printf("heap not initialized\n");
    return;
  }

  printf("heap start=%p size=%zu\n", _heap.start_addr, _heap.heap_size);

  char* start = (char*)_heap.start_addr;
  char* end = start + _heap.heap_size;
  Header* p = (Header*)start;
  size_t idx = 0;

  while ((char*)p < end && BLOCK_BYTES(p)) {
    size_t total = BLOCK_BYTES(p);
    uint8_t* pre = (uint8_t*)(p + 1);
    uint8_t* pay = pre + FENCE_SIZE;
    size_t psz = total - HEADER_SIZE_BYTES - 2 * FENCE_SIZE;
    uint8_t* post = pay + psz;

    printf(
        "block %zu: hdr=%p payload=%p total=%zu payload=%zu inuse=%d "
        "magic=0x%08x fence(pre=%s post=%s)\n",
        idx++, (void*)p, pay, total, psz, IS_INUSE(p), p->Info.magic,
        check_fence(pre) ? "ok" : "bad", check_fence(post) ? "ok" : "bad");

    p = (Header*)((char*)p + total);
  }
}

void heap_raw_dump(void) {
  if (!_heap.initialized) return;

  uint8_t* p = (uint8_t*)_heap.start_addr;
  uint8_t* end = p + _heap.heap_size;

  size_t i = 0;
  printf("\n            ");
  for (; p < end; p++, i++) {
    if (i && i % 32 == 0) printf("\n            ");
    printf("%02x ", *p);
  }
  printf("\n");
}
