#define _POSIX_C_SOURCE 200809L

#include <CUnit/Basic.h>

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>

#include <plily/llist.h>
#include <plily/common.h>


/* Helper: free PL_Value returned by pop() if it's a string.
   Your comment says caller frees string via pl_free_value_data().
   If you have that function, use it; otherwise this matches your current logic. */
static void free_value_if_str(PL_Value v) {
  if (v.vtype == PL_STR) {
    free(v.as.sval);
  }
}

static int get_int_value(PL_LinkedList *ll, int idx) {
  PL_Value v = ll->get(ll, idx);
  return v.as.ival;
}

static double get_double_value(PL_LinkedList *ll, int idx) {
  PL_Value v = ll->get(ll, idx);
  return v.as.dval;
}

static const char* get_str_value(PL_LinkedList *ll, int idx) {
  PL_Value v = ll->get(ll, idx);
  return v.as.sval;
}

/* ----------------------------
   1) Basic tests
-----------------------------*/

static void test_basic_init_empty(void) {
  PL_LinkedList *ll = pl_linkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(ll);
  CU_ASSERT_PTR_NOT_NULL(ll->_state);

  CU_ASSERT_TRUE(ll->is_empty(ll));
  CU_ASSERT_EQUAL(ll->length(ll), 0);

  pl_linkedlist_free(&ll);
  CU_ASSERT_PTR_NULL(ll);
}

static void test_basic_append_get_length(void) {
  PL_LinkedList *ll = pl_linkedlist_init();
  CU_ASSERT_TRUE(ll->is_empty(ll));
  CU_ASSERT_EQUAL(ll->length(ll), 0);

  int a = 10, b = 20, c = 30;
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &a));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &b));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &c));

  CU_ASSERT_FALSE(ll->is_empty(ll));
  CU_ASSERT_EQUAL(ll->length(ll), 3);

  CU_ASSERT_EQUAL(get_int_value(ll, 0), 10);
  CU_ASSERT_EQUAL(get_int_value(ll, 1), 20);
  CU_ASSERT_EQUAL(get_int_value(ll, 2), 30);

  pl_linkedlist_free(&ll);
}

static void test_basic_index_remove_by_value(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  int x = 5, y = 7, z = 5;
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &x));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &y));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &z));

  int idx = ll->index(ll, PL_INT, &z);
  CU_ASSERT_EQUAL(idx, 0); /* first occurrence of 5 */

  CU_ASSERT_TRUE(ll->remove(ll, PL_INT, &z)); /* remove first 5 */

  CU_ASSERT_EQUAL(ll->length(ll), 2);
  CU_ASSERT_EQUAL(get_int_value(ll, 0), 7);
  CU_ASSERT_EQUAL(get_int_value(ll, 1), 5);

  int idx2 = ll->index(ll, PL_INT, &z);
  CU_ASSERT_EQUAL(idx2, 1);

  pl_linkedlist_free(&ll);
}

static void test_basic_insert_head_middle_tail(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  int v1 = 1, v2 = 2, v3 = 3, v4 = 4;

  /* Start with [2, 3] */
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &v2));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &v3));

  /* Insert head: [1, 2, 3] */
  CU_ASSERT_TRUE(ll->insert(ll, 0, PL_INT, &v1));
  CU_ASSERT_EQUAL(ll->length(ll), 3);
  CU_ASSERT_EQUAL(get_int_value(ll, 0), 1);
  CU_ASSERT_EQUAL(get_int_value(ll, 1), 2);
  CU_ASSERT_EQUAL(get_int_value(ll, 2), 3);

  /* Insert middle at index 2 => [1, 2, 4, 3] */
  CU_ASSERT_TRUE(ll->insert(ll, 2, PL_INT, &v4));
  CU_ASSERT_EQUAL(ll->length(ll), 4);
  CU_ASSERT_EQUAL(get_int_value(ll, 2), 4);

  pl_linkedlist_free(&ll);
}

static void test_basic_replace_int(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  int a = 10, b = 20;
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &a));
  CU_ASSERT_TRUE(ll->replace(ll, 0, PL_INT, &b));

  CU_ASSERT_EQUAL(get_int_value(ll, 0), 20);

  pl_linkedlist_free(&ll);
}

