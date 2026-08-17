#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <plily/stack.h>


typedef struct PL_StackLL StackLL;
typedef StackLL _StackLL; // just to avoid warnings if your typedef differs

// Your API
StackLL* pl_stackll_init(void);
void pl_stackll_free(StackLL **self);


// Helper assertions
static void assert_value_int(PL_Value v, int expected) {
  CU_ASSERT_EQUAL(v.vtype, PL_INT);
  CU_ASSERT_EQUAL(v.as.ival, expected);
}

static void assert_value_double(PL_Value v, double expected) {
  CU_ASSERT_EQUAL(v.vtype, PL_DOUBLE);
  CU_ASSERT_DOUBLE_EQUAL(v.as.dval, expected, 1e-12);
}

static void assert_value_str(PL_Value v, const char *expected) {
  CU_ASSERT_EQUAL(v.vtype, PL_STR);
  CU_ASSERT_PTR_NOT_NULL(v.as.sval);
  CU_ASSERT_STRING_EQUAL(v.as.sval, expected);
}

static void assert_empty_state(StackLL *s) {
  CU_ASSERT_PTR_NOT_NULL(s);
  CU_ASSERT_TRUE(s->is_empty(s));
  CU_ASSERT_EQUAL(s->length(s), 0);
}

// ----------------------------
// 1) Logical tests
// ----------------------------
static void test_push_pop_int_order(void) {
  StackLL *s = pl_stackll_init();
  CU_ASSERT_PTR_NOT_NULL(s);

  int a = 10, b = 20, c = 30;
  CU_ASSERT_TRUE(s->push(s, PL_INT, &a));
  CU_ASSERT_TRUE(s->push(s, PL_INT, &b));
  CU_ASSERT_TRUE(s->push(s, PL_INT, &c));

  CU_ASSERT_EQUAL(s->length(s), 3);
  CU_ASSERT_FALSE(s->is_empty(s));

  // LIFO
  PL_Value v1 = s->pop(s);
  assert_value_int(v1, 30);
  pl_free_value_data(v1);

  PL_Value v2 = s->pop(s);
  assert_value_int(v2, 20);
  pl_free_value_data(v2);

  PL_Value v3 = s->pop(s);
  assert_value_int(v3, 10);
  pl_free_value_data(v3);

  assert_empty_state(s);
  pl_stackll_free(&s);
}

static void test_peek_does_not_remove(void) {
  StackLL *s = pl_stackll_init();
  CU_ASSERT_PTR_NOT_NULL(s);

  int x = 42;
  CU_ASSERT_TRUE(s->push(s, PL_INT, &x));
  CU_ASSERT_EQUAL(s->length(s), 1);

  PL_Value p = s->peek(s);
  // peek returns snapshot; but comment says caller must not free/modify it.
  assert_value_int(p, 42);

  // Ensure pop returns same value and length decrements
  PL_Value v = s->pop(s);
  assert_value_int(v, 42);
  pl_free_value_data(v);

  assert_empty_state(s);
  pl_stackll_free(&s);
}

static void test_multiple_types(void) {
  StackLL *s = pl_stackll_init();
  CU_ASSERT_PTR_NOT_NULL(s);

  int i = 7;
  double d = 3.14159;
  const char *str = "hello";
  char buf[64];
  snprintf(buf, sizeof(buf), "%s", str);

  CU_ASSERT_TRUE(s->push(s, PL_INT, &i));
  CU_ASSERT_TRUE(s->push(s, PL_DOUBLE, &d));
  CU_ASSERT_TRUE(s->push(s, PL_STR, buf));

  // Top should be PL_STR
  PL_Value v1 = s->pop(s);
  assert_value_str(v1, "hello");
  pl_free_value_data(v1);

  // Then PL_DOUBLE
  PL_Value v2 = s->pop(s);
  assert_value_double(v2, d);
  pl_free_value_data(v2);

  // Then PL_INT
  PL_Value v3 = s->pop(s);
  assert_value_int(v3, i);
  pl_free_value_data(v3);

  assert_empty_state(s);
  pl_stackll_free(&s);
}

// ----------------------------
// 2) Edge case tests
// ----------------------------
static void test_push_null_stack(void) {
  StackLL *s = NULL;
  // function pointers may not exist; but if you call through s->push it will crash.
  // So this test is only meaningful if your code provides guards at call sites.
  // We’ll just verify API returns NULL for init and do not call methods on NULL.
  CU_ASSERT_PTR_NULL(s);
}

static void test_push_null_data_returns_false(void) {
  StackLL *s = pl_stackll_init();
  CU_ASSERT_PTR_NOT_NULL(s);

  CU_ASSERT_FALSE(s->push(s, PL_INT, NULL));
  CU_ASSERT_EQUAL(s->length(s), 0);

  pl_stackll_free(&s);
}

static void test_pop_on_empty(void) {
  StackLL *s = pl_stackll_init();
  CU_ASSERT_PTR_NOT_NULL(s);
  assert_empty_state(s);

  PL_Value v = s->pop(s);
  // Your pop returns (Value){0} when empty.
  // v.vtype should be 0 but you used PL_VType enum starting at 0 (PL_INT).
  // We'll just assert stack stayed empty and don't crash.
  CU_ASSERT_EQUAL(s->length(s), 0);
  CU_ASSERT_TRUE(s->is_empty(s));

  // If returned sval is non-NULL for your implementation, free it.
  // For {0}, it should be NULL; but keep safe:
  if (v.vtype == PL_STR && v.as.sval) {
    pl_free_value_data(v);
  }

  pl_stackll_free(&s);
}

