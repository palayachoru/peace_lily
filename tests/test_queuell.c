#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <CUnit/Basic.h>

#include <plily/queue.h>

/* -----------------------------
   Helpers
------------------------------*/

static PL_Value make_int_value(int x) {
  PL_Value v;
  v.vtype = PL_INT;
  v.as.ival = x;
  return v;
}

static PL_Value make_double_value(double x) {
  PL_Value v;
  v.vtype = PL_DOUBLE;
  v.as.dval = x;
  return v;
}

static PL_Value make_str_value(const char *s) {
  PL_Value v;
  v.vtype = PL_STR;

  size_t n = strlen(s);
  v.as.sval = (char*)malloc(n + 1);
  CU_ASSERT_PTR_NOT_NULL_FATAL(v.as.sval);
  memcpy(v.as.sval, s, n + 1);

  return v;
}

static int enqueue_int(PL_QueueLL *q, int x) {
  /* Your enqueue signature takes (const void *data) and PL_VType.
     Since pl_new_node likely copies/consumes based on vtype, we pass the address of x.
     If your pl_new_node expects allocated memory for strings only, this is correct for ints. */
  return q->enqueue(q, PL_INT, &x);
}

static int enqueue_double(PL_QueueLL *q, double x) {
  return q->enqueue(q, PL_DOUBLE, &x);
}

static int enqueue_string(PL_QueueLL *q, const char *s) {
  /* Make an owning heap string, pass pointer. The queue should store/own it
     such that dequeued PL_Value has value.as.sval that must be freed by caller
     via pl_free_value_data(). */
  return q->enqueue(q, PL_STR, s);
}

/* -----------------------------
   Logical tests
------------------------------*/

static void test_init_basic(void) {
  PL_QueueLL *q = pl_queuell_init();
  CU_ASSERT_PTR_NOT_NULL(q);
  CU_ASSERT_PTR_NOT_NULL(q->_state);

  CU_ASSERT_TRUE(q->is_empty(q));
  CU_ASSERT_EQUAL(q->length(q), 0);

  PL_Value dv = q->dequeue(q);
  PL_Value pv = q->peek(q);
  (void)dv; (void)pv;

  CU_ASSERT_TRUE(q->is_empty(q));
  CU_ASSERT_EQUAL(q->length(q), 0);

  pl_queuell_free(&q);
  CU_ASSERT_PTR_NULL(q);
}

static void test_enqueue_rejects_null_data(void) {
  PL_QueueLL *q = pl_queuell_init();
  CU_ASSERT_PTR_NOT_NULL(q);

  CU_ASSERT_FALSE(q->enqueue(q, PL_INT, NULL));
  CU_ASSERT_TRUE(q->is_empty(q));
  CU_ASSERT_EQUAL(q->length(q), 0);

  pl_queuell_free(&q);
}

static void test_fifo_ints_enqueue_dequeue(void) {
  PL_QueueLL *q = pl_queuell_init();
  CU_ASSERT_PTR_NOT_NULL(q);

  for (int i = 0; i < 5; i++) {
    CU_ASSERT_TRUE(enqueue_int(q, i * 10));
    CU_ASSERT_EQUAL(q->length(q), i + 1);
    CU_ASSERT_FALSE(q->is_empty(q));
  }

  PL_Value p = q->peek(q);
  CU_ASSERT_EQUAL(p.vtype, PL_INT);
  CU_ASSERT_EQUAL(p.as.ival, 0);
  CU_ASSERT_EQUAL(q->length(q), 5);

  for (int i = 0; i < 5; i++) {
    PL_Value v = q->dequeue(q);
    CU_ASSERT_EQUAL(v.vtype, PL_INT);
    CU_ASSERT_EQUAL(v.as.ival, i * 10);
    /* No pl_free_value_data for ints */
  }

  CU_ASSERT_TRUE(q->is_empty(q));
  CU_ASSERT_EQUAL(q->length(q), 0);

  pl_queuell_free(&q);
}

