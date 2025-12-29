#ifndef TEST_GC_H
#define TEST_GC_H

#include "heap.h"
#include "heap_garbage.h"
#include "heap_internal.h"
#include "test_utils.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>

static void test_gc(void) {
  LOG_TEST("Testing GC...");


  HeapErrorCode res = hinit(10*1024);
  assert(res == HEAP_SUCCESS);
  ASSERT_HEAP_ERROR(HEAP_SUCCESS);
  LOG_TEST("heap init complete");

  gc_init();
  assert(gc_is_initialized());
  LOG_TEST("gc init complete");



  void* p1 = halloc(500);
  ASSERT_HEAP_SUCCESS(p1);
  memset(p1, 0xAA, 500);
  LOG_TEST("p1 halloc complete");


  void* p2 = halloc(640);
  ASSERT_HEAP_SUCCESS(p2);
  memset(p2, 0xBB, 640);
  LOG_TEST("p2 halloc complete");


  
  void* p3;
  Header* h3;

  {
    void* tmp = halloc(1280);
    ASSERT_HEAP_SUCCESS(tmp);
    LOG_TEST("tmp init complete");

    memset(tmp, 0xCC, 1280);

    /* keep raw pointer only */
    p3 = tmp;

    /* capture header BEFORE GC */
    h3 = heap_block_from_payload(p3);
    assert(IS_INUSE(h3));
    LOG_TEST("heap init complete");
  }

  gc_collect();

  assert(!IS_INUSE(h3));
  assert(h3->Info.magic == HEAP_MAGIC_FREE);

  /* payload must be poisoned */
  uint8_t* poison = (uint8_t*)p3;
  for (int i = 0; i < 1280; i++) {
     assert(poison[i] == 0xDE);
   }

  uint8_t* c1 = (uint8_t*)p1;
  uint8_t* c2 = (uint8_t*)p2;

  for (int i = 0; i < 500; i++) {
    assert(c1[i] == 0xAA);
  }

  for (int i = 0; i < 640; i++) {
    assert(c2[i] == 0xBB);
  }

//   void* p4 = halloc(128);
//   ASSERT_HEAP_SUCCESS(p4);

  /* Optional debug */
  DUMP_HEAP_PROMPT();
}

#endif /* TEST_GC_H */
