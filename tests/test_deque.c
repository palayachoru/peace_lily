#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <plily/queue.h>

#define TEST_INT    PL_INT
#define TEST_STRING PL_STR

static int value_as_int(const PL_Value *value)
{
    /*
     * Change this according to your PL_Value definition.
     */
    return value->as.ival;
}

static const char *value_as_string(const PL_Value *value)
{
    /*
     * Change this according to your PL_Value definition.
     */
    return value->as.sval;
}

static PL_Value dequeue_int_front(PL_Deque *deque)
{
    int expected_dummy = 0;
    (void)expected_dummy;

    PL_Value value = deque->dequeue_front(deque);
    return value;
}

static PL_Value dequeue_int_rear(PL_Deque *deque)
{
    return deque->dequeue_rear(deque);
}

static void assert_dequeued_int_front(PL_Deque *deque, int expected)
{
    PL_Value value = dequeue_int_front(deque);

    CU_ASSERT_EQUAL(value_as_int(&value), expected);

    /*
     * If integer values do not own heap memory, this is harmless only if
     * your implementation supports it. Remove this call for non-owning ints.
     */
    pl_free_value_data(value);
}

static void assert_dequeued_int_rear(PL_Deque *deque, int expected)
{
    PL_Value value = dequeue_int_rear(deque);

    CU_ASSERT_EQUAL(value_as_int(&value), expected);

    /*
     * Remove this if integer values do not need cleanup.
     */
    pl_free_value_data(value);
}

static void assert_dequeued_string_front(PL_Deque *deque,
                                         const char *expected)
{
    PL_Value value = deque->dequeue_front(deque);

    CU_ASSERT_PTR_NOT_NULL(value_as_string(&value));
    CU_ASSERT_STRING_EQUAL(value_as_string(&value), expected);

    pl_free_value_data(value);
}

static void assert_dequeued_string_rear(PL_Deque *deque,
                                        const char *expected)
{
    PL_Value value = deque->dequeue_rear(deque);

    CU_ASSERT_PTR_NOT_NULL(value_as_string(&value));
    CU_ASSERT_STRING_EQUAL(value_as_string(&value), expected);

    pl_free_value_data(value);
}


/* ============================================================
 * 1. Logical tests
 * ============================================================
 */

static void test_init_state(void)
{
    PL_Deque *deque = pl_deque_init();

    CU_ASSERT_PTR_NOT_NULL(deque);
    CU_ASSERT_PTR_NOT_NULL(deque->_state);

    CU_ASSERT_TRUE(deque->is_empty(deque));
    CU_ASSERT_EQUAL(deque->length(deque), 0);

    pl_deque_free(&deque);

    CU_ASSERT_PTR_NULL(deque);
}

static void test_enqueue_rear_and_dequeue_front(void)
{
    PL_Deque *deque = pl_deque_init();

    int a = 10;
    int b = 20;
    int c = 30;

    CU_ASSERT_TRUE(deque->enqueue_rear(deque, TEST_INT, &a));
    CU_ASSERT_TRUE(deque->enqueue_rear(deque, TEST_INT, &b));
    CU_ASSERT_TRUE(deque->enqueue_rear(deque, TEST_INT, &c));

    CU_ASSERT_EQUAL(deque->length(deque), 3);
    CU_ASSERT_FALSE(deque->is_empty(deque));

    assert_dequeued_int_front(deque, 10);
    assert_dequeued_int_front(deque, 20);
    assert_dequeued_int_front(deque, 30);

    CU_ASSERT_TRUE(deque->is_empty(deque));

    pl_deque_free(&deque);
}

static void test_enqueue_front_and_dequeue_rear(void)
{
    PL_Deque *deque = pl_deque_init();

    int a = 10;
    int b = 20;
    int c = 30;

    CU_ASSERT_TRUE(deque->enqueue_front(deque, TEST_INT, &a));
    CU_ASSERT_TRUE(deque->enqueue_front(deque, TEST_INT, &b));
    CU_ASSERT_TRUE(deque->enqueue_front(deque, TEST_INT, &c));

    CU_ASSERT_EQUAL(deque->length(deque), 3);

    assert_dequeued_int_rear(deque, 10);
    assert_dequeued_int_rear(deque, 20);
    assert_dequeued_int_rear(deque, 30);

    CU_ASSERT_TRUE(deque->is_empty(deque));

    pl_deque_free(&deque);
}