static void test_basic_replace_str(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  char s1[] = "hello";
  char s2[] = "world";

  CU_ASSERT_TRUE(ll->append(ll, PL_STR, s1));
  CU_ASSERT_TRUE(ll->replace(ll, 0, PL_STR, s2));

  const char *got = get_str_value(ll, 0);
  CU_ASSERT_STRING_EQUAL(got, "world");

  pl_linkedlist_free(&ll);
}

static void test_basic_pop_last_and_order(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  int a = 1, b = 2, c = 3;
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &a));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &b));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &c));

  PL_Value v = ll->pop(ll);
  CU_ASSERT_EQUAL(v.vtype, PL_INT);
  CU_ASSERT_EQUAL(v.as.ival, 3);
  CU_ASSERT_EQUAL(ll->length(ll), 2);
  pl_free_value_data(v);

  CU_ASSERT_EQUAL(get_int_value(ll, 0), 1);
  CU_ASSERT_EQUAL(get_int_value(ll, 1), 2);

  v = ll->pop(ll);
  CU_ASSERT_EQUAL(v.as.ival, 2);
  CU_ASSERT_EQUAL(ll->length(ll), 1);
  pl_free_value_data(v);

  v = ll->pop(ll);
  CU_ASSERT_EQUAL(v.as.ival, 1);
  CU_ASSERT_TRUE(ll->is_empty(ll));
  CU_ASSERT_EQUAL(ll->length(ll), 0);
  pl_free_value_data(v);

  pl_linkedlist_free(&ll);
}

static void test_basic_remove_at_and_tail_update(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  int a = 1, b = 2, c = 3;
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &a));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &b));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &c));

  /* remove middle index 1 => [1,3] */
  CU_ASSERT_TRUE(ll->remove_at(ll, 1));
  CU_ASSERT_EQUAL(ll->length(ll), 2);
  CU_ASSERT_EQUAL(get_int_value(ll, 0), 1);
  CU_ASSERT_EQUAL(get_int_value(ll, 1), 3);

  pl_linkedlist_free(&ll);
}

static void test_basic_reverse(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  int a = 1, b = 2, c = 3, d = 4;
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &a));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &b));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &c));
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &d));

  ll->reverse(ll);

  CU_ASSERT_EQUAL(get_int_value(ll, 0), 4);
  CU_ASSERT_EQUAL(get_int_value(ll, 1), 3);
  CU_ASSERT_EQUAL(get_int_value(ll, 2), 2);
  CU_ASSERT_EQUAL(get_int_value(ll, 3), 1);

  pl_linkedlist_free(&ll);
}

/* ----------------------------
   2) Edge case tests
-----------------------------*/

static void test_edge_get_out_of_range(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  int a = 42;
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &a));

  PL_Value v1 = ll->get(ll, -1);
  PL_Value v2 = ll->get(ll, 1); /* out of range */

  /* Your get() returns (PL_Value){0} */
  CU_ASSERT_EQUAL(v1.vtype, 0);
  CU_ASSERT_EQUAL(v2.vtype, 0);

  pl_linkedlist_free(&ll);
}

static void test_edge_index_not_found(void) {
  PL_LinkedList *ll = pl_linkedlist_init();
  int a = 10;
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &a));

  int b = 999;
  CU_ASSERT_EQUAL(ll->index(ll, PL_INT, &b), -1);

  pl_linkedlist_free(&ll);
}

static void test_edge_insert_invalid_indices_and_null_data(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  int x = 1;
  CU_ASSERT_FALSE(ll->insert(ll, -1, PL_INT, &x));
  CU_ASSERT_FALSE(ll->insert(ll, 1, PL_INT, &x)); /* list length 0, index > length */
  CU_ASSERT_FALSE(ll->insert(ll, 0, PL_INT, NULL)); /* null data */
  CU_ASSERT_TRUE(ll->insert(ll, 0, PL_INT, &x));   /* valid */

  pl_linkedlist_free(&ll);
}

static void test_edge_replace_invalid_idx_null_data_bad_vtype(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  int x = 1, y = 2;
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &x));

  CU_ASSERT_FALSE(ll->replace(ll, -1, PL_INT, &y));
  CU_ASSERT_FALSE(ll->replace(ll, 2, PL_INT, &y)); /* idx > length */
  CU_ASSERT_FALSE(ll->replace(ll, 0, PL_INT, NULL));
  /* bad vtype check in your code: only PL_INT/PL_DOUBLE/PL_STR accepted */
  /* If you don't have an invalid enum value, skip this assertion in your build. */
  CU_ASSERT_FALSE(ll->replace(ll, 0, (PL_VType)999, &y));

  pl_linkedlist_free(&ll);
}

