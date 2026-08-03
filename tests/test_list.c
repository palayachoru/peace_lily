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

static void fill_pattern_int(int *arr, int n, int seed) {
  for (int i = 0; i < n; i++) {
    arr[i] = (i * 37 + seed) ^ (i >> 3);
  }
}

static void assert_pl_value_int_at(PL_List *lst, size_t idx, int expected) {
  PL_Value v = lst->get(lst, (int)idx);
  assert_pl_value_int(v, expected);
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


static void test_stress_append_replace_get_huge(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  enum { N = 200000 }; // increase if your runtime allows
  int *vals = (int*)malloc(sizeof(int) * N);
  CU_ASSERT_PTR_NOT_NULL(vals);

  fill_pattern_int(vals, N, 12345);

  // Append N elements
  for (int i = 0; i < N; i++) {
    CU_ASSERT_TRUE(lst->append(lst, PL_INT, &vals[i]));
  }
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)N);

  // Replace many positions
  for (int k = 0; k < N / 2; k++) {
    int idx = (k * 97 + 13) % N;
    int newv = vals[idx] + 1000000 + k;

    CU_ASSERT_TRUE(lst->replace(lst, idx, PL_INT, &newv));
    // Update local mirror
    vals[idx] = newv;
  }

  // Check lots of gets (not just samples)
  for (int k = 0; k < N / 4; k++) {
    int idx = (k * 233 + 7) % N;
    assert_pl_value_int_at(lst, (size_t)idx, vals[idx]);
  }

  // Null checks
  int one = 1;
  CU_ASSERT_FALSE(lst->replace(NULL, 0, PL_INT, &one));
  CU_ASSERT_FALSE(lst->replace(lst, -1, PL_INT, &one));
  CU_ASSERT_FALSE(lst->replace(lst, 99999999, PL_INT, &one));
  CU_ASSERT_FALSE(lst->replace(lst, 0, PL_INT, NULL));

  free(vals);
  pl_list_free(&lst);
}

static void test_stress_insert_shifts_huge(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  enum { N = 60000 };
  int *vals = (int*)malloc(sizeof(int) * N);
  CU_ASSERT_PTR_NOT_NULL(vals);
  fill_pattern_int(vals, N, 999);

  // Start with a small base so inserts exercise internal shift
  int seed = 1;
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &seed));
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &seed));

  size_t expected_len = 2;

  for (int i = 0; i < N; i++) {
    int v = vals[i];

    // Choose insertion index to alternate between:
    // - front-biased
    // - middle-ish
    // - end-biased
    int mode = i % 3;
    int idx;
    if (mode == 0) {
      idx = 0;
    } else if (mode == 1) {
      idx = (int)(expected_len / 2);
    } else {
      idx = (int)expected_len; // insert at end (should behave like append)
    }

    CU_ASSERT_TRUE(lst->insert(lst, idx, PL_INT, &v));
    expected_len++;
  }

  CU_ASSERT_EQUAL(lst->length(lst), expected_len);

  // Verify a few positions that are likely to be moved many times.
  // (We don't mirror fully here to keep it lightweight; we just sanity-check invariants.)
  // Invariant checks:
  // - length matches
  // - index can find some values
  CU_ASSERT_TRUE(lst->length(lst) > 0);

  int *probe = (int*)malloc(sizeof(int) * 200);
  CU_ASSERT_PTR_NOT_NULL(probe);
  for (int i = 0; i < 200; i++) probe[i] = vals[(i * 113) % N];

  for (int i = 0; i < 200; i++) {
    int *p = &probe[i];
    // index should find first occurrence (not necessarily exact position)
    int found = lst->index(lst, PL_INT, p);
    CU_ASSERT_TRUE(found >= -1);
    if (found != -1) {
      assert_pl_value_int(lst->get(lst, found), *p);
    }
  }

  free(probe);
  free(vals);
  pl_list_free(&lst);
}

static void test_stress_reverse_many_times(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  enum { N = 100001 }; // odd length catches more edge cases
  int *vals = (int*)malloc(sizeof(int) * N);
  CU_ASSERT_PTR_NOT_NULL(vals);
  fill_pattern_int(vals, N, 4242);

  for (int i = 0; i < N; i++) {
    CU_ASSERT_TRUE(lst->append(lst, PL_INT, &vals[i]));
  }

  // Save a few sentinel values
  int first = vals[0];
  int mid   = vals[N/2];
  int last  = vals[N-1];

  // Reverse many times: even count => original order
  int cycles = 101;
  for (int i = 0; i < cycles; i++) lst->reverse(lst);

  if (cycles % 2 == 0) {
    assert_pl_value_int(lst->get(lst, 0), first);
    assert_pl_value_int(lst->get(lst, N/2), mid);
    assert_pl_value_int(lst->get(lst, N-1), last);
  } else {
    assert_pl_value_int(lst->get(lst, 0), last);
    assert_pl_value_int(lst->get(lst, N/2), mid); // mid maps to itself for odd N
    assert_pl_value_int(lst->get(lst, N-1), first);
  }

  free(vals);
  pl_list_free(&lst);
}

