# BuAli Sina Heap Allocator (bualloc)

**BuAli Sina Heap Allocator** (`bualloc`) is a small educational custom heap allocator in C for the Operating Systems course final project. It implements a free-list allocator with practical enhancements including low-bit flags, page-aligned `mmap` memory, and robust error handling.

## Features

* Low bits of the size field are used for **flags** (e.g., `INUSE`, `HEAP_FLAG_MARK`).
* Backed by **`mmap`** with page-aligned allocations.
* **First-fit allocation** with splitting and coalescing.
* Boundary **fences** for detecting buffer overflows.
* Pointer validation and detection of **double-free** or corrupted blocks.
* Fully testable with logging of heap state and raw memory dumps.

## Testing

All aspects of the allocator are tested, including:

* Initialization (`hinit`)
* Allocation (`halloc`)
* Free (`hfree`)
* Error handling (`HEAP_*` errors and `errno`)
* Memory fences and coalescing

Run tests:

```bash
make test
```

Run with Valgrind for memory safety:

```bash
valgrind --leak-check=full ./bin/test_runner
```


## Memory Layout

Each heap block is laid out as:

```
[Header | Fence | Payload | Fence]
```

* **Header** → metadata (`size`, `next_ptr`, `magic`)
* **Fence** → boundary pattern for overflow detection
* **Payload** → user-accessible memory

## Block Size & Low-Bit Flags

* Block size is **stored in bytes**, including the header.
* Low bits of `size` are reserved for flags because `HEADER_SIZE_BYTES` is a multiple of the size, leaving low bits always zero for real sizes.

| Flag                      | Meaning                      |
| ------------------------- | ---------------------------- |
| `HEAP_FLAG_INUSE` (`0x1`) | Block is currently allocated |
|`HEAP_FLAG_MARK` (`0x2`)   |Block is marked as reachable  |


*  `BLOCK_BYTES(p)` is used for pointer arithmetic and coalescing.
* Flags are stored in-place, no extra memory required.



## Public API

| Function                                    | Description                                                                                      |
| ------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| `HeapErrorCode hinit(size_t initial_bytes)` | Initialize heap with `initial_bytes` (0 for default). Returns `HEAP_SUCCESS` or error code.      |
| `void* halloc(size_t size)`                 | Allocate `size` bytes of payload. Returns pointer or `NULL` on failure. Sets `errno` on failure. |
| `void hfree(void* ptr)`                     | Free a previously allocated block. Safe for valid pointers; checks fences and coalesces.         |
| `HeapErrorCode heap_last_error(void)`       | Returns the last error code for diagnostic purposes.                                             |


## Diagnostics

* `heap_walk_dump()` → prints detailed block-by-block heap state, including in-use flags, size, fences
* `heap_raw_dump()` → prints raw memory content for debugging

# Considerations

## Fragmentation (Coalescing / Freeing Behavior)

When freeing a block (`bp`), the allocator inserts it into the **address-sorted free list** and coalesces adjacent free blocks.

### explanation

* `[Header|Payload]` → a block
* `p` → previous free block in list
* `bp` → block being freed
* `next` → block after `p` in free list

### Case 1: No merge

```
Memory: [p]      [bp]      [next]
Free list: p -> next
After free: p -> bp -> next
```

### Case 2: Forward merge only (`bp` + `next`)

```
Memory: [p] [bp][next]  (bp immediately before next)
Free list before: p -> next
After free (forward coalescing): p -> bp
```

Only `bp` and `next` merged.

### Case 3: Backward merge only (`p` + `bp`)

```
Memory: [p][bp]      [next]  (p immediately before bp)
Free list before: p -> next
After free (backward coalescing): p -> next
```

Only `p` + `bp` merged.

### Case 4: Full coalescing (`p` + `bp` + `next`)

```
Memory: [p][bp][next]  (all contiguous)
Free list before: p -> next
After free: p -> next->next_ptr  (p represents p+bp+next)
```

Full coalescing of both sides.


## Heap security

* Pointer validation before freeing
* Fence checking for buffer overruns
* Magic numbers to detect corruption
* Safe coalescing logic
* Double-free detection
* Integer overflow hardening
* Use-after-free poisoning

## Error Handling

* Custom `HeapErrorCode`s are returned by functions like `hinit` or logged via `heap_last_error()`.
* `halloc` sets `errno` on allocation failures.


|   Macro | Value | Description                             |
| ------: | ----: | --------------------------------------- |
|   EPERM |     1 | Operation not permitted                 |
|  ENOENT |     2 | No such file or directory               |
|   ESRCH |     3 | No such process                         |
|   EINTR |     4 | Interrupted system call                 |
|     EIO |     5 | I/O error                               |
|   ENXIO |     6 | No such device or address               |
|   E2BIG |     7 | Argument list too long                  |
| ENOEXEC |     8 | Exec format error                       |
|   EBADF |     9 | Bad file number                         |
|  ECHILD |    10 | No child processes                      |
|  EAGAIN |    11 | Try again                               |
|  ENOMEM |    12 | Out of memory                           |
|  EACCES |    13 | Permission denied                       |
|  EFAULT |    14 | Bad address                             |
| ENOTBLK |    15 | Block device required                   |
|   EBUSY |    16 | Device or resource busy                 |
|  EEXIST |    17 | File exists                             |
|   EXDEV |    18 | Cross-device link                       |
|  ENODEV |    19 | No such device                          |
| ENOTDIR |    20 | Not a directory                         |
|  EISDIR |    21 | Is a directory                          |
|  EINVAL |    22 | Invalid argument                        |
|  ENFILE |    23 | File table overflow                     |
|  EMFILE |    24 | Too many open files                     |
|  ENOTTY |    25 | Not a typewriter                        |
| ETXTBSY |    26 | Text file busy                          |
|   EFBIG |    27 | File too large                          |
|  ENOSPC |    28 | No space left on device                 |
|  ESPIPE |    29 | Illegal seek                            |
|   EROFS |    30 | Read-only file system                   |
|  EMLINK |    31 | Too many links                          |
|   EPIPE |    32 | Broken pipe                             |
|    EDOM |    33 | Math argument out of domain of function |
|  ERANGE |    34 | Math result not representable           |

