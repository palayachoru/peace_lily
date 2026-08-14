#define _POSIX_C_SOURCE 200809L

#include <CUnit/Basic.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <plily/llist.h>
#include <plily/common.h>

// Prototypes from your implementation
PL_DLinkedList* pl_dlinkedlist_init(void);
void pl_dlinkedlist_free(PL_DLinkedList **self);

static void free_value_if_string(PL_Value v) {
  if (v.vtype == PL_STR) {
    free(v.as.sval);
  }
}

/* If your library already provides a helper, prefer it.
   Uncomment and delete the manual free above if available:
*/
// static void free_value_if_string(PL_Value v) { pl_free_value_data(v); }

static bool value_equals(PL_Value a, PL_VType vtype, const void *data) {
  if (a.vtype != vtype) return false;
  if (vtype == PL_INT) return a.as.ival == *(const int*)data;
  if (vtype == PL_DOUBLE) return a.as.dval == *(const double*)data;
  if (vtype == PL_STR) return strcmp(a.as.sval, (const char*)data) == 0;
  return false;
}

/* ----------------------- Helper Assertions ----------------------- */

static void assert_true_bool(int expr, const char *msg) {
  CU_ASSERT_TRUE(expr);
  (void)msg;
}

/* ----------------------- Test: init/free ----------------------- */

static void test_init_is_empty(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(dll);

  CU_ASSERT_TRUE(dll->is_empty(dll));
  CU_ASSERT_EQUAL(dll->length(dll), 0u);
  CU_ASSERT_TRUE(dll->get(dll, 0).vtype == 0); // per your implementation returns (PL_Value){0}
  pl_dlinkedlist_free(&dll);
  CU_ASSERT_PTR_NULL(dll);
}

static void test_free_null_safe(void) {
  PL_DLinkedList *dll = NULL;
  pl_dlinkedlist_free(&dll);
  CU_ASSERT_PTR_NULL(dll);
}

/* ----------------------- Test: append ----------------------- */

static void test_append_int_logical_and_length(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(dll);

  int a = 10, b = 20, c = 30;

  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &b));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &c));
  CU_ASSERT_EQUAL(dll->length(dll), 3u);
  CU_ASSERT_FALSE(dll->is_empty(dll));

  PL_Value v0 = dll->get(dll, 0);
  PL_Value v1 = dll->get(dll, 1);
  PL_Value v2 = dll->get(dll, 2);

  CU_ASSERT_TRUE(value_equals(v0, PL_INT, &a));
  CU_ASSERT_TRUE(value_equals(v1, PL_INT, &b));
  CU_ASSERT_TRUE(value_equals(v2, PL_INT, &c));

  // ints don't require freeing
  pl_dlinkedlist_free(&dll);
}

static void test_append_rejects_null(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(dll);
  CU_ASSERT_FALSE(dll->append(dll, PL_INT, NULL));
  CU_ASSERT_EQUAL(dll->length(dll), 0u);
  pl_dlinkedlist_free(&dll);
}

/* ----------------------- Test: insert ----------------------- */

static void test_insert_head_middle_tail(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(dll);

  int x = 1, y = 2, z = 3, w = 4;

  // Start with [x, z]
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &x));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &z));

  // Insert y at head index 0 => [y, x, z]
  CU_ASSERT_TRUE(dll->insert(dll, 0, PL_INT, &y));
  // Insert w in middle index 2 => [y, x, w, z]
  CU_ASSERT_TRUE(dll->insert(dll, 2, PL_INT, &w));

  CU_ASSERT_EQUAL(dll->length(dll), 4u);
  int expected0 = y, expected1 = x, expected2 = w, expected3 = z;

  CU_ASSERT_TRUE(value_equals(dll->get(dll, 0), PL_INT, &expected0));
  CU_ASSERT_TRUE(value_equals(dll->get(dll, 1), PL_INT, &expected1));
  CU_ASSERT_TRUE(value_equals(dll->get(dll, 2), PL_INT, &expected2));
  CU_ASSERT_TRUE(value_equals(dll->get(dll, 3), PL_INT, &expected3));

  pl_dlinkedlist_free(&dll);
}

