#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <plily/queue.h>

/*
 * Change this if your actual enum constant has another name.
 */
#define TEST_INT_TYPE PL_INT


/* ============================================================
 * Test helpers
 * ============================================================ */

static void enqueue_int(PL_Queue *queue, int value) {
  CU_ASSERT_PTR_NOT_NULL_FATAL(queue);

  bool result = queue->enqueue(queue, TEST_INT_TYPE, &value);

  CU_ASSERT_TRUE(result);
}

static int dequeue_int(PL_Queue *queue) {
  PL_Value value = queue->dequeue(queue);

  CU_ASSERT_EQUAL(value.vtype, TEST_INT_TYPE);

  int result = value.as.ival;

  /*
   * Safe for integer values.
   * Required for string values because dequeue transfers ownership.
   */
  pl_free_value_data(value);

  return result;
}

static void assert_queue_empty(PL_Queue *queue) {
  CU_ASSERT_TRUE(queue->is_empty(queue));
  CU_ASSERT_EQUAL(queue->length(queue), 0);
  CU_ASSERT_PTR_NULL(queue->peek(queue));
}


/* ============================================================
 * 1. Basic logical tests
 * ============================================================ */

static void test_queue_initial_state(void) {
  PL_Queue *queue = pl_queue_init();

  CU_ASSERT_PTR_NOT_NULL(queue);
  CU_ASSERT_PTR_NOT_NULL(queue->_state);
  CU_ASSERT_TRUE(queue->is_empty(queue));
  CU_ASSERT_EQUAL(queue->length(queue), 0);
  CU_ASSERT_PTR_NULL(queue->peek(queue));

  pl_queue_free(&queue);

  CU_ASSERT_PTR_NULL(queue);
}

static void test_enqueue_and_dequeue_single_value(void) {
  PL_Queue *queue = pl_queue_init();

  enqueue_int(queue, 12345);

  CU_ASSERT_FALSE(queue->is_empty(queue));
  CU_ASSERT_EQUAL(queue->length(queue), 1);

  CU_ASSERT_EQUAL(dequeue_int(queue), 12345);

  assert_queue_empty(queue);

  pl_queue_free(&queue);
}

static void test_fifo_order(void) {
  PL_Queue *queue = pl_queue_init();

  enqueue_int(queue, 10);
  enqueue_int(queue, 20);
  enqueue_int(queue, 30);

  CU_ASSERT_EQUAL(queue->length(queue), 3);

  CU_ASSERT_EQUAL(dequeue_int(queue), 10);
  CU_ASSERT_EQUAL(dequeue_int(queue), 20);
  CU_ASSERT_EQUAL(dequeue_int(queue), 30);

  assert_queue_empty(queue);

  pl_queue_free(&queue);
}

static void test_peek_does_not_remove_value(void) {
  PL_Queue *queue = pl_queue_init();

  enqueue_int(queue, 77);

  const PL_Value *value = queue->peek(queue);

  CU_ASSERT_PTR_NOT_NULL(value);
  CU_ASSERT_EQUAL(value->vtype, TEST_INT_TYPE);
  CU_ASSERT_EQUAL(value->as.ival, 77);

  /*
   * Peek must not remove the value.
   */
  CU_ASSERT_EQUAL(queue->length(queue), 1);
  CU_ASSERT_FALSE(queue->is_empty(queue));

  CU_ASSERT_EQUAL(dequeue_int(queue), 77);

  pl_queue_free(&queue);
}


/* ============================================================
 * 2. Edge-case tests
 * ============================================================ */

static void test_dequeue_empty_queue(void) {
  PL_Queue *queue = pl_queue_init();

  PL_Value value = queue->dequeue(queue);

  /*
   * Your current API uses a zero-initialized PL_Value to indicate
   * failure. This is inherently ambiguous if zero is valid data.
   */
  CU_ASSERT_EQUAL(value.vtype, 0);
  CU_ASSERT_EQUAL(queue->length(queue), 0);
  CU_ASSERT_TRUE(queue->is_empty(queue));

  pl_queue_free(&queue);
}

static void test_peek_empty_queue(void) {
  PL_Queue *queue = pl_queue_init();

  CU_ASSERT_PTR_NULL(queue->peek(queue));

  pl_queue_free(&queue);
}

static void test_null_arguments(void) {
  PL_Queue *queue = pl_queue_init();
  int value = 42;

  CU_ASSERT_FALSE(queue->enqueue(NULL, TEST_INT_TYPE, &value));
  CU_ASSERT_FALSE(queue->enqueue(queue, TEST_INT_TYPE, NULL));

  PL_Value popped = queue->dequeue(NULL);

  CU_ASSERT_EQUAL(popped.vtype, 0);
  CU_ASSERT_PTR_NULL(queue->peek(NULL));
  CU_ASSERT_EQUAL(queue->length(NULL), 0);
  CU_ASSERT_TRUE(queue->is_empty(NULL));

  pl_queue_free(NULL);
  pl_queue_free(&queue);
}

