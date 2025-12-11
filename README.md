# BuAli Sina Heap Allocator (bualloc)

A small educational custom heap allocator in C for the Operating Systems course (Bu-Ali Sina).
Implements a K&R-style allocator core with the following practical enhancements:

* Block sizes stored in **bytes** (including header).
* Low bits of the size field are reserved for flags (e.g. `INUSE`) — safe because every stored size is a multiple of the header size.
* Backed by `mmap` (page-aligned mappings). We round the requested mapping up to page size and then round that value **down** to a multiple of the header size to guarantee internal arithmetic correctness.
* Simple first-fit allocation with splitting and coalescing (circular free list).

---

## Build & Run

```bash
make          # build
./bin/main    # run
```

## Tests

```bash
make test     # build and run tests
```

Run tests under valgrind for memory-safety verification:

```bash
valgrind --leak-check=full ./bin/test_runner
```

## Clean

```bash
make clean    # remove build artifacts
```

---

## Design Notes (summary)

This section explains the key design choices you need to understand when reading the code.

### Block representation & flags (byte-based sizes)

Each heap block is represented by a `Header` containing:

* a pointer used for the free list (`ptr`)
* a `size` field (type `size_t`) that stores the block size in **bytes**, including the header itself

Because we enforce that every stored block size is a multiple of `sizeof(Header)`, the lowest `log2(sizeof(Header))` bits are always zero for real sizes. Those low bits are therefore safe to use as flags (for example `INUSE`). Always use the provided macros to read/write sizes and flags.

Practical consequences:

* Use `BLOCK_BYTES(p)` (macro) to get the numeric size for arithmetic.
* Use `IS_INUSE/SET_INUSE/CLEAR_INUSE` macros when inspecting or changing allocation flags.
* All pointer arithmetic in the allocator uses `char *` (byte arithmetic).

### Why bytes instead of units

Storing sizes in bytes simplifies interaction with `mmap` and pointer arithmetic, and makes it straightforward to use low-bit flags (as glibc does) while keeping the K&R free-list and coalescing logic. It is robust and portable on normal UNIX-like platforms.

---

## Alignment and `mmap` policy

We need to satisfy two alignment constraints:

1. **Page alignment for `mmap`**: the mapping length passed to `mmap` must be a multiple of the system page size. We round the requested heap size **up** to the next page size before mapping.

2. **Header alignment for allocator arithmetic**: the allocator relies on `sizeof(Header)` alignment (the header is a `union` with a `long` member). After page-rounding we **round down** the length to a multiple of `sizeof(Header)`. This guarantees:

   * block sizes in bytes are exact multiples of `sizeof(Header)`;
   * the low bits reserved for flags are zero for real sizes (so we can safely use them);
   * pointer arithmetic for `next`/`split`/`coalesce` is always correct.

Rounding down after page rounding wastes at most `sizeof(Header) - 1` bytes and is the simplest, safe approach.

---

## Important header (canonical `include/heap_internal.h`)


```c
#ifndef HEAP_INTERNAL_H
#define HEAP_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

/* force header alignment to worst-case primitive */
typedef long Align;

union header {
  struct {
    union header* ptr; /* next block if on free list (circular) */
    size_t size;       /* size of this block in BYTES (including header). Low bits used for flags */
  } s;
  Align x; /* force alignment to at least sizeof(long) */
};

typedef union header Header;

/* header size in bytes */
#define HEADER_SIZE_BYTES (sizeof(Header))

/* mask for alignment bits: HEADER_SIZE_BYTES must be a power-of-two on mainstream ABIs */
#define SIZE_ALIGN_MASK ((size_t)(HEADER_SIZE_BYTES - 1))

/* Flags stored in the low bits of s.size (safe because sizes are multiples of HEADER_SIZE_BYTES) */
#define HEAP_FLAG_INUSE ((size_t)1)

/* Mask to clear flags and get the pure size in bytes */
#define HEAP_SIZE_MASK (~(SIZE_ALIGN_MASK))

/* Helpers */
#define BLOCK_BYTES(p) ((p)->s.size & HEAP_SIZE_MASK)
#define IS_INUSE(p) (((p)->s.size & HEAP_FLAG_INUSE) != 0)
#define SET_INUSE(p) ((p)->s.size |= HEAP_FLAG_INUSE)
#define CLEAR_INUSE(p) ((p)->s.size &= ~HEAP_FLAG_INUSE)

#endif /* HEAP_INTERNAL_H */
```

> Note: `HEADER_SIZE_BYTES` is typically 8 or 16 on mainstream systems (power-of-two). If you port to an exotic ABI where `sizeof(Header)` is not power-of-two, the low-bit flag trick is invalid.

---

## Public API behaviour

* `HeapErrorCode hinit(size_t initial_bytes)`
  Initialize heap with `initial_bytes` (0 means default). Returns `HEAP_SUCCESS` or `HEAP_INIT_FAILED` (and other error codes defined in `include/heap_errors.h`).

* `void *halloc(size_t size)`
  Allocate a payload of `size` bytes. Returns a pointer to usable payload or `NULL` on failure. On allocation failure `halloc` sets `errno = ENOMEM`.

* `void hfree(void *ptr)`
  Free the block previously returned by `halloc`. Passing `NULL` is a no-op.

Implementation details:

* The allocator uses a circular free list (K&R style) and first-fit search.
* On allocation we attempt to split big free blocks; very small remainders are not split (we require a minimum remainder of `HEADER_SIZE_BYTES * 2` to split).
* On free, coalescing is performed with adjacent free neighbors (address-ordered insertion makes this straightforward).
* All arithmetic involving sizes must use the `BLOCK_BYTES` macro or `HEAP_SIZE_MASK` to avoid mixing flags and numeric sizes.

---

## Error handling

Error codes and messages live in `include/heap_errors.h`. The public functions return `HeapErrorCode` where appropriate (e.g., `hinit`) and use conventional C conventions elsewhere:

* `halloc` returns `NULL` and sets `errno` on OOM.
* `hfree` is `void` but must behave safely for valid pointers previously returned by `halloc`.

---

## Testing & safety

Add and run unit tests (see `tests/`) for:

* `hinit(0)` success.
* 
Run tests under `valgrind` to detect illegal reads/writes and leaks.

---

## Coding style & references

This project follows the MaJerle C coding style and borrows conceptual ideas from:

1. The C Programming Language — K&R (classic allocator concepts)
2. glibc malloc internals — for flag usage inspiration (but not full behavior)
   [https://sourceware.org/glibc/wiki/MallocInternals](https://sourceware.org/glibc/wiki/MallocInternals)

See `include/heap_internal.h` above for the canonical layout and macros that the code expects.