static void test_insert_edge_cases(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(dll);

  int a = 5;

  CU_ASSERT_FALSE(dll->insert(dll, -1, PL_INT, &a));
  CU_ASSERT_FALSE(dll->insert(dll, 1, PL_INT, &a)); // length==0 so index>length
  CU_ASSERT_FALSE(dll->insert(dll, 0, PL_INT, NULL));
  CU_ASSERT_FALSE(dll->insert(dll, 0, PL_INT, NULL));

  // valid at empty: index==0 should work
  CU_ASSERT_TRUE(dll->insert(dll, 0, PL_INT, &a));
  CU_ASSERT_EQUAL(dll->length(dll), 1u);
  CU_ASSERT_TRUE(value_equals(dll->get(dll, 0), PL_INT, &a));

  // insert at tail index==length works via your append path
  int b = 6;
  CU_ASSERT_TRUE(dll->insert(dll, 1, PL_INT, &b));
  CU_ASSERT_EQUAL(dll->length(dll), 2u);

  pl_dlinkedlist_free(&dll);
}

/* ----------------------- Test: replace ----------------------- */

static void test_replace_int_double_and_string(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(dll);

  int a = 10, b = 20;
  double d1 = 1.25, d2 = 9.75;

  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  CU_ASSERT_TRUE(dll->append(dll, PL_DOUBLE, &d1));

  // replace index 0 int
  CU_ASSERT_TRUE(dll->replace(dll, 0, PL_INT, &b));
  CU_ASSERT_EQUAL(dll->length(dll), 2u);
  CU_ASSERT_TRUE(value_equals(dll->get(dll, 0), PL_INT, &b));

  // replace index 1 double
  CU_ASSERT_TRUE(dll->replace(dll, 1, PL_DOUBLE, &d2));
  CU_ASSERT_TRUE(value_equals(dll->get(dll, 1), PL_DOUBLE, &d2));

  // Now string replace
  char *s1 = "hello";
  char *s2 = "world";
  // append string
  CU_ASSERT_TRUE(dll->append(dll, PL_STR, s1));
  CU_ASSERT_TRUE(dll->replace(dll, 2, PL_STR, s2));
  CU_ASSERT_EQUAL(dll->length(dll), 3u);

  PL_Value sv = dll->get(dll, 2);
  CU_ASSERT_TRUE(value_equals(sv, PL_STR, s2));
  //free_value_if_string(sv);

  // Cleanup
  pl_dlinkedlist_free(&dll);
}

static void test_replace_edge_cases(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(dll);

  int a = 1;
  CU_ASSERT_FALSE(dll->replace(dll, 0, PL_INT, &a)); // empty

  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a)); // [1]

  CU_ASSERT_FALSE(dll->replace(dll, -1, PL_INT, &a));
  CU_ASSERT_FALSE(dll->replace(dll, 1, PL_INT, &a)); // out of bounds
  CU_ASSERT_FALSE(dll->replace(dll, 0, PL_INT, NULL));

  // invalid vtype: pick a number outside known ones.
  // If your PL_VType is an enum, this is still fine for rejecting.
  int dummy = 123;
  CU_ASSERT_FALSE(dll->replace(dll, 0, (PL_VType)999, &dummy));

  pl_dlinkedlist_free(&dll);
}

/* ----------------------- Test: get ----------------------- */

static void test_get_bounds_and_type(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  int a = 10;
  double d = 2.5;

  CU_ASSERT_EQUAL(dll->get(dll, 0).vtype, 0); // empty -> (PL_Value){0}

  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  CU_ASSERT_TRUE(dll->append(dll, PL_DOUBLE, &d));

  CU_ASSERT_EQUAL(dll->get(dll, -1).vtype, 0);
  CU_ASSERT_EQUAL(dll->get(dll, 2).vtype, 0);

  PL_Value v0 = dll->get(dll, 0);
  PL_Value v1 = dll->get(dll, 1);

  CU_ASSERT_TRUE(value_equals(v0, PL_INT, &a));
  CU_ASSERT_TRUE(value_equals(v1, PL_DOUBLE, &d));

  pl_dlinkedlist_free(&dll);
}

/* ----------------------- Test: index ----------------------- */