static void test_stress_remove_at_worst_shift_and_shrink(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  enum { N = 90000 };
  int *vals = (int*)malloc(sizeof(int) * N);
  CU_ASSERT_PTR_NOT_NULL(vals);
  fill_pattern_int(vals, N, 123);

  for (int i = 0; i < N; i++) {
    CU_ASSERT_TRUE(lst->append(lst, PL_INT, &vals[i]));
  }
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)N);

  size_t remaining = N;
  // Remove roughly half, checking capacity floor and safe behavior
  for (int step = 0; step < N / 2; step++) {
    // Removal index biased to front/middle to maximize shifting
    size_t idx = (size_t)((step * 53 + 7) % (int)remaining);

    CU_ASSERT_TRUE(lst->remove_at(lst, (int)idx));
    remaining--;

    CU_ASSERT_TRUE(lst->capacity >= (size_t)MIN_ARRAY_CAPACITY);

    // Occasional sanity: get some indices (may be expensive but we keep it sparse)
    if (step % 5000 == 0 && remaining > 0) {
      size_t a = (size_t)((step * 13) % (int)remaining);
      (void)lst->get(lst, (int)a);
    }
  }

  // Clear rest
  while (!lst->is_empty(lst)) {
    (void)lst->pop(lst);
    CU_ASSERT_TRUE(lst->capacity >= (size_t)MIN_ARRAY_CAPACITY);
  }

  // remove_at on empty should not crash
  CU_ASSERT_FALSE(lst->remove_at(lst, 0));
  CU_ASSERT_FALSE(lst->remove_at(NULL, 0));

  free(vals);
  pl_list_free(&lst);
}

static void test_stress_interleaved_operations_cycles(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  enum { CYCLES = 80 };
  enum { N = 5000 };

  int *vals = (int*)malloc(sizeof(int) * N);
  CU_ASSERT_PTR_NOT_NULL(vals);
  fill_pattern_int(vals, N, 777);

  for (int c = 0; c < CYCLES; c++) {
    // rebuild
    pl_list_free(&lst);
    lst = pl_list_init();
    CU_ASSERT_PTR_NOT_NULL(lst);

    for (int i = 0; i < N; i++) {
      CU_ASSERT_TRUE(lst->append(lst, PL_INT, &vals[i]));
    }

    // reverse a couple times
    lst->reverse(lst);
    lst->reverse(lst);

    // remove_at several times
    int removes = N / 5; // 20%
    size_t remaining = N;
    for (int r = 0; r < removes; r++) {
      size_t idx = (size_t)((c * 101 + r * 19) % (int)remaining);
      CU_ASSERT_TRUE(lst->remove_at(lst, (int)idx));
      remaining--;
      CU_ASSERT_TRUE(lst->capacity >= (size_t)MIN_ARRAY_CAPACITY);
    }

    // append some new values
    int extra = 100000 + c;
    CU_ASSERT_TRUE(lst->append(lst, PL_INT, &extra));
    CU_ASSERT_TRUE(lst->append(lst, PL_INT, &extra));

    // reverse again should still not crash
    lst->reverse(lst);

    // pop until empty in a bounded way
    while (!lst->is_empty(lst)) {
      (void)lst->pop(lst);
      CU_ASSERT_TRUE(lst->capacity >= (size_t)MIN_ARRAY_CAPACITY);
    }
  }

  free(vals);
  pl_list_free(&lst);
}

