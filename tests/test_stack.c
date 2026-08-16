// stack_cunit_tests.c
#define _POSIX_C_SOURCE 200809L

#include <CUnit/Basic.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include <plily/stack.h>
#include <plily/common.h>

// If you have a helper to free a PL_Value safely, include it.
// Example guesses (update to your actual API):
// void pl_free_value_data(PL_Value v);
// void pl_free_node_data(PL_Value v);

static void free_returned_value_if_string(PL_Value v) {
  if (v.vtype == PL_STR) {
    // Your stack_free frees st->_state->arr[i].as.sval
    // Do the same for popped snapshots that caller must free.
    free(v.as.sval);
  }
}

// Helper: push an int value
static void push_int(PL_Stack *st, int x) {
  // pl_update_value likely expects address of x
  // and vtype PL_INT (or similar). Adjust vtype name if different.
  bool push_ok;
  (void)push_ok;

  push_ok = st->push(st, PL_INT, &x);
  CU_ASSERT_TRUE(push_ok);
}

// Helper: push a string (will duplicate/own it inside your stack if pl_update_value does so)
static void push_str(PL_Stack *st, const char *s) {
  // Many implementations take const char* as data.
  bool ok = st->push(st, PL_STR, s);
  CU_ASSERT_TRUE(ok);
}

// Helper: compare peek/pop for int
static void assert_top_int(PL_Stack *st, int expected) {
  CU_ASSERT_FALSE(st->is_empty(st));
  PL_Value v = st->peek(st);
  CU_ASSERT_EQUAL(v.vtype, PL_INT);
  CU_ASSERT_EQUAL(v.as.ival, expected);
}

static void assert_empty(PL_Stack *st) {
  CU_ASSERT_TRUE(st->is_empty(st));
  CU_ASSERT_EQUAL(st->length(st), 0);
  PL_Value v = st->peek(st);
  CU_ASSERT_EQUAL(v.vtype, 0); // (Value){0} from your code path; adjust if needed
}

void test_stack_init_not_null(void) {
  PL_Stack *st = pl_stack_init();
  CU_ASSERT_PTR_NOT_NULL(st);

  CU_ASSERT_TRUE(st->is_empty(st));
  CU_ASSERT_EQUAL(st->length(st), 0);

  pl_stack_free(&st);
  CU_ASSERT_PTR_NULL(st);
}

void test_stack_basic_push_pop_peek(void) {
  PL_Stack *st = pl_stack_init();
  CU_ASSERT_PTR_NOT_NULL(st);

  int a = 10, b = 20, c = 30;

  // Push
  CU_ASSERT_TRUE(st->push(st, PL_INT, &a));
  CU_ASSERT_EQUAL(st->length(st), 1);
  assert_top_int(st, 10);

  CU_ASSERT_TRUE(st->push(st, PL_INT, &b));
  CU_ASSERT_EQUAL(st->length(st), 2);
  assert_top_int(st, 20);

  CU_ASSERT_TRUE(st->push(st, PL_INT, &c));
  CU_ASSERT_EQUAL(st->length(st), 3);
  assert_top_int(st, 30);

  // Pop LIFO
  PL_Value v1 = st->pop(st);
  CU_ASSERT_EQUAL(v1.vtype, PL_INT);
  CU_ASSERT_EQUAL(v1.as.ival, 30);
  free_returned_value_if_string(v1);
  CU_ASSERT_EQUAL(st->length(st), 2);
  assert_top_int(st, 20);

  PL_Value v2 = st->pop(st);
  CU_ASSERT_EQUAL(v2.vtype, PL_INT);
  CU_ASSERT_EQUAL(v2.as.ival, 20);
  free_returned_value_if_string(v2);
  CU_ASSERT_EQUAL(st->length(st), 1);
  assert_top_int(st, 10);

  PL_Value v3 = st->pop(st);
  CU_ASSERT_EQUAL(v3.vtype, PL_INT);
  CU_ASSERT_EQUAL(v3.as.ival, 10);
  free_returned_value_if_string(v3);
  CU_ASSERT_TRUE(st->is_empty(st));
  CU_ASSERT_EQUAL(st->length(st), 0);

  pl_stack_free(&st);
}

void test_stack_pop_empty_peek_empty(void) {
  PL_Stack *st = pl_stack_init();
  CU_ASSERT_PTR_NOT_NULL(st);

  assert_empty(st);

  PL_Value p = st->pop(st);
  // Your pop returns (Value){0} when empty
  CU_ASSERT_EQUAL(p.vtype, 0);

  PL_Value q = st->peek(st);
  CU_ASSERT_EQUAL(q.vtype, 0);

  pl_stack_free(&st);
}

