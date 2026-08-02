#define _POSIX_C_SOURCE 200809L

#include <CUnit/Basic.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <plily/list.h>
#include <plily/common.h>


static void assert_pl_value_int(const PL_Value v, int expected) {
  //CU_ASSERT_PTR_NOT_NULL(v);
  CU_ASSERT_EQUAL(v.vtype, PL_INT);
  CU_ASSERT_EQUAL(v.as.ival, expected);
}

static void assert_pl_value_double(const PL_Value *v, double expected) {
  CU_ASSERT_PTR_NOT_NULL(v);
  CU_ASSERT_EQUAL(v->vtype, PL_DOUBLE);
  CU_ASSERT_DOUBLE_EQUAL(v->as.dval, expected, 1e-9);
}

static void assert_pl_value_str(const PL_Value *v, const char *expected) {
  CU_ASSERT_PTR_NOT_NULL(v);
  CU_ASSERT_EQUAL(v->vtype, PL_STR);
  CU_ASSERT_PTR_NOT_NULL(v->as.sval);
  CU_ASSERT_STRING_EQUAL(v->as.sval, expected);
}

static void test_public_api_init_free(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  CU_ASSERT_PTR_NOT_NULL(lst->arr);
  CU_ASSERT_EQUAL(lst->capacity, (size_t)MIN_ARRAY_CAPACITY);
  CU_ASSERT_EQUAL(lst->size, (size_t)0);
  CU_ASSERT_PTR_NOT_NULL(lst->get);
  CU_ASSERT_PTR_NOT_NULL(lst->index);
  CU_ASSERT_PTR_NOT_NULL(lst->append);
  CU_ASSERT_PTR_NOT_NULL(lst->insert);
  CU_ASSERT_PTR_NOT_NULL(lst->replace);
  CU_ASSERT_PTR_NOT_NULL(lst->pop);
  CU_ASSERT_PTR_NOT_NULL(lst->remove);
  CU_ASSERT_PTR_NOT_NULL(lst->remove_at);
  CU_ASSERT_PTR_NOT_NULL(lst->reverse);
  CU_ASSERT_PTR_NOT_NULL(lst->length);
  CU_ASSERT_PTR_NOT_NULL(lst->is_empty);

  CU_ASSERT_TRUE(lst->is_empty(lst));
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)0);

  // free(NULL) and free(&NULL) should be safe
  PL_List *nil = NULL;
  pl_list_free(&nil);
  CU_ASSERT_PTR_NULL(nil);

  // double-free should not crash (your code sets *self=NULL)
  pl_list_free(&lst);
  CU_ASSERT_PTR_NULL(lst);

  pl_list_free(&lst);
  CU_ASSERT_PTR_NULL(lst);
}

static void test_get_and_is_empty_edges(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  // empty get returns (Value){0} (at least must not crash)
  (void)lst->get(lst, 0);
  (void)lst->get(lst, -1);
  (void)lst->get(NULL, 0);

  CU_ASSERT_TRUE(lst->is_empty(lst));
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)0);

  // append -> is_empty false
  int x = 11;
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &x));
  CU_ASSERT_FALSE(lst->is_empty(lst));
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)1);

  // out-of-bounds get must not crash
  (void)lst->get(lst, 1);
  (void)lst->get(lst, 999);

  pl_list_free(&lst);
}

static void test_append_growth_and_get(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  // aggressive growth: append many ints to force multiple resizes
  enum { N = 5000 };
  int *buf = (int*)malloc(sizeof(int) * N);
  CU_ASSERT_PTR_NOT_NULL(buf);

  for (int i = 0; i < N; i++) {
    buf[i] = i * 7 - 3;
    CU_ASSERT_TRUE(lst->append(lst, PL_INT, &buf[i]));
  }

  CU_ASSERT_EQUAL(lst->length(lst), (size_t)N);
  CU_ASSERT_TRUE(lst->capacity >= (size_t)N);

  // sample get correctness
  for (int i = 0; i < N; i += 499) {
    PL_Value v = lst->get(lst, i);
    assert_pl_value_int(v, buf[i]);
  }

  // null self / null data checks
  CU_ASSERT_FALSE(lst->append(NULL, PL_INT, &buf[0]));
  CU_ASSERT_FALSE(lst->append(lst, PL_INT, NULL));

  free(buf);
  pl_list_free(&lst);
}