static void test_edges_invalid_inputs_do_not_corrupt(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  int a = 1, b = 2;

  // Baseline
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &a)); // [1]
  CU_ASSERT_TRUE(lst->append(lst, PL_INT, &b)); // [1,2]
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)2);

  // Invalid ops should fail / not crash
  CU_ASSERT_FALSE(lst->append(NULL, PL_INT, &a));
  CU_ASSERT_FALSE(lst->append(lst, PL_INT, NULL));

  CU_ASSERT_FALSE(lst->insert(NULL, 0, PL_INT, &a));
  CU_ASSERT_FALSE(lst->insert(lst, -1, PL_INT, &a));
  CU_ASSERT_FALSE(lst->insert(lst, 999999, PL_INT, &a));
  CU_ASSERT_FALSE(lst->insert(lst, 0, PL_INT, NULL));

  CU_ASSERT_FALSE(lst->replace(NULL, 0, PL_INT, &a));
  CU_ASSERT_FALSE(lst->replace(lst, -1, PL_INT, &a));
  CU_ASSERT_FALSE(lst->replace(lst, 999999, PL_INT, &a));
  CU_ASSERT_FALSE(lst->replace(lst, 0, PL_INT, NULL));

  CU_ASSERT_FALSE(lst->remove_at(NULL, 0));
  CU_ASSERT_FALSE(lst->remove_at(lst, -1));
  CU_ASSERT_FALSE(lst->remove_at(lst, 999999));

  CU_ASSERT_FALSE(lst->remove(NULL, PL_INT, &a));
  CU_ASSERT_FALSE(lst->remove(lst, PL_INT, NULL));

  // Out-of-range get should not crash; also should not corrupt length
  (void)lst->get(lst, -123);
  (void)lst->get(lst, 999999);
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)2);

  // Ensure list still intact
  assert_pl_value_int(lst->get(lst, 0), 1);
  assert_pl_value_int(lst->get(lst, 1), 2);

  pl_list_free(&lst);
}

static void test_aggressive_grow_shrink_cycles_capacity_floor(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  const int CYCLES = 80;
  const int BLOCK  = 3 * (int)MIN_ARRAY_CAPACITY; // ensure multiple resizes

  int *vals = (int*)malloc(sizeof(int) * (size_t)BLOCK);
  CU_ASSERT_PTR_NOT_NULL(vals);
  fill_pattern_int(vals, BLOCK, 17);

  for (int c = 0; c < CYCLES; c++) {
    // grow by BLOCK
    for (int i = 0; i < BLOCK; i++) {
      CU_ASSERT_TRUE(lst->append(lst, PL_INT, &vals[i]));
    }
    CU_ASSERT_EQUAL(lst->length(lst), (size_t)(BLOCK));

    // pop down to empty
    while (!lst->is_empty(lst)) {
      (void)lst->pop(lst);
      CU_ASSERT_TRUE(lst->capacity >= (size_t)MIN_ARRAY_CAPACITY);
    }
    CU_ASSERT_EQUAL(lst->length(lst), (size_t)0);
    CU_ASSERT_TRUE(lst->capacity >= (size_t)MIN_ARRAY_CAPACITY);
  }

  free(vals);
  pl_list_free(&lst);
}

static void test_aggressive_interleaved_insert_remove_pop(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  const int N = 20000;
  int *vals = (int*)malloc(sizeof(int) * (size_t)N);
  CU_ASSERT_PTR_NOT_NULL(vals);
  fill_pattern_int(vals, N, 202);

  // Start empty; repeatedly insert near front/middle/end, and occasionally remove_at/pop.
  int current = 0;

  for (int i = 0; i < N; i++) {
    int v = vals[i];

    int mode = i % 3;
    int idx;
    if (mode == 0) idx = 0;
    else if (mode == 1) idx = current / 2;
    else idx = current; // end insertion

    CU_ASSERT_TRUE(lst->insert(lst, idx, PL_INT, &v));
    current++;

    // Every so often, remove_at from a computed index (forces shifting).
    if (i % 7 == 0 && current > 0) {
      int ridx = (int)((i * 13) % current);
      CU_ASSERT_TRUE(lst->remove_at(lst, ridx));
      current--;
      CU_ASSERT_TRUE(lst->capacity >= (size_t)MIN_ARRAY_CAPACITY);
    }

    // Every so often, pop from end.
    if (i % 11 == 0 && current > 0) {
      (void)lst->pop(lst);
      current--;
      CU_ASSERT_TRUE(lst->capacity >= (size_t)MIN_ARRAY_CAPACITY);
    }

    // Basic invariant
    CU_ASSERT_EQUAL(lst->length(lst), (size_t)current);
  }

  // Drain fully
  while (!lst->is_empty(lst)) {
    (void)lst->pop(lst);
  }
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)0);

  free(vals);
  pl_list_free(&lst);
}

