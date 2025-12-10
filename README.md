# BuAli Sina Heap Allocator (bualloc)


## Build & Run
```bash
make          # build
./bin/main    # run
````

## Tests

```bash
make test     # build and run tests
```

## Clean

```bash
make clean    # remove build artifacts
```

# Design Notes

This section explains the theoretical foundation behind the allocator and describes what the reader should expect from the implementation.

---

### Flag Bits in glibc (3 LSB of size field)

Because chunks are aligned to 8 bytes, the lower three bits of the size field are free for flags:

| Flag         | Meaning                                          |
| ------------ | ------------------------------------------------ |
| **A (0x04)** | Allocation origin (main arena vs mmap’d arena)   |
| **M (0x02)** | Entire chunk was allocated with mmap             |
| **P (0x01)** | Previous chunk is “in use” (cannot be coalesced) |

These flags drive correctness and performance.

### What We Borrow

Our system is simpler, but we adopt:

* The concept of **chunk headers**
* The habit of storing **size in units**, not bytes
* The rule that all blocks must be **aligned**
* Space reuse: once a block is freed, its user-data area may become allocator metadata

We do **not** implement glibc-level features like multiple arenas, fastbins, or mmap-chunks.

---

## 3. Alignment

To ensure correct alignment for all objects:

* Every memory block is a multiple of the **header size**.
* The header is aligned via a **union containing a long**.
* This guarantees the strictest alignment typically required on modern systems.

`include/heap_internal.h`

```c
typedef long Align;

union header {
  struct {
    union header* ptr;  // next block if on free list
    unsigned size;      // block size (in header units)
  } s;
  Align x;               // never used; forces alignment
};
```

This pattern is inherited from **K&R malloc**, ensuring:

* All headers are aligned
* All payloads are aligned
* No platform-specific alignment logic is required

The value stored in `Align x;` is never used—it exists solely to guarantee correct alignment.

---


## 4. Error Handling

The error system is clearly defined in:

```
include/heap_errors.h
```

Error codes include:

* `HEAP_INIT_FAILED`
* `HEAP_OUT_OF_MEMORY`
* `HEAP_INVALID_POINTER`
* `HEAP_CORRUPTION_DETECTED`
* etc.

Every public API returns a `HeapErrorCode`.

A human-readable description can be retrieved via:

```c
const char* heap_error_what(HeapErrorCode code);
```

---

## 6. Code Style

This project follows:

**MaJerle Coding Style Guide**
[https://github.com/MaJerle/c-code-style](https://github.com/MaJerle/c-code-style)

This ensures:

* Consistent indentation
* Clean include ordering
* Clear header separation
* Avoidance of unnecessary typedefs
* Readable and maintainable low-level C

---

## 7. References

1. **The C Programming Language – K&R**, Second Edition (classic malloc implementation)
2. **glibc Malloc Internals**
   [https://sourceware.org/glibc/wiki/MallocInternals](https://sourceware.org/glibc/wiki/MallocInternals)
3. **MaJerle C Coding Style**
   [https://github.com/MaJerle/c-code-style](https://github.com/MaJerle/c-code-style)