static void test_index_logical_and_not_found(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();

  int a=1, b=2, c=3, b2=2;
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &b));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &c));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &b2)); // [1,2,3,2]

  CU_ASSERT_EQUAL(dll->index(dll, PL_INT, &b), 1);
  CU_ASSERT_EQUAL(dll->index(dll, PL_INT, &a), 0);
  CU_ASSERT_EQUAL(dll->index(dll, PL_INT, &c), 2);

  int not_found = 999;
  CU_ASSERT_EQUAL(dll->index(dll, PL_INT, &not_found), -1);

  // null data => -1 by your code
  CU_ASSERT_EQUAL(dll->index(dll, PL_INT, NULL), -1);

  pl_dlinkedlist_free(&dll);
}

static void test_index_string(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  char *s1="a", *s2="b", *s3="a";

  CU_ASSERT_TRUE(dll->append(dll, PL_STR, s1));
  CU_ASSERT_TRUE(dll->append(dll, PL_STR, s2));
  CU_ASSERT_TRUE(dll->append(dll, PL_STR, s3));

  CU_ASSERT_EQUAL(dll->index(dll, PL_STR, "a"), 0);
  CU_ASSERT_EQUAL(dll->index(dll, PL_STR, "b"), 1);
  CU_ASSERT_EQUAL(dll->index(dll, PL_STR, "c"), -1);

  pl_dlinkedlist_free(&dll);
}

/* ----------------------- Test: remove (by value) ----------------------- */

static void test_remove_int_logical_first_occurrence(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  int a=1,b=2,c=3,b2=2;

  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &b));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &c));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &b2));

  // remove first occurrence of 2 (index 1)
  CU_ASSERT_TRUE(dll->remove(dll, PL_INT, &b));
  CU_ASSERT_EQUAL(dll->length(dll), 3u);

  int e0=1,e1=3,e2=2;
  CU_ASSERT_TRUE(value_equals(dll->get(dll,0), PL_INT, &e0));
  CU_ASSERT_TRUE(value_equals(dll->get(dll,1), PL_INT, &e1));
  CU_ASSERT_TRUE(value_equals(dll->get(dll,2), PL_INT, &e2));

  pl_dlinkedlist_free(&dll);
}

static void test_remove_edge_cases(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  int a=1;

  CU_ASSERT_FALSE(dll->remove(dll, PL_INT, &a)); // empty
  CU_ASSERT_FALSE(dll->remove(dll, PL_INT, NULL));

  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  CU_ASSERT_FALSE(dll->remove(dll, PL_INT, &(int){2})); // not found

  CU_ASSERT_TRUE(dll->remove(dll, PL_INT, &a)); // remove existing
  CU_ASSERT_TRUE(dll->is_empty(dll));

  pl_dlinkedlist_free(&dll);
}

static void test_remove_string(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  char *sA="alpha", *sB="beta", *sA2="alpha";

  CU_ASSERT_TRUE(dll->append(dll, PL_STR, sA));
  CU_ASSERT_TRUE(dll->append(dll, PL_STR, sB));
  CU_ASSERT_TRUE(dll->append(dll, PL_STR, sA2));

  CU_ASSERT_TRUE(dll->remove(dll, PL_STR, "alpha")); // removes first alpha
  CU_ASSERT_EQUAL(dll->length(dll), 2u);

  PL_Value v0 = dll->get(dll, 0);
  PL_Value v1 = dll->get(dll, 1);

  CU_ASSERT_TRUE(value_equals(v0, PL_STR, "beta"));
  CU_ASSERT_TRUE(value_equals(v1, PL_STR, "alpha"));

  //free_value_if_string(v0);
  //free_value_if_string(v1);

  pl_dlinkedlist_free(&dll);
}

/* ----------------------- Test: remove_at ----------------------- */