static void test_index_replace_and_string_lifetime(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  // index should find first occurrence
  int a = 10, b = 20, c = 10;
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &a)); // [10]
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &b)); // [10,20]
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &c)); // [10,20,10]

  CU_ASSERT_EQUAL(lst->index(lst, PL_INT, &a), 0);
  CU_ASSERT_EQUAL(lst->index(lst, PL_INT, &b), 1);

  int z = 999;
  CU_ASSERT_EQUAL(lst->index(lst, PL_INT, &z), -1);

  // replace invalid inputs
  CU_ASSERT_FALSE(lst->replace(NULL, 0, PL_INT, &a));
  CU_ASSERT_FALSE(lst->replace(lst, -1, PL_INT, &a));
  CU_ASSERT_FALSE(lst->replace(lst, 100000, PL_INT, &a));
  CU_ASSERT_FALSE(lst->replace(lst, 0, PL_INT, NULL));

  // replace wrong vtype (cast out-of-range)
  CU_ASSERT_FALSE(lst->replace(lst, 0, (PL_VType)999, &a));

  // replace string, ensure stored string contents correct
  const char *s1 = "hello";
  const char *s2 = "world";
  CU_ASSERT_TRUE(lst->append(lst, PL_STR, (void*)s1));
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)4);

  // confirm get type
  PL_Value before = lst->get(lst, 3);
  assert_pl_value_str(&before, s1);

  // replace string at idx=3
  CU_ASSERT_TRUE(lst->replace(lst, 3, PL_STR, (void*)s2));
  PL_Value after = lst->get(lst, 3);
  assert_pl_value_str(&after, s2);

  // index string by content; requires exact strcmp in your implementation
  CU_ASSERT_EQUAL(lst->index(lst, PL_STR, (void*)s2), 3);
  CU_ASSERT_EQUAL(lst->index(lst, PL_STR, (void*)s1), -1);

  pl_list_free(&lst);
}

static void test_insert_valid_invalid_and_shifting_ints(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  int v0 = 1, v1 = 2, v2 = 3, v3 = 4, v4 = 5, v5 = 6;

  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &v0)); // [1]
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &v1)); // [1,2]
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &v2)); // [1,2,3]

  // invalid insert
  CU_ASSERT_FALSE(lst->insert(NULL, 0, PL_INT, &v3));
  CU_ASSERT_FALSE(lst->insert(lst, -1, PL_INT, &v3));
  CU_ASSERT_FALSE(lst->insert(lst, 9999, PL_INT, &v3));
  CU_ASSERT_FALSE(lst->insert(lst, 0, PL_INT, NULL));

  // insert at beginning
  CU_ASSERT_TRUE(lst->insert(lst, 0, PL_INT, &v3)); // [4,1,2,3]
  assert_pl_value_int(lst->get(lst, 0), v3);
  assert_pl_value_int(lst->get(lst, 1), v0);

  // insert in middle
  CU_ASSERT_TRUE(lst->insert(lst, 2, PL_INT, &v4)); // [4,1,5,2,3]
  assert_pl_value_int(lst->get(lst, 2), v4);
  assert_pl_value_int(lst->get(lst, 3), v1);

  // insert at end (index==size) should append
  CU_ASSERT_TRUE(lst->insert(lst, (int)lst->length(lst), PL_INT, &v5)); // [...,6]
  assert_pl_value_int(lst->get(lst, lst->length(lst) - 1), v5);

  pl_list_free(&lst);
}