static void test_mixed_operations(void)
{
    PL_Deque *deque = pl_deque_init();

    int one = 1;
    int two = 2;
    int three = 3;
    int four = 4;

    CU_ASSERT_TRUE(deque->enqueue_rear(deque, TEST_INT, &one));
    CU_ASSERT_TRUE(deque->enqueue_rear(deque, TEST_INT, &two));
    CU_ASSERT_TRUE(deque->enqueue_front(deque, TEST_INT, &three));
    CU_ASSERT_TRUE(deque->enqueue_front(deque, TEST_INT, &four));

    /*
     * Logical order is: 4, 3, 1, 2
     */
    CU_ASSERT_EQUAL(value_as_int(deque->peek_front(deque)), 4);
    CU_ASSERT_EQUAL(value_as_int(deque->peek_rear(deque)), 2);

    assert_dequeued_int_front(deque, 4);
    assert_dequeued_int_rear(deque, 2);
    assert_dequeued_int_front(deque, 3);
    assert_dequeued_int_rear(deque, 1);

    CU_ASSERT_TRUE(deque->is_empty(deque));

    pl_deque_free(&deque);
}

static void test_peek_does_not_remove(void)
{
    PL_Deque *deque = pl_deque_init();

    int value = 42;

    CU_ASSERT_TRUE(deque->enqueue_rear(deque, TEST_INT, &value));

    const PL_Value *front = deque->peek_front(deque);
    const PL_Value *rear = deque->peek_rear(deque);

    CU_ASSERT_PTR_NOT_NULL(front);
    CU_ASSERT_PTR_NOT_NULL(rear);
    CU_ASSERT_EQUAL(value_as_int(front), 42);
    CU_ASSERT_EQUAL(value_as_int(rear), 42);
    CU_ASSERT_EQUAL(deque->length(deque), 1);

    assert_dequeued_int_front(deque, 42);

    pl_deque_free(&deque);
}

static void test_resize_preserves_order(void)
{
    PL_Deque *deque = pl_deque_init();

    enum { COUNT = 1000 };

    for (int i = 0; i < COUNT; ++i) {
        CU_ASSERT_TRUE(
            deque->enqueue_rear(deque, TEST_INT, &i)
        );
    }

    CU_ASSERT_EQUAL(deque->length(deque), COUNT);

    for (int i = 0; i < COUNT; ++i) {
        PL_Value value = deque->dequeue_front(deque);

        CU_ASSERT_EQUAL(value_as_int(&value), i);
        pl_free_value_data(value);
    }

    CU_ASSERT_TRUE(deque->is_empty(deque));

    pl_deque_free(&deque);
}

static void test_resize_preserves_wrapped_order(void)
{
    PL_Deque *deque = pl_deque_init();

    int value;

    /*
     * Create wrapping before resizing.
     */
    for (int i = 0; i < 5; ++i) {
        value = i;
        CU_ASSERT_TRUE(
            deque->enqueue_rear(deque, TEST_INT, &value)
        );
    }

    for (int i = 0; i < 3; ++i) {
        PL_Value popped = deque->dequeue_front(deque);
        pl_free_value_data(popped);
    }

    for (int i = 5; i < 30; ++i) {
        value = i;
        CU_ASSERT_TRUE(
            deque->enqueue_rear(deque, TEST_INT, &value)
        );
    }

    /*
     * Remaining logical order should be:
     * 3, 4, 5, 6, ..., 29
     */
    for (int expected = 3; expected < 30; ++expected) {
        PL_Value popped = deque->dequeue_front(deque);

        CU_ASSERT_EQUAL(value_as_int(&popped), expected);
        pl_free_value_data(popped);
    }

    CU_ASSERT_TRUE(deque->is_empty(deque));

    pl_deque_free(&deque);
}


/* ============================================================
 * 2. Edge-case tests
 * ============================================================
 */

static void test_empty_deque_operations(void)
{
    PL_Deque *deque = pl_deque_init();

    PL_Value front_value = deque->dequeue_front(deque);
    PL_Value rear_value = deque->dequeue_rear(deque);

    CU_ASSERT_TRUE(deque->is_empty(deque));
    CU_ASSERT_EQUAL(deque->length(deque), 0);
    CU_ASSERT_PTR_NULL(deque->peek_front(deque));
    CU_ASSERT_PTR_NULL(deque->peek_rear(deque));

    pl_deque_free(&deque);
}

