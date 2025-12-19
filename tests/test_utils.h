#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "heap.h"
#include "heap_errors.h"

#define LOG_TEST(msg)           \
  do {                          \
    printf("[TEST] %s\n", msg); \
  } while (0)

#define ASSERT_HEAP_SUCCESS(ptr)                                              \
  do {                                                                        \
    if (!(ptr)) {                                                             \
      fprintf(stderr,                                                         \
              "[FAIL] Allocation failed: ptr=%p errno=%d (%s) last_error=%d " \
              "(%s)\n",                                                       \
              (ptr), errno, strerror(errno), heap_last_error(),               \
              heap_error_what(heap_last_error()));                            \
      assert(ptr);                                                            \
    } else {                                                                  \
      printf("[PASS] Allocation succeeded: ptr=%p\n", (ptr));                 \
    }                                                                         \
  } while (0)

#define ASSERT_HEAP_ERROR(code)                                              \
  do {                                                                       \
    if (heap_last_error() != (code)) {                                       \
      fprintf(                                                               \
          stderr,                                                            \
          "[FAIL] Expected heap error %d (%s), got %d (%s) errno=%d (%s)\n", \
          (code), heap_error_what(code), heap_last_error(),                  \
          heap_error_what(heap_last_error()), errno, strerror(errno));       \
      assert(0);                                                             \
    } else {                                                                 \
      printf("[PASS] Expected heap error: %d (%s)\n", (code),                \
             heap_error_what(code));                                         \
    }                                                                        \
  } while (0)

#define ASSERT_ERRNO(err)                                                    \
  do {                                                                       \
    if (errno != (err)) {                                                    \
      fprintf(stderr, "[FAIL] Expected errno %d (%s), got %d (%s)\n", (err), \
              strerror(err), errno, strerror(errno));                        \
      assert(0);                                                             \
    } else {                                                                 \
      printf("[PASS] Expected errno: %d (%s)\n", (err), strerror(err));      \
    }                                                                        \
  } while (0)

#define DUMP_HEAP_PROMPT()                              \
  do {                                                  \
    int choice = 0;                                     \
    printf("Press 1 to see the dump, 0 to continue: ");     \
    if (scanf("%d", &choice) != 1) {                    \
      fprintf(stderr, "Invalid input.\n");              \
    } else if (choice == 1) {                           \
      heap_walk_dump();                                 \
    }                                                   \
                                                        \
    printf("Press 1 to see the raw dump, 0 to exit: "); \
    if (scanf("%d", &choice) != 1) {                    \
      fprintf(stderr, "Invalid input.\n");              \
    } else if (choice == 1) {                           \
      heap_raw_dump();                                  \
    }                                                   \
  } while (0)

#endif /* TEST_UTILS_H */
