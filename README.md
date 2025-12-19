# BuAli Sina Heap Allocator (bualloc)

**BuAli Sina Heap Allocator** (`bualloc`) is a small educational custom heap allocator in C for the Operating Systems course. It implements a K&R-style free-list allocator with practical enhancements including low-bit flags, page-aligned `mmap` memory, and robust error handling.

---

## Features

* Low bits of the size field are used for **flags** (e.g., `INUSE`).
* Backed by **`mmap`** with page-aligned allocations.
* **First-fit allocation** with splitting and coalescing.
* Boundary **fences** for detecting buffer overflows.
* Pointer validation and detection of **double-free** or corrupted blocks.
* Fully testable with logging of heap state and raw memory dumps.

---

## Build & Run

```bash
make          
./bin/main
```

---

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

---

## Memory Layout

Each heap block is laid out as:

```
[Header | Fence | Payload | Fence]
```

* **Header** → metadata (`size`, `next_ptr`, `magic`)
* **Fence** → boundary pattern for overflow detection
* **Payload** → user-accessible memory

---

## Block Size & Low-Bit Flags

* Block size is **stored in bytes**, including the header.
* Low bits of `size` are reserved for flags because `HEADER_SIZE_BYTES` is a multiple of the size, leaving low bits always zero for real sizes.

| Flag                      | Meaning                      |
| ------------------------- | ---------------------------- |
| `HEAP_FLAG_INUSE` (`0x1`) | Block is currently allocated |

**Macros for safe flag handling:**

```c
#define BLOCK_BYTES(p) ((p)->Info.size & HEAP_SIZE_MASK) // numeric size
#define IS_INUSE(p)   (((p)->Info.size & HEAP_FLAG_INUSE) != 0)
#define SET_INUSE(p)  ((p)->Info.size |= HEAP_FLAG_INUSE)
#define CLEAR_INUSE(p)((p)->Info.size &= ~HEAP_FLAG_INUSE)
```

*  `BLOCK_BYTES(p)` is used for pointer arithmetic and coalescing.
* Flags are stored in-place, no extra memory required.

---

## Coalescing / Freeing Behavior

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

---

## Public API

| Function                                    | Description                                                                                      |
| ------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| `HeapErrorCode hinit(size_t initial_bytes)` | Initialize heap with `initial_bytes` (0 for default). Returns `HEAP_SUCCESS` or error code.      |
| `void* halloc(size_t size)`                 | Allocate `size` bytes of payload. Returns pointer or `NULL` on failure. Sets `errno` on failure. |
| `void hfree(void* ptr)`                     | Free a previously allocated block. Safe for valid pointers; checks fences and coalesces.         |
| `HeapErrorCode heap_last_error(void)`       | Returns the last error code for diagnostic purposes.                                             |

---

## Diagnostics

* `heap_walk_dump()` → prints detailed block-by-block heap state, including in-use flags, size, fences
* `heap_raw_dump()` → prints raw memory content for debugging

---

## Error Handling

* Custom `HeapErrorCode`s are returned by functions like `hinit` or logged via `heap_last_error()`.
* `halloc` sets `errno` on allocation failures (`ENOMEM`, `EINVAL`, etc.).
* `hfree` validates pointers and fences, detecting double-free or corruption.

---

## Security Features

* Pointer validation before freeing
* Fence checking for buffer overruns
* Magic numbers to detect corruption
* Safe coalescing logic
* Double-free detection
* Integer overflow hardening
* Use-after-free poisoning

---

## References

1. The C Programming Language — K&R (classic allocator concepts)
2. [glibc malloc internals: for flag usage inspiration](https://sourceware.org/glibc/wiki/MallocInternals)
3. [Memory Management with mmap](https://my.eng.utah.edu/~cs4400/malloc.pdf)
4. [glibc malloc implementation](https://github.com/lattera/glibc/blob/master/malloc/malloc.c)