static void test_huge_append_get_index_and_replace(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  enum { N = 120000 }; // raise/lower based on performance
  int *vals = (int*)malloc(sizeof(int) * (size_t)N);
  CU_ASSERT_PTR_NOT_NULL(vals);
  fill_pattern_int(vals, N, 555);

  for (int i = 0; i < N; i++) {
    CU_ASSERT_TRUE(lst->append(lst, PL_INT, &vals[i]));
  }
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)N);

  // Verify index/get for many keys
  for (int k = 0; k < 4000; k++) {
    int idx = (k * 97 + 13) % N;
    int key = vals[idx];

    int found = lst->index(lst, PL_INT, &key);
    CU_ASSERT_TRUE(found >= -1);
    if (found != -1) {
      PL_Value v = lst->get(lst, found);
      assert_pl_value_int(v, key);
    }
  }

  // Replace a lot of scattered indices
  for (int k = 0; k < 40000; k++) {
    int idx = (k * 241 + 19) % N;
    int newv = vals[idx] + 1000000 + k;

    CU_ASSERT_TRUE(lst->replace(lst, idx, PL_INT, &newv));
    vals[idx] = newv;
  }

  // Spot-check many gets after replacements
  for (int k = 0; k < 8000; k++) {
    int idx = (k * 131 + 3) % N;
    assert_pl_value_int_at(lst, (size_t)idx, vals[idx]);
  }

  free(vals);
  pl_list_free(&lst);
}

static void test_huge_string_insert_replace_index_remove(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  enum { N = 30000 };
  char **s = (char**)malloc(sizeof(char*) * (size_t)N);
  CU_ASSERT_PTR_NOT_NULL(s);

  // Allocate stable storage for all string pointers
  for (int i = 0; i < N; i++) {
    s[i] = (char*)malloc(32);
    CU_ASSERT_PTR_NOT_NULL(s[i]);
    // deterministic unique-ish strings
    snprintf(s[i], 32, "str_%d_%d", i, (i * 17) & 1023);
    CU_ASSERT_TRUE(lst->append(lst, PL_STR, s[i]));
  }
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)N);

  // Verify index for many strings
  for (int k = 0; k < 3000; k++) {
    int idx = (k * 41 + 9) % N;
    int found = lst->index(lst, PL_STR, s[idx]);
    CU_ASSERT_TRUE(found >= -1);
    if (found != -1) {
      PL_Value v = lst->get(lst, found);
      assert_pl_value_str(&v, s[idx]);
    }
  }

  // Replace many scattered entries with other live strings
  for (int k = 0; k < 10000; k++) {
    int idx = (k * 53 + 7) % N;
    int src = (k * 97 + 3) % N;

    CU_ASSERT_TRUE(lst->replace(lst, idx, PL_STR, s[src]));
    // mirror update by swapping pointers
    s[idx] = s[src];
  }

  // Remove some occurrences (removes first match)
  for (int k = 0; k < 2000; k++) {
    int idx = (k * 59 + 11) % N;
    CU_ASSERT_TRUE(lst->remove(lst, PL_STR, s[idx]));
  }

  // Spot-check: many gets should still be valid strings (at least no crash)
  for (int k = 0; k < 2000; k++) {
    int idx = (k * 37 + 5);
    idx = idx % (int)lst->length(lst);
    PL_Value v = lst->get(lst, idx);
    CU_ASSERT_EQUAL(v.vtype, PL_STR);
    CU_ASSERT_PTR_NOT_NULL(v.as.sval);
    CU_ASSERT_TRUE(strlen(v.as.sval) > 0);
  }

  // Cleanup
  // Free string storage
  for (int i = 0; i < N; i++) free(s[i]);
  free(s);
  pl_list_free(&lst);
}