static void test_peek_does_not_modify_queue(void) {
  PL_QueueLL *q = pl_queuell_init();
  CU_ASSERT_PTR_NOT_NULL(q);

  CU_ASSERT_TRUE(enqueue_int(q, 111));
  CU_ASSERT_TRUE(enqueue_int(q, 222));
  CU_ASSERT_EQUAL(q->length(q), 2);

  PL_Value p1 = q->peek(q);
  PL_Value p2 = q->peek(q);

  CU_ASSERT_EQUAL(p1.vtype, PL_INT);
  CU_ASSERT_EQUAL(p2.vtype, PL_INT);
  CU_ASSERT_EQUAL(p1.as.ival, 111);
  CU_ASSERT_EQUAL(p2.as.ival, 111);

  CU_ASSERT_EQUAL(q->length(q), 2);

  PL_Value d = q->dequeue(q);
  CU_ASSERT_EQUAL(d.vtype, PL_INT);
  CU_ASSERT_EQUAL(d.as.ival, 111);

  CU_ASSERT_EQUAL(q->length(q), 1);
  pl_queuell_free(&q);
}

static void test_dequeue_empty_returns_no_crash(void) {
  PL_QueueLL *q = pl_queuell_init();
  CU_ASSERT_PTR_NOT_NULL(q);

  CU_ASSERT_TRUE(q->is_empty(q));
  PL_Value v = q->dequeue(q); /* should not crash */
  (void)v;

  CU_ASSERT_TRUE(q->is_empty(q));
  CU_ASSERT_EQUAL(q->length(q), 0);

  pl_queuell_free(&q);
}

static void test_string_fifo_and_free_value_data(void) {
  PL_QueueLL *q = pl_queuell_init();
  CU_ASSERT_PTR_NOT_NULL(q);

  CU_ASSERT_TRUE(enqueue_string(q, "alpha"));
  CU_ASSERT_TRUE(enqueue_string(q, "beta"));
  CU_ASSERT_TRUE(enqueue_string(q, "gamma"));
  CU_ASSERT_EQUAL(q->length(q), 3);

  PL_Value p = q->peek(q);
  CU_ASSERT_EQUAL(p.vtype, PL_STR);
  CU_ASSERT_PTR_NOT_NULL(p.as.sval);
  CU_ASSERT_STRING_EQUAL(p.as.sval, "alpha");

  PL_Value d1 = q->dequeue(q);
  CU_ASSERT_EQUAL(d1.vtype, PL_STR);
  CU_ASSERT_STRING_EQUAL(d1.as.sval, "alpha");
  pl_free_value_data(d1);

  PL_Value d2 = q->dequeue(q);
  CU_ASSERT_EQUAL(d2.vtype, PL_STR);
  CU_ASSERT_STRING_EQUAL(d2.as.sval, "beta");
  pl_free_value_data(d2);

  PL_Value d3 = q->dequeue(q);
  CU_ASSERT_EQUAL(d3.vtype, PL_STR);
  CU_ASSERT_STRING_EQUAL(d3.as.sval, "gamma");
  pl_free_value_data(d3);

  CU_ASSERT_TRUE(q->is_empty(q));
  CU_ASSERT_EQUAL(q->length(q), 0);

  pl_queuell_free(&q);
}

/* -----------------------------
   Edge case tests
------------------------------*/

static void test_mixed_types_int_double_string_fifo(void) {
  PL_QueueLL *q = pl_queuell_init();
  CU_ASSERT_PTR_NOT_NULL(q);

  CU_ASSERT_TRUE(enqueue_int(q, 42));
  CU_ASSERT_TRUE(enqueue_double(q, 3.5));
  CU_ASSERT_TRUE(enqueue_string(q, "hello"));

  CU_ASSERT_EQUAL(q->length(q), 3);

  PL_Value v1 = q->dequeue(q);
  CU_ASSERT_EQUAL(v1.vtype, PL_INT);
  CU_ASSERT_EQUAL(v1.as.ival, 42);

  PL_Value v2 = q->dequeue(q);
  CU_ASSERT_EQUAL(v2.vtype, PL_DOUBLE);
  CU_ASSERT_TRUE(v2.as.dval > 3.49 && v2.as.dval < 3.51);

  PL_Value v3 = q->dequeue(q);
  CU_ASSERT_EQUAL(v3.vtype, PL_STR);
  CU_ASSERT_STRING_EQUAL(v3.as.sval, "hello");
  pl_free_value_data(v3);

  CU_ASSERT_TRUE(q->is_empty(q));
  CU_ASSERT_EQUAL(q->length(q), 0);

  pl_queuell_free(&q);
}