static void test_single_element_all_operations(void)
{
    PL_Deque *deque = pl_deque_init();

    int value = 99;

    CU_ASSERT_TRUE(deque->enqueue_rear(deque, TEST_INT, &value));
    CU_ASSERT_EQUAL(deque->length(deque), 1);
    CU_ASSERT_EQUAL(value_as_int(deque->peek_front(deque)), 99);
    CU_ASSERT_EQUAL(value_as_int(deque->peek_rear(deque)), 99);

    assert_dequeued_int_front(deque, 99);
    CU_ASSERT_TRUE(deque->is_empty(deque));

    CU_ASSERT_TRUE(deque->enqueue_front(deque, TEST_INT, &value));
    assert_dequeued_int_rear(deque, 99);

    CU_ASSERT_TRUE(deque->is_empty(deque));

    pl_deque_free(&deque);
}

static void test_null_arguments(void)
{
    int value = 123;

    PL_Deque *deque = pl_deque_init();

    CU_ASSERT_PTR_NOT_NULL_FATAL(deque);

    /*
     * Test NULL as the deque argument.
     */
    CU_ASSERT_FALSE(
        deque->enqueue_rear(NULL, TEST_INT, &value)
    );

    CU_ASSERT_FALSE(
        deque->enqueue_front(NULL, TEST_INT, &value)
    );

    CU_ASSERT_PTR_NULL(
        deque->peek_front(NULL)
    );

    CU_ASSERT_PTR_NULL(
        deque->peek_rear(NULL)
    );

    PL_Value front_value = deque->dequeue_front(NULL);
    PL_Value rear_value = deque->dequeue_rear(NULL);


    /*
     * Test NULL data using a valid deque.
     */
    CU_ASSERT_FALSE(
        deque->enqueue_rear(deque, TEST_INT, NULL)
    );

    CU_ASSERT_FALSE(
        deque->enqueue_front(deque, TEST_INT, NULL)
    );

    CU_ASSERT_TRUE(deque->is_empty(deque));
    CU_ASSERT_EQUAL(deque->length(deque), 0);

    /*
     * Free the allocation created by pl_deque_init().
     */
    pl_deque_free(&deque);

    CU_ASSERT_PTR_NULL(deque);
}


static void test_null_data(void)
{
    PL_Deque *deque = pl_deque_init();

    CU_ASSERT_FALSE(
        deque->enqueue_rear(deque, TEST_INT, NULL)
    );

    CU_ASSERT_FALSE(
        deque->enqueue_front(deque, TEST_INT, NULL)
    );

    CU_ASSERT_TRUE(deque->is_empty(deque));
    CU_ASSERT_EQUAL(deque->length(deque), 0);

    pl_deque_free(&deque);
}

static void test_full_capacity_causes_resize(void)
{
    PL_Deque *deque = pl_deque_init();

    size_t old_capacity = INITIAL_QUEUE_CAPACITY;
    size_t values_to_insert = old_capacity * 4;

    for (size_t i = 0; i < values_to_insert; ++i) {
        int value = (int)i;

        CU_ASSERT_TRUE(
            deque->enqueue_rear(deque, TEST_INT, &value)
        );
    }

    CU_ASSERT_EQUAL(deque->length(deque), values_to_insert);

    for (size_t i = 0; i < values_to_insert; ++i) {
        PL_Value value = deque->dequeue_front(deque);

        CU_ASSERT_EQUAL(value_as_int(&value), (int)i);
        pl_free_value_data(value);
    }

    pl_deque_free(&deque);
}

static void test_string_values_are_freed_correctly(void)
{
    PL_Deque *deque = pl_deque_init();

    const char *strings[] = {
        "alpha",
        "beta",
        "gamma",
        "delta",
        "epsilon"
    };

    for (size_t i = 0; i < 5; ++i) {
        CU_ASSERT_TRUE(
            deque->enqueue_rear(
                deque,
                TEST_STRING,
                strings[i]
            )
        );
    }

    assert_dequeued_string_front(deque, "alpha");
    assert_dequeued_string_rear(deque, "epsilon");
    assert_dequeued_string_front(deque, "beta");
    assert_dequeued_string_rear(deque, "delta");
    assert_dequeued_string_front(deque, "gamma");

    CU_ASSERT_TRUE(deque->is_empty(deque));

    pl_deque_free(&deque);
}