static void test_free_null_queue(void) {
  PL_Queue *queue = NULL;

  pl_queue_free(&queue);
  CU_ASSERT_PTR_NULL(queue);

  pl_queue_free(NULL);
}

static void test_negative_and_large_integers(void) {
  PL_Queue *queue = pl_queue_init();

  int values[] = {
    -1,
    0,
    1,
    -1000000,
    1000000
  };

  size_t count = sizeof(values) / sizeof(values[0]);

  for (size_t i = 0; i < count; ++i) {
    enqueue_int(queue, values[i]);
  }

  for (size_t i = 0; i < count; ++i) {
    CU_ASSERT_EQUAL(dequeue_int(queue), values[i]);
  }

  assert_queue_empty(queue);

  pl_queue_free(&queue);
}


/* ============================================================
 * 3. Circular-buffer wraparound tests
 * ============================================================ */

static void test_circular_wraparound(void) {
  PL_Queue *queue = pl_queue_init();

  for (int round = 0; round < 1000; ++round) {
    enqueue_int(queue, round);

    CU_ASSERT_EQUAL(queue->length(queue), 1);
    CU_ASSERT_EQUAL(dequeue_int(queue), round);

    CU_ASSERT_TRUE(queue->is_empty(queue));
  }

  pl_queue_free(&queue);
}

static void test_wraparound_preserves_fifo_order(void) {
  PL_Queue *queue = pl_queue_init();

  /*
   * Fill the queue with initial values.
   */
  for (int i = 0; i < 10; ++i) {
    enqueue_int(queue, i);
  }

  /*
   * Remove some values.
   */
  for (int i = 0; i < 5; ++i) {
    CU_ASSERT_EQUAL(dequeue_int(queue), i);
  }

  /*
   * Add more values. This should cause rear to wrap.
   */
  for (int i = 10; i < 20; ++i) {
    enqueue_int(queue, i);
  }

  /*
   * Verify FIFO order across the wrapped region.
   */
  for (int i = 5; i < 20; ++i) {
    CU_ASSERT_EQUAL(dequeue_int(queue), i);
  }

  assert_queue_empty(queue);

  pl_queue_free(&queue);
}


/* ============================================================
 * 4. Resize tests
 * ============================================================ */

static void test_resize_preserves_fifo_order(void) {
  PL_Queue *queue = pl_queue_init();

  enum {
    COUNT = 10000
  };

  for (int i = 0; i < COUNT; ++i) {
    enqueue_int(queue, i);
  }

  CU_ASSERT_EQUAL(queue->length(queue), COUNT);

  for (int i = 0; i < COUNT; ++i) {
    CU_ASSERT_EQUAL(dequeue_int(queue), i);
  }

  assert_queue_empty(queue);

  pl_queue_free(&queue);
}

static void test_resize_after_wraparound(void) {
  PL_Queue *queue = pl_queue_init();

  /*
   * Move front and rear around the circular buffer.
   */
  for (int i = 0; i < 100; ++i) {
    enqueue_int(queue, i);
    CU_ASSERT_EQUAL(dequeue_int(queue), i);
  }

  /*
   * Add enough values to force one or more resizes while the
   * logical queue may be wrapped in the old allocation.
   */
  for (int i = 100; i < 10000; ++i) {
    enqueue_int(queue, i);
  }

  CU_ASSERT_EQUAL(queue->length(queue), 9900);

  for (int i = 100; i < 10000; ++i) {
    CU_ASSERT_EQUAL(dequeue_int(queue), i);
  }

  assert_queue_empty(queue);

  pl_queue_free(&queue);
}


/* ============================================================
 * 5. Aggressive mixed read/write test
 * ============================================================ */