# Additional Features

## Memory Pool

The memory pool subsystem provides a fast-path allocator for small, fixed-size allocations. Instead of routing every allocation through the general heap, the allocator maintains multiple preallocated pools optimized for common object sizes.

Design Overview

The implementation defines a fixed number of pools (`NUM_POOLS`), each responsible for a specific block size:
* 64 bytes
* 128 bytes
* 256 bytes
* 1024 bytes

Each pool pre-allocates a contiguous memory region using `mmap`, split into a fixed number of equally sized blocks (`POOL_BLOCKS_PER_SIZE`). These blocks are managed via a singly linked free list.

**note**: `POOL_BLOCKS_PER_SIZE` is set to 1 for easier debug, it is defined as a macro and can be easily changed to more reasonable value for production.

Unlike the general heap allocator, pool allocation does not split or coalesce blocks. Every block has the same size within a pool, which eliminates internal bookkeeping overhead during allocation and makes the allocation constant time.

### Allocation and Freeing

* `pool_alloc(size)` selects the smallest pool that can satisfy the request
* `pool_free(ptr)` validates the pointer, checks boundaries and alignment, and returns the block to the pool

### Diagnostics

Each pool tracks runtime statistics such as:

* Allocation and free request counts
* Allocation failures
* Peak usage
* Free-list consistency

These metrics can be printed using `pool_print_stats()` and are useful for debugging allocator behavior and detecting abnormal allocation patterns.


## Garbage Collection

The project implements a **conservative mark-and-sweep garbage collector** integrated with the custom heap allocator.

Two GC designs are present:

1. **Root-based recursive GC** (implemented and used)
2. **Stack-scanning GC** (educational only, documented here for reference)

Only the **root-based GC** is active in the allocator. The stack-based GC is included to explain an alternative approach and is not used at runtime.


## Garbage Collection (Implemented)

### Root-Based Mark-and-Sweep GC

The implemented garbage collector is a **root-registered, conservative mark-and-sweep GC**. Instead of scanning the program stack, it relies on explicitly registered root pointers.

### Root Registration

Roots are pointers that are known to be live entry points into the heap. They are registered explicitly:
Each root is a pointer-to-pointer, allowing the GC to always observe the current value of the root variable.

### Mark Phase

Starting from registered roots, the GC recursively traverses heap objects and marks all reachable blocks:
This allows the collector to follow object graphs and reclaim memory that is no longer reachable from any registered root.

### Sweep Phase

After marking, the heap is scanned linearly:
Unmarked blocks are freed automatically; marked blocks are retained and unmarked for the next GC cycle.

## Garbage Collection (Educational Only)

### Stack-Scanning GC (Not Used)

For educational purposes, this readme  also includes a **stack-scanning mark-and-sweep GC**. This implementation is **not used** and exists only to demonstrate how automatic root detection can be implemented in low-level systems.

### Stack Bottom Detection

The GC determines the stack bounds by parsing the Linux process memory map:

```c
FILE* f = fopen("/proc/self/maps", "r");
```

`/proc/self/maps` is a Linux virtual file that lists all memory regions of the current process. By scanning for the entry labeled `[stack]`, the GC identifies the stack’s address range.

```c
if (strstr(line, "[stack]")) {
    sscanf(line, "%lx-%lx", &start, &end);
    stack_bottom = (void*)start;
}
```

### Stack Scanning

Once initialized, the GC scans every word between the current stack pointer and the detected stack bottom:

```c
uintptr_t* p = (uintptr_t*)stack_top;
uintptr_t* bottom = (uintptr_t*)stack_bottom;

while (p != bottom) {
    mark_from_ptr((void*)(*p));
    p += direction;
}
```

Each value is treated as a potential heap pointer and marked conservatively.

### Why This GC Is Not Used

Although automatic root detection is attractive, this approach:

* Is platform-specific (Linux `/proc`)
* Can retain garbage due to false positives
* Is harder to reason about in low-level C code

For these reasons, the project uses the **explicit root-based GC** in practice, while the stack-scanning GC is documented purely for learning and comparison.


## Heap Spraying

Heap spraying is treated as a potential attack pattern.

We assume heap spraying looks like this:

* The program repeatedly allocates the same size
* It does so many times
* It happens very quickly

To detect this behavior, the allocator:

* Tracks the last N allocations
* Counts repeated allocation sizes
* Monitors allocation frequency within a short time window

If many same-sized allocations occur rapidly, the behavior is flagged as a potential heap spraying attempt.


## References

1. The C Programming Language — K&R (classic allocator concepts)
2. [glibc malloc internals: for flag usage inspiration](https://sourceware.org/glibc/wiki/MallocInternals)
3. [Memory Management with mmap](https://my.eng.utah.edu/~cs4400/malloc.pdf)
4. [glibc malloc implementation](https://github.com/lattera/glibc/blob/master/malloc/malloc.c)
5. [Writing a Simple Garbage Collector in C](https://maplant.com/2020-04-25-Writing-a-Simple-Garbage-Collector-in-C.html)
6. [Custom memory allocator in C video](https://www.youtube.com/watch?v=CulF4YQt6zA)



_Developed with ❤️ for  Operating System 4041 - [BASU]_