static void test_free_queue_with_pending_elements(void) {
  PL_QueueLL *q = pl_queuell_init();
  CU_ASSERT_PTR_NOT_NULL(q);

  for (int i = 0; i < 50; i++) {
    char buf[64];
    snprintf(buf, sizeof(buf), "s-%d", i);
    CU_ASSERT_TRUE(enqueue_string(q, buf));
  }

  CU_ASSERT_EQUAL(q->length(q), 50);

  /* WARNING:
     Your pl_queuell_free() frees nodes but does NOT call pl_free_value_data()
     for PL_STR values stored in nodes.
     That means this test is primarily for detecting leaks under Valgrind/ASan.
     If you want leak-free behavior, queue_free must free PL_STR payloads as well. */
  pl_queuell_free(&q);
  CU_ASSERT_PTR_NULL(q);
}

/* -----------------------------
   Aggressive tests
------------------------------*/

static void test_aggressive_repeat_ints(void) {
  PL_QueueLL *q = pl_queuell_init();
  CU_ASSERT_PTR_NOT_NULL(q);

  const int rounds = 200;
  const int n = 500;

  for (int r = 0; r < rounds; r++) {
    for (int i = 0; i < n; i++) {
      CU_ASSERT_TRUE(enqueue_int(q, r * n + i));
    }
    CU_ASSERT_EQUAL(q->length(q), n);

    for (int i = 0; i < n; i++) {
      PL_Value v = q->dequeue(q);
      CU_ASSERT_EQUAL(v.vtype, PL_INT);
      CU_ASSERT_EQUAL(v.as.ival, r * n + i);
    }
    CU_ASSERT_TRUE(q->is_empty(q));
    CU_ASSERT_EQUAL(q->length(q), 0);
  }

  pl_queuell_free(&q);
}

/* -----------------------------
   Memory stress tests
------------------------------*/

static void test_memory_stress_string_ops(void) {
  PL_QueueLL *q = pl_queuell_init();
  CU_ASSERT_PTR_NOT_NULL(q);

  const int iters = 20000;
  char buf[64];

  for (int i = 0; i < iters; i++) {
    snprintf(buf, sizeof(buf), "str-%d", i);
    CU_ASSERT_TRUE(enqueue_string(q, buf));

    /* occasional peek */
    if ((i % 17) == 0) {
      PL_Value p = q->peek(q);
      CU_ASSERT_EQUAL(p.vtype, PL_STR);
      CU_ASSERT_PTR_NOT_NULL(p.as.sval);
    }

    PL_Value v = q->dequeue(q);
    CU_ASSERT_EQUAL(v.vtype, PL_STR);
    CU_ASSERT_STRING_EQUAL(v.as.sval, buf);
    pl_free_value_data(v);

    CU_ASSERT_TRUE(q->is_empty(q));
    CU_ASSERT_EQUAL(q->length(q), 0);
  }

  pl_queuell_free(&q);
}

/* -----------------------------
   Register and run
------------------------------*/

int main(void) {
  if (CU_initialize_registry() != CUE_SUCCESS)
    return CU_get_error();

  CU_pSuite suite = CU_add_suite("PL_QueueLL Tests", NULL, NULL);
  if (!suite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* logical tests */
  CU_add_test(suite, "init_basic", test_init_basic);
  CU_add_test(suite, "enqueue_rejects_null_data", test_enqueue_rejects_null_data);
  CU_add_test(suite, "fifo_ints_enqueue_dequeue", test_fifo_ints_enqueue_dequeue);
  CU_add_test(suite, "peek_does_not_modify_queue", test_peek_does_not_modify_queue);
  CU_add_test(suite, "dequeue_empty_no_crash", test_dequeue_empty_returns_no_crash);
  CU_add_test(suite, "string_fifo_and_free_value_data", test_string_fifo_and_free_value_data);

  /* edge cases */
  CU_add_test(suite, "mixed_types_fifo", test_mixed_types_int_double_string_fifo);
  CU_add_test(suite, "free_queue_with_pending_elements", test_free_queue_with_pending_elements);

  /* aggressive */
  CU_add_test(suite, "aggressive_repeat_ints", test_aggressive_repeat_ints);

  /* memory stress */
  CU_add_test(suite, "memory_stress_string_ops", test_memory_stress_string_ops);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return 0;
}
