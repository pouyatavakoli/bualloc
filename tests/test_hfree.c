#include "heap.h"
#include "heap_errors.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    HeapErrorCode rc = hinit(64 * 1024); 
    assert(rc == HEAP_SUCCESS);
    heap_dump();

    void *block1 = halloc(1024); 
    void *block2 = halloc(2048);  
    void *block3 = halloc(4096);  
    heap_dump();

    hfree(block2);
    printf("\nHeap after freeing block2:\n");
    heap_dump();

    hfree(block1);
    printf("\nHeap after freeing block1 (should coalesce with block2):\n");
    heap_dump();

    hfree(block3);
    printf("\nHeap after freeing block3 (full coalesce expected):\n");
    heap_dump();

    return 0;
}