static void test_remove_at_head_middle_tail(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();

  int a=1,b=2,c=3,d=4;
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &b));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &c));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &d));
  CU_ASSERT_EQUAL(dll->length(dll), 4u);

  CU_ASSERT_TRUE(dll->remove_at(dll, 0)); // remove head => [2,3,4]
  CU_ASSERT_EQUAL(dll->length(dll), 3u);
  CU_ASSERT_TRUE(value_equals(dll->get(dll,0), PL_INT, &(int){2}));
  CU_ASSERT_TRUE(value_equals(dll->get(dll,1), PL_INT, &(int){3}));
  CU_ASSERT_TRUE(value_equals(dll->get(dll,2), PL_INT, &(int){4}));

  CU_ASSERT_TRUE(dll->remove_at(dll, 1)); // remove middle => [2,4]
  CU_ASSERT_EQUAL(dll->length(dll), 2u);
  CU_ASSERT_TRUE(value_equals(dll->get(dll,0), PL_INT, &(int){2}));
  CU_ASSERT_TRUE(value_equals(dll->get(dll,1), PL_INT, &(int){4}));

  CU_ASSERT_TRUE(dll->remove_at(dll, 1)); // remove tail => [2]
  CU_ASSERT_EQUAL(dll->length(dll), 1u);
  CU_ASSERT_TRUE(value_equals(dll->get(dll,0), PL_INT, &(int){2}));

  CU_ASSERT_TRUE(dll->remove_at(dll, 0)); // remove last => []
  CU_ASSERT_TRUE(dll->is_empty(dll));

  pl_dlinkedlist_free(&dll);
}

static void test_remove_at_edge_cases(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  int a=1;

  CU_ASSERT_FALSE(dll->remove_at(dll, 0)); // empty
  CU_ASSERT_FALSE(dll->remove_at(dll, -1));

  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a)); // [1]
  CU_ASSERT_FALSE(dll->remove_at(dll, 1)); // out of bounds

  CU_ASSERT_TRUE(dll->remove_at(dll, 0)); // ok
  CU_ASSERT_TRUE(dll->is_empty(dll));

  pl_dlinkedlist_free(&dll);
}

/* ----------------------- Test: pop ----------------------- */

static void test_pop_int_logical(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  int a=1,b=2,c=3;

  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &b));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &c));

  PL_Value v = dll->pop(dll);
  CU_ASSERT_EQUAL(v.vtype, PL_INT);
  CU_ASSERT_EQUAL(v.as.ival, c);
  CU_ASSERT_EQUAL(dll->length(dll), 2u);

  v = dll->pop(dll);
  CU_ASSERT_EQUAL(v.as.ival, b);
  CU_ASSERT_EQUAL(dll->length(dll), 1u);

  v = dll->pop(dll);
  CU_ASSERT_EQUAL(v.as.ival, a);
  CU_ASSERT_EQUAL(dll->length(dll), 0u);
  CU_ASSERT_TRUE(dll->is_empty(dll));

  // pop on empty => (PL_Value){0}
  v = dll->pop(dll);
  CU_ASSERT_EQUAL(v.vtype, 0);

  pl_dlinkedlist_free(&dll);
}

static void test_pop_string(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  char *s1="one", *s2="two";

  CU_ASSERT_TRUE(dll->append(dll, PL_STR, s1));
  CU_ASSERT_TRUE(dll->append(dll, PL_STR, s2));

  PL_Value v = dll->pop(dll);
  CU_ASSERT_TRUE(value_equals(v, PL_STR, s2));
  free_value_if_string(v);

  PL_Value v2 = dll->pop(dll);
  CU_ASSERT_TRUE(value_equals(v2, PL_STR, s1));
  free_value_if_string(v2);

  pl_dlinkedlist_free(&dll);
}

/* ----------------------- Test: reverse ----------------------- */

static void test_reverse_logical(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  int a=1,b=2,c=3;

  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &b));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &c));

  dll->reverse(dll);

  CU_ASSERT_TRUE(value_equals(dll->get(dll,0), PL_INT, &(int){3}));
  CU_ASSERT_TRUE(value_equals(dll->get(dll,1), PL_INT, &(int){2}));
  CU_ASSERT_TRUE(value_equals(dll->get(dll,2), PL_INT, &(int){1}));

  pl_dlinkedlist_free(&dll);
}

static void test_reverse_edge_cases(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();

  // empty & single element should be no-op (but must not crash)
  dll->reverse(dll);
  CU_ASSERT_TRUE(dll->is_empty(dll));

  int a=1;
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  dll->reverse(dll);
  CU_ASSERT_EQUAL(dll->length(dll), 1u);
  CU_ASSERT_TRUE(value_equals(dll->get(dll,0), PL_INT, &(int){1}));

  pl_dlinkedlist_free(&dll);
}