static void test_aggressive_mixed_operations(void) {
  PL_Queue *queue = pl_queue_init();

  enum {
    OPERATIONS = 200000,
    REFERENCE_CAPACITY = OPERATIONS + 1
  };

  int *reference = malloc(sizeof(*reference) * REFERENCE_CAPACITY);

  CU_ASSERT_PTR_NOT_NULL_FATAL(reference);

  size_t reference_front = 0;
  size_t reference_length = 0;

  srand(1234567);

  for (size_t operation = 0;
       operation < OPERATIONS;
       ++operation) {

    /*
     * Enqueue 60% of the time, except when the reference queue
     * is empty, in which case enqueue must be selected.
     */
    bool do_enqueue =
        reference_length == 0 ||
        (rand() % 100) < 60;

    if (do_enqueue) {
      int value = (int)operation;

      bool result =
          queue->enqueue(queue, TEST_INT_TYPE, &value);

      CU_ASSERT_TRUE(result);

      size_t index =
          (reference_front + reference_length) %
          REFERENCE_CAPACITY;

      reference[index] = value;
      ++reference_length;
    } else {
      int expected = reference[reference_front];
      int actual = dequeue_int(queue);

      CU_ASSERT_EQUAL(actual, expected);

      reference_front =
          (reference_front + 1) %
          REFERENCE_CAPACITY;

      --reference_length;
    }

    CU_ASSERT_EQUAL(queue->length(queue), reference_length);
    CU_ASSERT_EQUAL(queue->is_empty(queue),
                    reference_length == 0);

    if (reference_length > 0) {
      const PL_Value *front = queue->peek(queue);

      CU_ASSERT_PTR_NOT_NULL(front);
      CU_ASSERT_EQUAL(front->vtype, TEST_INT_TYPE);
      CU_ASSERT_EQUAL(front->as.ival,
                      reference[reference_front]);
    } else {
      CU_ASSERT_PTR_NULL(queue->peek(queue));
    }
  }

  /*
   * Empty both queues and compare the remaining values.
   */
  while (reference_length > 0) {
    CU_ASSERT_EQUAL(dequeue_int(queue),
                    reference[reference_front]);

    reference_front =
        (reference_front + 1) %
        REFERENCE_CAPACITY;

    --reference_length;
  }

  assert_queue_empty(queue);

  free(reference);
  pl_queue_free(&queue);
}


/* ============================================================
 * 6. Memory stress test
 * ============================================================ */

static void test_memory_stress(void) {
  PL_Queue *queue = pl_queue_init();

  enum {
    ROUNDS = 20,
    ITEMS_PER_ROUND = 100000
  };

  for (int round = 0; round < ROUNDS; ++round) {
    for (int i = 0; i < ITEMS_PER_ROUND; ++i) {
      int value = round * ITEMS_PER_ROUND + i;

      CU_ASSERT_TRUE(
          queue->enqueue(queue, TEST_INT_TYPE, &value)
      );
    }

    CU_ASSERT_EQUAL(queue->length(queue),
                    ITEMS_PER_ROUND);

    for (int i = 0; i < ITEMS_PER_ROUND; ++i) {
      int expected = round * ITEMS_PER_ROUND + i;

      CU_ASSERT_EQUAL(dequeue_int(queue), expected);
    }

    CU_ASSERT_TRUE(queue->is_empty(queue));
    CU_ASSERT_EQUAL(queue->length(queue), 0);
  }

  pl_queue_free(&queue);
}


/* ============================================================
 * CUnit registration and main
 * ============================================================ */

int main(void) {
  if (CU_initialize_registry() != CUE_SUCCESS) {
    return CU_get_error();
  }

  CU_pSuite suite =
      CU_add_suite("Circular Queue Tests", NULL, NULL);

  if (suite == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_add_test(suite, "queue initial state", test_queue_initial_state);
  CU_add_test(suite, "enqueue and dequeue single value", test_enqueue_and_dequeue_single_value);
  CU_add_test(suite, "FIFO order", test_fifo_order);

  CU_add_test(suite, "peek does not remove value",
      test_peek_does_not_remove_value
  );

  CU_add_test(
      suite,
      "dequeue empty queue",
      test_dequeue_empty_queue
  );

  CU_add_test(
      suite,
      "peek empty queue",
      test_peek_empty_queue
  );

  CU_add_test(
      suite,
      "null arguments",
      test_null_arguments
  );

  CU_add_test(
      suite,
      "free null queue",
      test_free_null_queue
  );

  CU_add_test(
      suite,
      "negative and large integers",
      test_negative_and_large_integers
  );

  CU_add_test(
      suite,
      "circular wraparound",
      test_circular_wraparound
  );

  CU_add_test(
      suite,
      "wraparound preserves FIFO order",
      test_wraparound_preserves_fifo_order
  );

  CU_add_test(
      suite,
      "resize preserves FIFO order",
      test_resize_preserves_fifo_order
  );

  CU_add_test(
      suite,
      "resize after wraparound",
      test_resize_after_wraparound
  );

  CU_add_test(
      suite,
      "aggressive mixed operations",
      test_aggressive_mixed_operations
  );

  CU_add_test(
      suite,
      "memory stress",
      test_memory_stress
  );

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();

  unsigned failures = CU_get_number_of_failures();

  CU_cleanup_registry();

  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