static void test_edge_remove_empty_and_remove_at_empty(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  int x = 1;
  CU_ASSERT_FALSE(ll->remove(ll, PL_INT, &x));
  CU_ASSERT_FALSE(ll->remove_at(ll, 0));
  CU_ASSERT_FALSE(ll->remove_at(ll, -1));

  pl_linkedlist_free(&ll);
}

static void test_edge_pop_empty(void) {
  PL_LinkedList *ll = pl_linkedlist_init();
  PL_Value v = ll->pop(ll);
  CU_ASSERT_EQUAL(v.vtype, 0);
  CU_ASSERT_TRUE(ll->is_empty(ll));
  free_value_if_str(v);
  pl_linkedlist_free(&ll);
}

static void test_edge_reverse_single_and_empty(void) {
  PL_LinkedList *ll = pl_linkedlist_init();
  ll->reverse(ll); /* empty should do nothing */

  int x = 7;
  CU_ASSERT_TRUE(ll->append(ll, PL_INT, &x));
  ll->reverse(ll); /* single should do nothing */

  CU_ASSERT_EQUAL(get_int_value(ll, 0), 7);

  pl_linkedlist_free(&ll);
}

static void test_edge_string_workflow_pop_replace_remove(void) {
  PL_LinkedList *ll = pl_linkedlist_init();

  char s1[] = "a";
  char s2[] = "b";
  char s3[] = "c";

  CU_ASSERT_TRUE(ll->append(ll, PL_STR, s1));
  CU_ASSERT_TRUE(ll->append(ll, PL_STR, s2));
  CU_ASSERT_TRUE(ll->append(ll, PL_STR, s3));

  /* replace middle */
  char s2n[] = "bb";
  CU_ASSERT_TRUE(ll->replace(ll, 1, PL_STR, s2n));
  CU_ASSERT_STRING_EQUAL(get_str_value(ll, 1), "bb");

  /* remove first occurrence of "bb" => index 1 currently */
  CU_ASSERT_TRUE(ll->remove(ll, PL_STR, s2n));
  CU_ASSERT_EQUAL(ll->length(ll), 2);
  CU_ASSERT_STRING_EQUAL(get_str_value(ll, 0), "a");
  CU_ASSERT_STRING_EQUAL(get_str_value(ll, 1), "c");

  /* pop string: caller frees */
  PL_Value pv = ll->pop(ll);
  CU_ASSERT_EQUAL(pv.vtype, PL_STR);
  CU_ASSERT_STRING_EQUAL(pv.as.sval, "c");
  free_value_if_str(pv);

  pl_linkedlist_free(&ll);
}

/* ----------------------------
   3) Aggressive tests
   - memory + large data
-----------------------------*/

static void test_aggressive_large_append_get_index_remove(void) {
  PL_LinkedList *ll = pl_linkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(ll);

  const int N = 50000; /* bump higher if your CI allows */
  for (int i = 0; i < N; i++) {
    int v = i * 2;
    CU_ASSERT_TRUE(ll->append(ll, PL_INT, &v));
  }
  CU_ASSERT_EQUAL(ll->length(ll), (size_t)N);

  /* spot-check get */
  CU_ASSERT_EQUAL(get_int_value(ll, 0), 0);
  CU_ASSERT_EQUAL(get_int_value(ll, 1), 2);
  CU_ASSERT_EQUAL(get_int_value(ll, N/2), (N/2) * 2);
  CU_ASSERT_EQUAL(get_int_value(ll, N-1), (N-1) * 2);

  /* spot-check index */
  int target = (N/2) * 2;
  CU_ASSERT_EQUAL(ll->index(ll, PL_INT, &target), N/2);

  /* remove one occurrence */
  CU_ASSERT_TRUE(ll->remove(ll, PL_INT, &target));
  CU_ASSERT_EQUAL(ll->length(ll), (size_t)N - 1);

  /* ensure list is still usable */
  CU_ASSERT_FALSE(ll->is_empty(ll));
  PL_Value v = ll->get(ll, 0);
  CU_ASSERT_EQUAL(v.as.ival, 0);

  pl_linkedlist_free(&ll);
}