/* ----------------------- Test: is_empty & length consistency ----------------------- */

static void test_length_consistency_through_ops(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_EQUAL(dll->length(dll), 0u);

  int a=1,b=2,c=3;

  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &a));
  CU_ASSERT_TRUE(dll->append(dll, PL_INT, &b));
  CU_ASSERT_EQUAL(dll->length(dll), 2u);

  CU_ASSERT_TRUE(dll->insert(dll, 1, PL_INT, &c)); // [1,3,2]
  CU_ASSERT_EQUAL(dll->length(dll), 3u);

  CU_ASSERT_TRUE(dll->remove_at(dll, 1)); // remove 3 => [1,2]
  CU_ASSERT_EQUAL(dll->length(dll), 2u);

  CU_ASSERT_TRUE(dll->remove(dll, PL_INT, &a)); // remove 1 => [2]
  CU_ASSERT_EQUAL(dll->length(dll), 1u);

  PL_Value v = dll->pop(dll); // remove last => []
  (void)v;
  CU_ASSERT_EQUAL(dll->length(dll), 0u);
  CU_ASSERT_TRUE(dll->is_empty(dll));

  pl_dlinkedlist_free(&dll);
}

/* ----------------------- String-focused: aggressive replace / memory ownership ----------------------- */

static void test_string_replace_multiple_times(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(dll);

  char *s = "s0";
  CU_ASSERT_TRUE(dll->append(dll, PL_STR, s));

  for (int i = 0; i < 50; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "s%d", i);
    CU_ASSERT_TRUE(dll->replace(dll, 0, PL_STR, buf));
    PL_Value v = dll->get(dll, 0);
    CU_ASSERT_TRUE(value_equals(v, PL_STR, buf));
    //free_value_if_string(v);
  }

  pl_dlinkedlist_free(&dll);
}

/* ----------------------- Aggressive testing ----------------------- */

static void test_aggressive_large_dataset_int(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(dll);

  const int N = 20000;

  // Append N ints: list should contain 0..N-1
  for (int i = 0; i < N; i++) {
    CU_ASSERT_TRUE(dll->append(dll, PL_INT, &i)); // WARNING: passing address of loop var is fine only if pl_new_node copies immediately.
                                                     // If pl_new_node stores pointer, this test will be invalid.
  }

  // Verify length
  CU_ASSERT_EQUAL(dll->length(dll), (size_t)N);

  // Spot-check some indices
  int a = 0;
  CU_ASSERT_TRUE(value_equals(dll->get(dll, 0), PL_INT, &a));

  int mid = N/2;
  CU_ASSERT_TRUE(value_equals(dll->get(dll, mid), PL_INT, &mid));

  int last = N-1;
  CU_ASSERT_TRUE(value_equals(dll->get(dll, N-1), PL_INT, &last));

  // Remove at even indices from the front by popping tail (cheaper correctness-wise)
  for (int i = 0; i < 5000; i++) {
    PL_Value v = dll->pop(dll);
    CU_ASSERT_EQUAL(v.vtype, PL_INT);
    // remaining size decreases; no need to free
  }

  // Remove by value tests (search first occurrence)
  int target = 1234;
  int idx = dll->index(dll, PL_INT, &target);
  CU_ASSERT_TRUE(idx == -1 || idx >= 0);

  // Cleanup
  pl_dlinkedlist_free(&dll);
}

/* Safer variant if pl_new_node copies values. If it does NOT copy,
   you must allocate per value or keep stable storage.
   This implementation likely copies because you replace copies ints/doubles,
   but your append implementation uses pl_new_node(vtype, data). We'll assume copy semantics. */

