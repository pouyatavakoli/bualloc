#include <stdint.h>
#include <string.h>

#include "heap_garbage.h"
#include "heap.h"
#include "heap_internal.h"

#define MAX_ROOTS 1024

static void** roots[MAX_ROOTS] = {0};
static int num_roots = 0;

/* Conservative check: is this pointer a valid payload pointer inside the heap */
static int is_heap_payload_ptr(const void* ptr) {
    if (!ptr) return 0;

    void* heap_start = heap_start_addr();
    size_t heap_size = heap_total_size();

    if (!heap_start || heap_size == 0) return 0;

    uintptr_t p = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)heap_start;
    uintptr_t end = start + heap_size;

    if (p < start + HEADER_SIZE_BYTES + FENCE_SIZE || p >= end) return 0;

    if ((p & (sizeof(void*) - 1)) != 0) return 0;

    return 1;
}

/* Recursively mark reachable blocks starting from a header */
static void mark(Header* bp) {
    if (!bp || !IS_INUSE(bp) || bp->Info.magic != HEAP_MAGIC_ALLOC) return;
    if (IS_MARKED(bp)) return;

    SET_MARK(bp);

    size_t total_bytes = BLOCK_BYTES(bp);
    size_t payload_bytes = total_bytes - HEADER_SIZE_BYTES - 2 * FENCE_SIZE;
    uint8_t* payload = (uint8_t*)(bp + 1) + FENCE_SIZE;

    size_t num_words = payload_bytes / sizeof(uintptr_t);
    uintptr_t* words = (uintptr_t*)payload;

    for (size_t i = 0; i < num_words; ++i) {
        void* candidate = (void*)words[i];
        if (is_heap_payload_ptr(candidate)) {
            Header* child = (Header*)((uint8_t*)candidate - FENCE_SIZE) - 1;
            mark(child);
        }
    }
}

/* Mark phase: mark all reachable blocks starting from registered roots */
static void mark_phase(void) {
    for (int i = 0; i < num_roots; ++i) {
        void* ptr = *roots[i];
        if (is_heap_payload_ptr(ptr)) {
            Header* h = (Header*)((uint8_t*)ptr - FENCE_SIZE) - 1;
            mark(h);
        }
    }
}

/* Sweep phase: free all unmarked blocks and clear marks on marked blocks */
static void sweep_phase(void) {
    Header* bp = heap_first_block();
    while (bp) {
        Header* next = heap_next_block(bp);

        if (IS_INUSE(bp) && bp->Info.magic == HEAP_MAGIC_ALLOC && !IS_MARKED(bp)) {
            void* payload = (uint8_t*)(bp + 1) + FENCE_SIZE;
            hfree(payload);
        } else if (IS_INUSE(bp) && IS_MARKED(bp)) {
            CLEAR_MARK(bp);
        }

        bp = next;
    }
}

/* Public API */
void gc_add_root(void** root) {
    if (root && num_roots < MAX_ROOTS) {
        roots[num_roots++] = root;
    }
}

void gc_remove_root(void** root) {
    for (int i = 0; i < num_roots; ++i) {
        if (roots[i] == root) {
            roots[i] = roots[--num_roots];
            roots[num_roots] = NULL;
            break;
        }
    }
}

/* Run full mark-and-sweep */
void gc_collect(void) {
    if (heap_total_size() == 0) return; /* heap not initialized */

    /* Clear all marks first */
    Header* bp = heap_first_block();
    while (bp) {
        CLEAR_MARK(bp);
        bp = heap_next_block(bp);
    }

    mark_phase();
    sweep_phase();
}
