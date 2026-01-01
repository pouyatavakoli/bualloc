#define _POSIX_C_SOURCE 199309L

#include "heap_spray.h"

#include <string.h>
#include <time.h>

#define MAX_EVENTS 32
#define SAME_SIZE_LIMIT 8
#define TIME_WINDOW_NS 50000000ULL /* 50 ms */

typedef struct {
  size_t size;
  unsigned long long whenHappened;
} allocation_size_time;

static allocation_size_time events[MAX_EVENTS];
static int event_count;

/* Get current monotonic time in nanoseconds */
static long long getCurrentTime(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/* Initialize heap spray detector */
void heap_spray_init(void) {
  memset(events, 0, sizeof(events));
  event_count = 0;
}

/* Check allocation pattern for heap spray */
int heap_spray_check(size_t size) {
  long long t = getCurrentTime();

  /* Shift old events left */
  if (event_count == MAX_EVENTS) {
    memmove(&events[0], &events[1], sizeof(events[0]) * (MAX_EVENTS - 1));
    event_count--;
  }

  events[event_count++] = (allocation_size_time){size, t};

  /* Count same-size allocations */
  int same_size = 0;
  for (int i = 0; i < event_count; i++) {
    if (events[i].size == size) same_size++;
  }

  /* Check time window */
  unsigned long long earliest = events[0].whenHappened;
  int rapid = (t - earliest) < TIME_WINDOW_NS;

  if (same_size >= SAME_SIZE_LIMIT && rapid) {
    return HEAP_SPRAY_DETECTED;
  }

  return HEAP_SPRAY_OK;
}