static void test_aggressive_large_insert_and_reverse(void) {
  PL_LinkedList *ll = pl_linkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(ll);

  const int N = 20000;
  /* Build using front inserts to stress head handling */
  for (int i = 0; i < N; i++) {
    int v = i + 1; /* 1..N */
    CU_ASSERT_TRUE(ll->insert(ll, 0, PL_INT, &v));
  }
  CU_ASSERT_EQUAL(ll->length(ll), (size_t)N);

  /* Now list should be reversed order already; call reverse again */
  ll->reverse(ll);

  CU_ASSERT_EQUAL(get_int_value(ll, 0), 1);
  CU_ASSERT_EQUAL(get_int_value(ll, N-1), N);

  /* Pop many values to stress tail removal */
  for (int i = 0; i < 1000; i++) {
    PL_Value pv = ll->pop(ll);
    CU_ASSERT_EQUAL(pv.vtype, PL_INT);
    free_value_if_str(pv);
  }
  CU_ASSERT_EQUAL(ll->length(ll), (size_t)N - 1000);

  pl_linkedlist_free(&ll);
}

static void test_aggressive_string_replace_stress(void) {
  PL_LinkedList *ll = pl_linkedlist_init();
  CU_ASSERT_PTR_NOT_NULL(ll);

  const int N = 5000;

  /* append N strings */
  char buf[64];
  for (int i = 0; i < N; i++) {
    snprintf(buf, sizeof(buf), "str-%d", i);
    CU_ASSERT_TRUE(ll->append(ll, PL_STR, buf));
  }
  CU_ASSERT_EQUAL(ll->length(ll), (size_t)N);

  /* replace every element */
  for (int i = 0; i < N; i++) {
    snprintf(buf, sizeof(buf), "new-%d", i);
    CU_ASSERT_TRUE(ll->replace(ll, i, PL_STR, buf));
  }

  /* spot check */
  CU_ASSERT_STRING_EQUAL(get_str_value(ll, 0), "new-0");
  CU_ASSERT_STRING_EQUAL(get_str_value(ll, N-1), "new-4999");

  pl_linkedlist_free(&ll);
}

/* ----------------------------
   Suite registration
-----------------------------*/

int main(void) {
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  CU_pSuite suite = CU_add_suite("LinkedListTests", NULL, NULL);
  if (!suite) return CU_get_error();

  /* Basic */
  CU_add_test(suite, "basic_init_empty", test_basic_init_empty);
  CU_add_test(suite, "basic_append_get_length", test_basic_append_get_length);
  CU_add_test(suite, "basic_index_remove_by_value", test_basic_index_remove_by_value);
  CU_add_test(suite, "basic_insert_head_middle_tail", test_basic_insert_head_middle_tail);
  CU_add_test(suite, "basic_replace_int", test_basic_replace_int);
  CU_add_test(suite, "basic_replace_str", test_basic_replace_str);
  CU_add_test(suite, "basic_pop_last_and_order", test_basic_pop_last_and_order);
  CU_add_test(suite, "basic_remove_at_and_tail_update", test_basic_remove_at_and_tail_update);
  CU_add_test(suite, "basic_reverse", test_basic_reverse);

  /* Edge */
  CU_add_test(suite, "edge_get_out_of_range", test_edge_get_out_of_range);
  CU_add_test(suite, "edge_index_not_found", test_edge_index_not_found);
  CU_add_test(suite, "edge_insert_invalid_indices_and_null_data", test_edge_insert_invalid_indices_and_null_data);
  CU_add_test(suite, "edge_replace_invalid_idx_null_data_bad_vtype", test_edge_replace_invalid_idx_null_data_bad_vtype);
  CU_add_test(suite, "edge_remove_empty_and_remove_at_empty", test_edge_remove_empty_and_remove_at_empty);
  CU_add_test(suite, "edge_pop_empty", test_edge_pop_empty);
  CU_add_test(suite, "edge_reverse_single_and_empty", test_edge_reverse_single_and_empty);
  CU_add_test(suite, "edge_string_workflow_pop_replace_remove", test_edge_string_workflow_pop_replace_remove);

  /* Aggressive */
  CU_add_test(suite, "aggressive_large_append_get_index_remove", test_aggressive_large_append_get_index_remove);
  CU_add_test(suite, "aggressive_large_insert_and_reverse", test_aggressive_large_insert_and_reverse);
  CU_add_test(suite, "aggressive_string_replace_stress", test_aggressive_string_replace_stress);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