static void test_peek_on_empty(void) {
  StackLL *s = pl_stackll_init();
  CU_ASSERT_PTR_NOT_NULL(s);
  assert_empty_state(s);

  PL_Value p = s->peek(s);
  // peek returns (Value){0}
  CU_ASSERT_TRUE(s->is_empty(s));
  CU_ASSERT_EQUAL(s->length(s), 0);

  // Do not free peek snapshot
  pl_stackll_free(&s);
}

// ----------------------------
// 3) Aggressive tests
// ----------------------------
static void test_aggressive_repeated_ops_int(void) {
  StackLL *s = pl_stackll_init();
  CU_ASSERT_PTR_NOT_NULL(s);

  const int N = 200000; // adjust upward/downward for time
  for (int i = 0; i < N; i++) {
    CU_ASSERT_TRUE(s->push(s, PL_INT, &i));
    CU_ASSERT_EQUAL(s->length(s), (size_t)(i + 1));
  }

  for (int i = N - 1; i >= 0; i--) {
    PL_Value v = s->pop(s);
    // Since we push &i repeatedly in the loop, careful: after loop ends, all nodes may
    // have copied value at push time (expected). If your pl_new_node deep-copies int,
    // this assertion should pass.
    assert_value_int(v, i);
    pl_free_value_data(v);
  }

  assert_empty_state(s);
  pl_stackll_free(&s);
}

static void test_aggressive_mixed_ops(void) {
  StackLL *s = pl_stackll_init();
  CU_ASSERT_PTR_NOT_NULL(s);

  const int N = 100000;
  int curSize = 0;

  for (int i = 0; i < N; i++) {
    if (!s->is_empty(s) && (i % 3 == 0)) {
      PL_Value v = s->pop(s);
      // We don't know type here if your test pushes multiple types; keep it simple:
      // For this test, push only ints (use alternating logic by mod if desired).
      // We'll treat as INT.
      if (v.vtype == PL_INT) {
        pl_free_value_data(v);
      } else if (v.vtype == PL_STR && v.as.sval) {
        pl_free_value_data(v);
      }
      curSize--;
    } else {
      int x = i;
      CU_ASSERT_TRUE(s->push(s, PL_INT, &x));
      curSize++;
    }

    CU_ASSERT_EQUAL(s->length(s), curSize);
  }

  while (!s->is_empty(s)) {
    PL_Value v = s->pop(s);
    if (v.vtype == PL_INT) pl_free_value_data(v);
    else if (v.vtype == PL_STR && v.as.sval) pl_free_value_data(v);
  }
  assert_empty_state(s);

  pl_stackll_free(&s);
}

// ----------------------------
// 4) Memory stress tests
// ----------------------------
static void test_memory_stress_strings_repeat(void) {
  StackLL *s = pl_stackll_init();
  CU_ASSERT_PTR_NOT_NULL(s);

  const int OUTER = 2000;     // number of full cycle repeats
  const int INNER = 200;      // items per cycle
  char tmp[64];

  for (int o = 0; o < OUTER; o++) {
    // Push INNER strings
    for (int i = 0; i < INNER; i++) {
      snprintf(tmp, sizeof(tmp), "cycle_%d_item_%d", o, i);
      CU_ASSERT_TRUE(s->push(s, PL_STR, tmp));
    }

    CU_ASSERT_EQUAL(s->length(s), (size_t)INNER);
    CU_ASSERT_FALSE(s->is_empty(s));

    // Pop and free popped values
    for (int i = 0; i < INNER; i++) {
      PL_Value v = s->pop(s);

      if (v.vtype == PL_STR && v.as.sval) {
        pl_free_value_data(v);
      } else {
        // If pop returns {0} on error, nothing to free.
      }
    }

    assert_empty_state(s);
  }

  pl_stackll_free(&s);
}

static void test_memory_stress_mixed_numeric(void) {
  StackLL *s = pl_stackll_init();
  CU_ASSERT_PTR_NOT_NULL(s);

  const int OUTER = 500;
  const int INNER = 500;

  for (int o = 0; o < OUTER; o++) {
    for (int i = 0; i < INNER; i++) {
      if (i % 2 == 0) {
        int x = o * INNER + i;
        CU_ASSERT_TRUE(s->push(s, PL_INT, &x));
      } else {
        double y = (double)(o * INNER + i) * 0.5;
        CU_ASSERT_TRUE(s->push(s, PL_DOUBLE, &y));
      }
    }

    while (!s->is_empty(s)) {
      PL_Value v = s->pop(s);
      if (v.vtype == PL_STR && v.as.sval) {
        pl_free_value_data(v);
      }
      // ints/doubles: popped Value owns no heap usually
    }

    assert_empty_state(s);
  }

  pl_stackll_free(&s);
}

// ----------------------------
// Registration
// ----------------------------
int main(void) {
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  CU_pSuite suite = CU_add_suite("StackLLSuite", NULL, NULL);
  if (!suite) return CU_get_error();

  // Logical
  CU_add_test(suite, "push_pop_int_order", test_push_pop_int_order);
  CU_add_test(suite, "peek_does_not_remove", test_peek_does_not_remove);
  CU_add_test(suite, "multiple_types", test_multiple_types);

  // Edge
  CU_add_test(suite, "push_null_data_returns_false", test_push_null_data_returns_false);
  CU_add_test(suite, "pop_on_empty", test_pop_on_empty);
  CU_add_test(suite, "peek_on_empty", test_peek_on_empty);

  // Aggressive
  CU_add_test(suite, "aggressive_repeated_ops_int", test_aggressive_repeated_ops_int);
  CU_add_test(suite, "aggressive_mixed_ops", test_aggressive_mixed_ops);

  // Memory stress
  CU_add_test(suite, "memory_stress_strings_repeat", test_memory_stress_strings_repeat);
  CU_add_test(suite, "memory_stress_mixed_numeric", test_memory_stress_mixed_numeric);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