void test_stack_null_self(void) {
  // Since your methods are function pointers inside the PL_Stack instance,
  // we can only test public APIs (init/free). If your CI wants NULL robustness
  // for methods, you may need additional tests with dummy st methods.
  PL_Stack *st = NULL;
  pl_stack_free(&st);
  CU_ASSERT_PTR_NULL(st);

  // Also check init returns non-null and doesn't crash
  PL_Stack *st2 = pl_stack_init();
  CU_ASSERT_PTR_NOT_NULL(st2);
  pl_stack_free(&st2);
}

void test_stack_push_null_data(void) {
  PL_Stack *st = pl_stack_init();
  CU_ASSERT_PTR_NOT_NULL(st);

  // push checks `if (!self || !data) return false;`
  CU_ASSERT_FALSE(st->push(st, PL_INT, NULL));
  CU_ASSERT_TRUE(st->is_empty(st));
  CU_ASSERT_EQUAL(st->length(st), 0);

  pl_stack_free(&st);
}

void test_stack_edge_resize_and_shrink(void) {
  // Your INITIAL_STACK_CAPACITY is 4
  // We'll push 4 then 5 to trigger resize to 8,
  // then pop down below 25% (8/4=2) -> shrink to 4.
  PL_Stack *st = pl_stack_init();
  CU_ASSERT_PTR_NOT_NULL(st);

  int v[20];
  for (int i = 0; i < 10; i++) v[i] = i + 1;

  // Push 4 elements
  for (int i = 0; i < 4; i++) {
    CU_ASSERT_TRUE(st->push(st, PL_INT, &v[i]));
  }
  CU_ASSERT_EQUAL(st->length(st), 4);
  assert_top_int(st, v[3]);

  // Push 5th => triggers resize
  CU_ASSERT_TRUE(st->push(st, PL_INT, &v[4]));
  CU_ASSERT_EQUAL(st->length(st), 5);
  assert_top_int(st, v[4]);

  // Pop down to 2 elements => shrink trigger for capacity 8:
  // shrink when top < capacity/4 => top < 2
  // So when top becomes 1 after popping to 1, shrink should occur.
  // We'll pop to 1 explicitly.
  for (int i = 0; i < 4; i++) {
    PL_Value popped = st->pop(st);
    CU_ASSERT_EQUAL(popped.vtype, PL_INT);
    free_returned_value_if_string(popped);
  }
  // Now originally 5 elements - 4 pops => 1 element left
  CU_ASSERT_EQUAL(st->length(st), 1);
  assert_top_int(st, v[0]);

  // Pop last => empty
  PL_Value last = st->pop(st);
  CU_ASSERT_EQUAL(last.vtype, PL_INT);
  free_returned_value_if_string(last);

  CU_ASSERT_TRUE(st->is_empty(st));
  CU_ASSERT_EQUAL(st->length(st), 0);

  pl_stack_free(&st);
}

void test_stack_aggressive_large_push_pop(void) {
  PL_Stack *st = pl_stack_init();
  CU_ASSERT_PTR_NOT_NULL(st);

  const int N = 200000; // big enough to force multiple resizes
  int *vals = (int*)malloc(sizeof(int) * (size_t)N);
  CU_ASSERT_PTR_NOT_NULL(vals);

  for (int i = 0; i < N; i++) {
    vals[i] = i * 3 + 7;
    CU_ASSERT_TRUE(st->push(st, PL_INT, &vals[i]));
  }
  CU_ASSERT_EQUAL(st->length(st), N);

  // Verify LIFO on a sample and/or fully pop
  for (int i = N - 1; i >= 0; i--) {
    PL_Value popped = st->pop(st);
    CU_ASSERT_EQUAL(popped.vtype, PL_INT);
    CU_ASSERT_EQUAL(popped.as.ival, vals[i]);
    free_returned_value_if_string(popped);

    // Optional occasional peek checks
    if (i > 0 && (i % 50000 == 0)) {
      CU_ASSERT_FALSE(st->is_empty(st));
      PL_Value pk = st->peek(st);
      CU_ASSERT_EQUAL(pk.vtype, PL_INT);
      CU_ASSERT_EQUAL(pk.as.ival, vals[i - 1]);
    }
  }

  CU_ASSERT_TRUE(st->is_empty(st));
  CU_ASSERT_EQUAL(st->length(st), 0);

  free(vals);
  pl_stack_free(&st);
}