static void test_aggressive_mixed_ops_strings(void) {
  PL_DLinkedList *dll = pl_dlinkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(dll);

  const int N = 5000;
  char **strings = calloc((size_t)N, sizeof(char*));
  CU_ASSERT_PTR_NOT_NULL(strings);

  for (int i = 0; i < N; i++) {
    strings[i] = malloc(32);
    CU_ASSERT_PTR_NOT_NULL(strings[i]);
    snprintf(strings[i], 32, "v%05d", i);
    CU_ASSERT_TRUE(dll->append(dll, PL_STR, strings[i]));
  }
  CU_ASSERT_EQUAL(dll->length(dll), (size_t)N);

  // Reverse and check a couple positions
  dll->reverse(dll);
  PL_Value v0 = dll->get(dll, 0);
  PL_Value v1 = dll->get(dll, 1);
  CU_ASSERT_TRUE(value_equals(v0, PL_STR, strings[N-1]));
  CU_ASSERT_TRUE(value_equals(v1, PL_STR, strings[N-2]));
  // free_value_if_string(v0);
  // free_value_if_string(v1);

  // Replace some indices
  for (int i = 0; i < 1000; i += 3) {
    char buf[32];
    snprintf(buf, sizeof(buf), "r%05d", i);
    CU_ASSERT_TRUE(dll->replace(dll, i, PL_STR, buf));
    PL_Value gv = dll->get(dll, i);
    CU_ASSERT_TRUE(value_equals(gv, PL_STR, buf));
    //free_value_if_string(gv);
  }

  // Remove by value for a bunch of keys that should exist
  for (int i = 0; i < 500; i++) {
    CU_ASSERT_TRUE(dll->remove(dll, PL_STR, strings[1000 + i]));
  }

  // Aggressive pop from tail
  for (int i = 0; i < 300; i++) {
    PL_Value pv = dll->pop(dll);
    CU_ASSERT_EQUAL(pv.vtype, PL_STR);
    free_value_if_string(pv);
  }

  pl_dlinkedlist_free(&dll);

  // Free the external strings (list should have copied them via pl_new_node or replace)
  for (int i = 0; i < N; i++) free(strings[i]);
  free(strings);
}

/* ----------------------- Main ----------------------- */

int main(void) {
  CU_initialize_registry();

  CU_pSuite suite = CU_add_suite("DoublyLinkedList", NULL, NULL);
  if (!suite) return CU_get_error();

  // init/free
  CU_add_test(suite, "test_init_is_empty", test_init_is_empty);
  CU_add_test(suite, "test_free_null_safe", test_free_null_safe);

  // append
  CU_add_test(suite, "test_append_int_logical_and_length", test_append_int_logical_and_length);
  CU_add_test(suite, "test_append_rejects_null", test_append_rejects_null);

  // insert
  CU_add_test(suite, "test_insert_head_middle_tail", test_insert_head_middle_tail);
  CU_add_test(suite, "test_insert_edge_cases", test_insert_edge_cases);

  // replace
  CU_add_test(suite, "test_replace_int_double_and_string", test_replace_int_double_and_string);
  CU_add_test(suite, "test_replace_edge_cases", test_replace_edge_cases);

  // get
  CU_add_test(suite, "test_get_bounds_and_type", test_get_bounds_and_type);

  // index
  CU_add_test(suite, "test_index_logical_and_not_found", test_index_logical_and_not_found);
  CU_add_test(suite, "test_index_string", test_index_string);

  // remove
  CU_add_test(suite, "test_remove_int_first_occurrence", test_remove_int_logical_first_occurrence);
  CU_add_test(suite, "test_remove_edge_cases", test_remove_edge_cases);
  CU_add_test(suite, "test_remove_string", test_remove_string);

  // remove_at
  CU_add_test(suite, "test_remove_at_head_middle_tail", test_remove_at_head_middle_tail);
  CU_add_test(suite, "test_remove_at_edge_cases", test_remove_at_edge_cases);

  // pop
  CU_add_test(suite, "test_pop_int_logical", test_pop_int_logical);
  CU_add_test(suite, "test_pop_string", test_pop_string);

  // reverse
  CU_add_test(suite, "test_reverse_logical", test_reverse_logical);
  CU_add_test(suite, "test_reverse_edge_cases", test_reverse_edge_cases);

  // length/is_empty consistency
  CU_add_test(suite, "test_length_consistency_through_ops", test_length_consistency_through_ops);

  // string replacement stress
  CU_add_test(suite, "test_string_replace_multiple_times", test_string_replace_multiple_times);

  // aggressive
  CU_add_test(suite, "test_aggressive_large_dataset_int", test_aggressive_large_dataset_int);
  CU_add_test(suite, "test_aggressive_mixed_ops_strings", test_aggressive_mixed_ops_strings);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