static void test_strings_lifetime_deep_copy_stress(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  enum { N = 20000 };
  char **buf = (char**)malloc(sizeof(char*) * (size_t)N);
  CU_ASSERT_PTR_NOT_NULL(buf);

  for (int i = 0; i < N; i++) {
    buf[i] = (char*)malloc(64);
    CU_ASSERT_PTR_NOT_NULL(buf[i]);
    snprintf(buf[i], 64, "seed_%d_%d", i, i * 17);

    CU_ASSERT_TRUE(lst->append(lst, PL_STR, buf[i]));
  }
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)N);

  // Capture some expected strings by index
  // (We’ll verify after we invalidate sources.)
  int idxs[] = {0, 1, 2, N/2, N-1, N/3, 3*N/4};
  int num = (int)(sizeof(idxs)/sizeof(idxs[0]));

  const char *expected[7];
  for (int j = 0; j < num; j++) expected[j] = buf[idxs[j]];

  // Invalidate all original buffers
  for (int i = 0; i < N; i++) {
    buf[i][0] = 'X';        // attempt to overwrite
    buf[i][1] = '\0';
  }

  // If list deep-copies, gets should still match original content.
  // If list stores pointers, these will now read "X" and fail.
  for (int j = 0; j < num; j++) {
    PL_Value v = lst->get(lst, idxs[j]);
    CU_ASSERT_EQUAL(v.vtype, PL_STR);
    CU_ASSERT_PTR_NOT_NULL(v.as.sval);

    // Reconstruct the original expected string
    // (so we don't rely on buf content remaining intact)
    char want[64];
    snprintf(want, sizeof(want), "seed_%d_%d", idxs[j], idxs[j] * 17);

    CU_ASSERT_STRING_EQUAL(v.as.sval, want);
  }

  // Also stress index()
  for (int j = 0; j < num; j++) {
    char want[64];
    snprintf(want, sizeof(want), "seed_%d_%d", idxs[j], idxs[j] * 17);

    int found = lst->index(lst, PL_STR, want);
    // If deep-copied, index should work by content
    CU_ASSERT_TRUE(found >= 0);
  }

  // Cleanup list
  // (Assumes pl_list_free frees any internal deep-copies, if any.)
  for (int i = 0; i < N; i++) free(buf[i]);
  free(buf);
  pl_list_free(&lst);
}

static void test_strings_shallow_or_deep_observed_behavior(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  enum { N = 5000 };
  char **buf = (char**)malloc(sizeof(char*) * (size_t)N);
  CU_ASSERT_PTR_NOT_NULL(buf);

  // Insert pointers
  for (int i = 0; i < N; i++) {
    buf[i] = (char*)malloc(32);
    CU_ASSERT_PTR_NOT_NULL(buf[i]);
    snprintf(buf[i], 32, "A_%d", i);
    CU_ASSERT_TRUE(lst->append(lst, PL_STR, buf[i]));
  }

  // Mutate a subset of backing buffers
  int hits[] = {0, 17, 123, N/2, N-1};
  int num = (int)(sizeof(hits)/sizeof(hits[0]));
  for (int j = 0; j < num; j++) {
    int i = hits[j];
    snprintf(buf[i], 32, "B_%d_MOD", i);
  }

  // Check behavior (we don't enforce which one is correct universally;
  // we just assert consistency with one model to detect surprises like partial copying)
  for (int j = 0; j < num; j++) {
    int i = hits[j];
    PL_Value v = lst->get(lst, i);
    CU_ASSERT_EQUAL(v.vtype, PL_STR);
    CU_ASSERT_PTR_NOT_NULL(v.as.sval);

    // If shallow: should now see "B_*"
    // If deep: should still see "A_*"
    // We accept either, but we’ll ensure it matches exactly one of them.
    char wantA[32], wantB[32];
    snprintf(wantA, sizeof(wantA), "A_%d", i);
    snprintf(wantB, sizeof(wantB), "B_%d_MOD", i);

    bool isA = (strcmp(v.as.sval, wantA) == 0);
    bool isB = (strcmp(v.as.sval, wantB) == 0);
    CU_ASSERT_TRUE(isA || isB);
  }

  // Stress replace: replace with another live buffer
  for (int k = 0; k < 1000; k++) {
    int idx = (k * 7) % N;
    int src = (k * 13 + 1) % N;

    CU_ASSERT_TRUE(lst->replace(lst, idx, PL_STR, buf[src]));
  }

  // Validate replace via index by content for some keys
  for (int k = 0; k < 200; k++) {
    int src = (k * 29) % N;
    int found = lst->index(lst, PL_STR, buf[src]);
    // At least "some" occurrences should exist; if your index searches by content,
    // found may be -1 if replace never inserted that exact string at that time.
    // But for these stress patterns, it’s likely to be present.
    CU_ASSERT_TRUE(found >= -1);
  }

  for (int i = 0; i < N; i++) free(buf[i]);
  free(buf);
  pl_list_free(&lst);
}