void test_stack_memory_stress_repeated_churn(void) {
  PL_Stack *st = pl_stack_init();
  CU_ASSERT_PTR_NOT_NULL(st);

  // Random-like deterministic churn with varying sizes to trigger frequent resize/shrink.
  // Use smaller blocks to increase reallocation frequency.
  const int ROUNDS = 200;
  const int MAX_PUSH = 5000;

  int *vals = (int*)malloc(sizeof(int) * (size_t)MAX_PUSH);
  CU_ASSERT_PTR_NOT_NULL(vals);

  for (int r = 0; r < ROUNDS; r++) {
    int k = (r * 9973) % MAX_PUSH; // deterministic
    if (k < 1) k = 1;

    // push k
    for (int i = 0; i < k; i++) {
      vals[i] = (r + 1) * 1000000 + i;
      CU_ASSERT_TRUE(st->push(st, PL_INT, &vals[i]));
    }
    CU_ASSERT_EQUAL(st->length(st), k);

    // pop all and verify
    for (int i = k - 1; i >= 0; i--) {
      PL_Value popped = st->pop(st);
      CU_ASSERT_EQUAL(popped.vtype, PL_INT);
      CU_ASSERT_EQUAL(popped.as.ival, vals[i]);
      free_returned_value_if_string(popped);
    }
    CU_ASSERT_TRUE(st->is_empty(st));
    CU_ASSERT_EQUAL(st->length(st), 0);
  }

  free(vals);
  pl_stack_free(&st);
}

// Ensure strings behave across resize/shrink and popped snapshot can be freed safely.
void test_stack_string_basic(void) {
  PL_Stack *st = pl_stack_init();
  CU_ASSERT_PTR_NOT_NULL(st);

  push_str(st, "alpha");
  push_str(st, "beta");
  push_str(st, "gamma");

  CU_ASSERT_EQUAL(st->length(st), 3);

  PL_Value pk = st->peek(st);
  CU_ASSERT_EQUAL(pk.vtype, PL_STR);
  CU_ASSERT_PTR_NOT_NULL(pk.as.sval);
  CU_ASSERT_STRING_EQUAL(pk.as.sval, "gamma");

  PL_Value a = st->pop(st);
  CU_ASSERT_EQUAL(a.vtype, PL_STR);
  CU_ASSERT_STRING_EQUAL(a.as.sval, "gamma");
  free_returned_value_if_string(a);

  PL_Value b = st->pop(st);
  CU_ASSERT_STRING_EQUAL(b.as.sval, "beta");
  free_returned_value_if_string(b);

  PL_Value c = st->pop(st);
  CU_ASSERT_STRING_EQUAL(c.as.sval, "alpha");
  free_returned_value_if_string(c);

  CU_ASSERT_TRUE(st->is_empty(st));
  pl_stack_free(&st);
}

void test_stack_string_resize_and_shrink(void) {
  PL_Stack *st = pl_stack_init();
  CU_ASSERT_PTR_NOT_NULL(st);

  // Push enough strings to trigger multiple resizes
  const int N = 2000;
  char buf[64];

  for (int i = 0; i < N; i++) {
    snprintf(buf, sizeof(buf), "s-%d", i);
    push_str(st, buf);
  }
  CU_ASSERT_EQUAL(st->length(st), N);

  // Pop half; verify LIFO correctness for strings
  for (int i = N - 1; i >= N / 2; i--) {
    PL_Value popped = st->pop(st);
    CU_ASSERT_EQUAL(popped.vtype, PL_STR);
    snprintf(buf, sizeof(buf), "s-%d", i);
    CU_ASSERT_STRING_EQUAL(popped.as.sval, buf);
    free_returned_value_if_string(popped);
  }

  // Pop rest
  while (!st->is_empty(st)) {
    PL_Value popped = st->pop(st);
    CU_ASSERT_EQUAL(popped.vtype, PL_STR);
    // We can't easily know exact expected value here without storing,
    // but LIFO order already verified for first half; free to avoid leaks.
    free_returned_value_if_string(popped);
  }

  CU_ASSERT_TRUE(st->is_empty(st));
  pl_stack_free(&st);
}

// Suite setup
int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

int main(void) {
  if (CU_initialize_registry() != CUE_SUCCESS) return CU_get_error();

  CU_pSuite suite = CU_add_suite("Stack_DynamicArray_Tests", init_suite, clean_suite);
  if (!suite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  // 1. Basic logical test
  CU_add_test(suite, "test_stack_init_not_null", test_stack_init_not_null);
  CU_add_test(suite, "test_stack_basic_push_pop_peek", test_stack_basic_push_pop_peek);

  // 2. Edge condition tests
  CU_add_test(suite, "test_stack_pop_empty_peek_empty", test_stack_pop_empty_peek_empty);
  CU_add_test(suite, "test_stack_push_null_data", test_stack_push_null_data);
  CU_add_test(suite, "test_stack_edge_resize_and_shrink", test_stack_edge_resize_and_shrink);
  CU_add_test(suite, "test_stack_null_self", test_stack_null_self);

  // 3. Aggressive test with large amount of data
  CU_add_test(suite, "test_stack_aggressive_large_push_pop", test_stack_aggressive_large_push_pop);

  // 4. Memory stress tests
  CU_add_test(suite, "test_stack_memory_stress_repeated_churn", test_stack_memory_stress_repeated_churn);

  // 5. String based tests
  CU_add_test(suite, "test_stack_string_basic", test_stack_string_basic);
  CU_add_test(suite, "test_stack_string_resize_and_shrink", test_stack_string_resize_and_shrink);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