static void test_pop_and_remove_at_and_shrink(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  // Append enough to grow then pop to trigger shrink condition.
  enum { N = 2000 };
  int *buf = (int*)malloc(sizeof(int) * N);
  CU_ASSERT_PTR_NOT_NULL(buf);

  for (int i = 0; i < N; i++) {
    buf[i] = i;
    CU_ASSERT_TRUE(lst->append(lst, PL_INT, &buf[i]));
  }

  size_t cap0 = lst->capacity;
  CU_ASSERT_TRUE(cap0 >= (size_t)MIN_ARRAY_CAPACITY);

  // Pop many, watch that pop never fails and list remains consistent.
  for (int i = 0; i < N / 2; i++) {
    PL_Value pv = lst->pop(lst);
    (void)pv;
    // capacity should never go below MIN_ARRAY_CAPACITY
    CU_ASSERT_TRUE(lst->capacity >= (size_t)MIN_ARRAY_CAPACITY);
  }

  // Pop all remaining
  while (!lst->is_empty(lst)) {
    (void)lst->pop(lst);
    CU_ASSERT_TRUE(lst->capacity >= (size_t)MIN_ARRAY_CAPACITY);
  }

  // pop on empty => must not crash (returns {0})
  (void)lst->pop(lst);
  (void)lst->pop(NULL);

  free(buf);
  pl_list_free(&lst);
}

static void test_remove_value_duplicates_and_remove_at_shift(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  // duplicates for removal
  int x = 42, y = 7, z = 42;
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &x)); // [42]
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &y)); // [42,7]
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &z)); // [42,7,42]

  // remove first occurrence of 42 => removes index 0
  CU_ASSERT_TRUE(lst->remove(lst, PL_INT, &x));
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)2);
  assert_pl_value_int(lst->get(lst, 0), y);
  assert_pl_value_int(lst->get(lst, 1), z);

  // remove_at shifts correctly: remove index 0 => remaining [42]
  CU_ASSERT_TRUE(lst->remove_at(lst, 0));
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)1);
  assert_pl_value_int(lst->get(lst, 0), z);

  // invalid remove_at
  CU_ASSERT_FALSE(lst->remove_at(NULL, 0));
  CU_ASSERT_FALSE(lst->remove_at(lst, -1));
  CU_ASSERT_FALSE(lst->remove_at(lst, 123));

  // remove not found
  int notfound = 999;
  CU_ASSERT_FALSE(lst->remove(lst, PL_INT, &notfound));

  pl_list_free(&lst);
}

static void test_reverse_and_reverse_idempotence(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  // empty reverse no crash
  lst->reverse(lst);

  // size 1 reverse no change
  int a = 1;
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &a));
  lst->reverse(lst);
  assert_pl_value_int(lst->get(lst, 0), a);

  // size > 1
  int b = 2, c = 3, d = 4;
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &b)); // [1,2]
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &c)); // [1,2,3]
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &d)); // [1,2,3,4]

  lst->reverse(lst);
  assert_pl_value_int(lst->get(lst, 0), d);
  assert_pl_value_int(lst->get(lst, 3), a);

  // reverse twice returns original
  lst->reverse(lst);
  assert_pl_value_int(lst->get(lst, 0), a);
  assert_pl_value_int(lst->get(lst, 3), d);

  // reverse NULL self should not crash
  lst->reverse(NULL);

  pl_list_free(&lst);
}


int main(void) {
  CU_initialize_registry();

  CU_pSuite suite = CU_add_suite("pl_list", NULL, NULL);
  if (!suite) return (int)CU_get_error();

  CU_add_test(suite, "public_api_init_free", test_public_api_init_free);

  CU_add_test(suite, "public_api_get_is_empty_edges", test_get_and_is_empty_edges);
  CU_add_test(suite, "public_api_append_growth_and_get", test_append_growth_and_get);

  CU_add_test(suite, "public_api_index_replace_and_string_lifetime", test_index_replace_and_string_lifetime);

  CU_add_test(suite, "public_api_insert_valid_invalid_and_shifting", test_insert_valid_invalid_and_shifting_ints);

  CU_add_test(suite, "public_api_pop_and_remove_at_and_shrink", test_pop_and_remove_at_and_shrink);
  CU_add_test(suite, "public_api_remove_value_duplicates_and_remove_at_shift", test_remove_value_duplicates_and_remove_at_shift);

  CU_add_test(suite, "public_api_reverse_and_idempotence", test_reverse_and_reverse_idempotence);


  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return 0;
}
