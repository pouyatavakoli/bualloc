#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "heap.h"         
#include "heap_garbage.h"
#include "heap_internal.h"

static void* stack_bottom = NULL;
static int gc_initialized = 0;

static void detect_stack_bottom(void) {
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return;

    char line[256];

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "[stack]")) {
            unsigned long start, end;
            if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
                stack_bottom = (void*)start;
                break;
            }
        }
    }

    fclose(f);
}

void gc_init(void) {
    if (gc_initialized) return;

    detect_stack_bottom();

    if (stack_bottom != NULL) {
        gc_initialized = 1;
    }
}

int gc_is_initialized(void) {
    return gc_initialized;
}

int is_possible_heap_ptr(void* ptr) {
    if (!ptr) return 0;

    uintptr_t p = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)heap_start_addr();
    uintptr_t end   = start + heap_total_size();

    if (p < start || p >= end) return 0;          
    if (p & SIZE_ALIGN_MASK) return 0;
    return 1;
}