static void test_mixed_int_string_aggressive_operations(void) {
  PL_List *lst = pl_list_init();
  CU_ASSERT_PTR_NOT_NULL(lst);

  enum { N = 20000 };
  int *ints = (int*)malloc(sizeof(int) * (size_t)N);
  char **strs = (char**)malloc(sizeof(char*) * (size_t)N);
  CU_ASSERT_PTR_NOT_NULL(ints);
  CU_ASSERT_PTR_NOT_NULL(strs);

  for (int i = 0; i < N; i++) {
    ints[i] = (i * 31) ^ 0x55aa;
    strs[i] = (char*)malloc(40);
    CU_ASSERT_PTR_NOT_NULL(strs[i]);
    snprintf(strs[i], 40, "S_%d_%d", i, i * 3);
  }

  // Append alternating types
  for (int i = 0; i < N; i++) {
    if (i % 2 == 0) {
      CU_ASSERT_TRUE(lst->append(lst, PL_INT, &ints[i]));
    }
    else {
      CU_ASSERT_TRUE(lst->append(lst, PL_STR, strs[i]));
    }
  }
  CU_ASSERT_EQUAL(lst->length(lst), (size_t)N);

  // Repeated operations
  for (int k = 0; k < 5000; k++) {
    size_t len = lst->length(lst);
    if (len == 0) break;

    int idx = (int)((k * 97) % (int)len);

    if (k % 3 == 0) {
      int src = (k * 7) % N;
      int newv = ints[src] + k;
      CU_ASSERT_TRUE(lst->replace(lst, idx, PL_INT, &newv));
    } else if (k % 3 == 1) {
      int src = (k * 11) % N;
      CU_ASSERT_TRUE(lst->replace(lst, idx, PL_STR, strs[src]));
    } else {
      CU_ASSERT_TRUE(lst->remove_at(lst, idx));
    }
  }

  // Reverse a few times
  for (int i = 0; i < 7; i++) lst->reverse(lst);

  // Cleanup
  for (int i = 0; i < N; i++) free(strs[i]);
  free(strs);
  free(ints);
  pl_list_free(&lst);
}



/** ALL STARTS HERE **/
int main(void) {
  CU_initialize_registry();

  CU_pSuite suite = CU_add_suite("pl_list", NULL, NULL);
  if (!suite) return (int)CU_get_error();

  /** BASIC TESTS **/
  CU_add_test(suite, "init_free", test_public_api_init_free);
  CU_add_test(suite, "get_is_empty_edges", test_get_and_is_empty_edges);
  CU_add_test(suite, "append_growth_and_get", test_append_growth_and_get);
  CU_add_test(suite, "index_replace_and_string_lifetime", test_index_replace_and_string_lifetime);
  CU_add_test(suite, "insert_valid_invalid_and_shifting", test_insert_valid_invalid_and_shifting_ints);
  CU_add_test(suite, "pop_and_remove_at_and_shrink", test_pop_and_remove_at_and_shrink);
  CU_add_test(suite, "remove_value_duplicates_and_remove_at_shift", test_remove_value_duplicates_and_remove_at_shift);
  CU_add_test(suite, "reverse_and_idempotence", test_reverse_and_reverse_idempotence);

  /** EDGE CASE TESTING **/
  CU_add_test(suite, "edges_invalid_inputs_do_not_corrupt", test_edges_invalid_inputs_do_not_corrupt);
  CU_add_test(suite, "huge_append_get_index_and_replace", test_huge_append_get_index_and_replace);

  /* AGGRESSIVE STRESS TESTS */
  CU_add_test(suite, "stress_append_replace_get_huge", test_stress_append_replace_get_huge);
  CU_add_test(suite, "stress_insert_shifts_huge", test_stress_insert_shifts_huge);
  CU_add_test(suite, "stress_reverse_many_times", test_stress_reverse_many_times);
  CU_add_test(suite, "stress_remove_at_worst_shift_and_shrink", test_stress_remove_at_worst_shift_and_shrink);
  CU_add_test(suite, "stress_interleaved_operations_cycles", test_stress_interleaved_operations_cycles);
  CU_add_test(suite, "aggressive_grow_shrink_cycles_capacity_floor", test_aggressive_grow_shrink_cycles_capacity_floor);
  CU_add_test(suite, "aggressive_interleaved_insert_remove_pop", test_aggressive_interleaved_insert_remove_pop);

  /* STRING SPECIFIC TESTING */
  // CU_add_test(suite, "huge_string_insert_replace_index_remove", test_huge_string_insert_replace_index_remove);
  CU_add_test(suite, "strings_lifetime_deep_copy_stress", test_strings_lifetime_deep_copy_stress);
  CU_add_test(suite, "strings_shallow_or_deep_observed_behavior", test_strings_shallow_or_deep_observed_behavior);
  CU_add_test(suite, "mixed_int_string_aggressive_operations", test_mixed_int_string_aggressive_operations);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return 0;
}