/* ============================================================
 * 3. Aggressive read/write tests
 * ============================================================
 */

static void test_aggressive_alternating_operations(void)
{
    PL_Deque *deque = pl_deque_init();

    enum { OPERATIONS = 100000 };

    for (int i = 0; i < OPERATIONS; ++i) {
        int value = i;

        if ((i % 4) == 0) {
            CU_ASSERT_TRUE(
                deque->enqueue_front(deque, TEST_INT, &value)
            );
        } else {
            CU_ASSERT_TRUE(
                deque->enqueue_rear(deque, TEST_INT, &value)
            );
        }

        if ((i % 7) == 0 && !deque->is_empty(deque)) {
            PL_Value popped = deque->dequeue_front(deque);
            pl_free_value_data(popped);
        }

        if ((i % 11) == 0 && !deque->is_empty(deque)) {
            PL_Value popped = deque->dequeue_rear(deque);
            pl_free_value_data(popped);
        }

        CU_ASSERT_TRUE(deque->length(deque) >= 0);
    }

    while (!deque->is_empty(deque)) {
        PL_Value popped;

        if (deque->length(deque) % 2 == 0) {
            popped = deque->dequeue_front(deque);
        } else {
            popped = deque->dequeue_rear(deque);
        }

        pl_free_value_data(popped);
    }

    CU_ASSERT_EQUAL(deque->length(deque), 0);
    CU_ASSERT_TRUE(deque->is_empty(deque));

    pl_deque_free(&deque);
}

static void test_aggressive_string_insert_remove(void)
{
    PL_Deque *deque = pl_deque_init();

    enum { OPERATIONS = 20000 };

    for (int i = 0; i < OPERATIONS; ++i) {
        char buffer[64];

        snprintf(buffer, sizeof(buffer), "value-%d", i);

        if ((i % 2) == 0) {
            CU_ASSERT_TRUE(
                deque->enqueue_front(deque, TEST_STRING, buffer)
            );
        } else {
            CU_ASSERT_TRUE(
                deque->enqueue_rear(deque, TEST_STRING, buffer)
            );
        }

        if ((i % 3) == 0 && !deque->is_empty(deque)) {
            PL_Value popped = deque->dequeue_front(deque);
            pl_free_value_data(popped);
        }

        if ((i % 5) == 0 && !deque->is_empty(deque)) {
            PL_Value popped = deque->dequeue_rear(deque);
            pl_free_value_data(popped);
        }
    }

    while (!deque->is_empty(deque)) {
        PL_Value popped = deque->dequeue_front(deque);
        pl_free_value_data(popped);
    }

    pl_deque_free(&deque);
}

static void test_repeated_wraparound(void)
{
    PL_Deque *deque = pl_deque_init();

    enum { ROUNDS = 10000 };

    for (int round = 0; round < ROUNDS; ++round) {
        int a = round;
        int b = -round;

        CU_ASSERT_TRUE(
            deque->enqueue_rear(deque, TEST_INT, &a)
        );

        CU_ASSERT_TRUE(
            deque->enqueue_front(deque, TEST_INT, &b)
        );

        assert_dequeued_int_front(deque, -round);
        assert_dequeued_int_rear(deque, round);

        CU_ASSERT_TRUE(deque->is_empty(deque));
    }

    pl_deque_free(&deque);
}


/* ============================================================
 * 4. Memory-stress tests
 *
 * Run these separately under AddressSanitizer or Valgrind.
 * ============================================================
 */

static void test_memory_stress_large_integer_deque(void)
{
    PL_Deque *deque = pl_deque_init();

    enum { COUNT = 1000000 };

    for (int i = 0; i < COUNT; ++i) {
        CU_ASSERT_TRUE(
            deque->enqueue_rear(deque, TEST_INT, &i)
        );
    }

    CU_ASSERT_EQUAL(deque->length(deque), COUNT);

    for (int i = 0; i < COUNT; ++i) {
        PL_Value value = deque->dequeue_front(deque);

        CU_ASSERT_EQUAL(value_as_int(&value), i);
        pl_free_value_data(value);
    }

    CU_ASSERT_TRUE(deque->is_empty(deque));

    pl_deque_free(&deque);
}

