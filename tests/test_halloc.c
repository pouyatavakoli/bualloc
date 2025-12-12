#include "heap.h"
#include "heap_errors.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int main(void) {
    HeapErrorCode rc = hinit(100 * 1024);
    assert(rc == HEAP_SUCCESS);

    void *p1 = halloc(1024);  
    assert(p1 != NULL);

    void *p2 = halloc(2048);  
    assert(p2 != NULL);

    void *p3 = halloc(512);   
    assert(p3 != NULL);
    heap_dump();


    return 0;
}