static void test_memory_stress_repeated_create_destroy(void)
{
    enum {
        ITERATIONS = 10000,
        VALUES_PER_DEQUE = 100
    };

    for (int iteration = 0; iteration < ITERATIONS; ++iteration) {
        PL_Deque *deque = pl_deque_init();

        CU_ASSERT_PTR_NOT_NULL_FATAL(deque);

        for (int i = 0; i < VALUES_PER_DEQUE; ++i) {
            char buffer[64];

            snprintf(
                buffer,
                sizeof(buffer),
                "iteration-%d-value-%d",
                iteration,
                i
            );

            CU_ASSERT_TRUE(
                deque->enqueue_rear(
                    deque,
                    TEST_STRING,
                    buffer
                )
            );
        }

        /*
         * pl_deque_free() must free every value still stored in the deque.
         */
        pl_deque_free(&deque);

        CU_ASSERT_PTR_NULL(deque);
    }
}

static void test_memory_stress_mixed_cleanup(void)
{
    PL_Deque *deque = pl_deque_init();

    enum { COUNT = 100000 };

    for (int i = 0; i < COUNT; ++i) {
        char buffer[64];

        snprintf(buffer, sizeof(buffer), "item-%d", i);

        if (i % 2 == 0) {
            CU_ASSERT_TRUE(
                deque->enqueue_front(deque, TEST_STRING, buffer)
            );
        } else {
            CU_ASSERT_TRUE(
                deque->enqueue_rear(deque, TEST_STRING, buffer)
            );
        }

        if (i % 4 == 0 && !deque->is_empty(deque)) {
            PL_Value value = deque->dequeue_front(deque);
            pl_free_value_data(value);
        }

        if (i % 9 == 0 && !deque->is_empty(deque)) {
            PL_Value value = deque->dequeue_rear(deque);
            pl_free_value_data(value);
        }
    }

    /*
     * Remaining values must be freed by pl_deque_free().
     */
    pl_deque_free(&deque);

    CU_ASSERT_PTR_NULL(deque);
}


/* ============================================================
 * Test runner
 * ============================================================
 */

static void register_deque_tests(void)
{
    CU_pSuite suite = CU_add_suite(
        "PL Deque Tests",
        NULL,
        NULL
    );

    if (!suite) {
        return;
    }

    /* Logical tests */
    CU_add_test(suite, "Initialization", test_init_state);
    CU_add_test(suite, "Rear enqueue/front dequeue",
                test_enqueue_rear_and_dequeue_front);
    CU_add_test(suite, "Front enqueue/rear dequeue",
                test_enqueue_front_and_dequeue_rear);
    CU_add_test(suite, "Mixed operations", test_mixed_operations);
    CU_add_test(suite, "Peek does not remove",
                test_peek_does_not_remove);
    CU_add_test(suite, "Resize preserves order",
                test_resize_preserves_order);
    CU_add_test(suite, "Resize preserves wrapped order",
                test_resize_preserves_wrapped_order);

    /* Edge-case tests */
    CU_add_test(suite, "Empty deque operations",
                test_empty_deque_operations);
    CU_add_test(suite, "Single element operations",
                test_single_element_all_operations);
    CU_add_test(suite, "NULL arguments",
                test_null_arguments);
    CU_add_test(suite, "NULL data",
                test_null_data);
    CU_add_test(suite, "Full capacity causes resize",
                test_full_capacity_causes_resize);
    CU_add_test(suite, "String values",
                test_string_values_are_freed_correctly);

    /* Aggressive tests */
    CU_add_test(suite, "Aggressive alternating operations",
                test_aggressive_alternating_operations);
    CU_add_test(suite, "Aggressive string operations",
                test_aggressive_string_insert_remove);
    CU_add_test(suite, "Repeated wraparound",
                test_repeated_wraparound);

    /* Memory-stress tests */
    CU_add_test(suite, "Large integer deque",
                test_memory_stress_large_integer_deque);
    CU_add_test(suite, "Repeated create/destroy",
                test_memory_stress_repeated_create_destroy);
    CU_add_test(suite, "Mixed memory cleanup",
                test_memory_stress_mixed_cleanup);
}

int main(void)
{
    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    register_deque_tests();

    CU_basic_set_mode(CU_BRM_VERBOSE);

    CU_basic_run_tests();

    unsigned failed = CU_get_number_of_failures();

    CU_cleanup_registry();

    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
