#pragma once

/**
 * @file queue.h
 * @brief Implementation of Queue (Dynamic array & LinkedList)
 *
 */

#include <stdbool.h>

#include <plily/common.h>

# define INITIAL_QUEUE_CAPACITY  4



/// Forward declaration: Struct to represent the state of the Queue
/// This struct is opaque to the consumers to prevent modifying the states
typedef struct _QueueState _QueueState;
typedef struct _QueueLLState _QueueLLState;


/// Representation of Queue struct (Implementation using Circular Array)
typedef struct PL_Queue {
  // Push a value to queue
  bool (* enqueue)(struct PL_Queue *self, PL_VType vtype, const void *data);

  /// Pop an value from queue
  /// Caller is expected to free the returned PL_Value; use pl_free_value_data()
  PL_Value (* dequeue)(struct PL_Queue *self);

  /// Peek at the value at front of queue
  /// Returned PL_Value is snapshop of value in Stack: Caller must not free/modify it
  const PL_Value* (* peek)(struct PL_Queue *self);

  /// Sizze of Queue
  int (* length)(struct PL_Queue *self);

  /// Is queue empty
  bool (* is_empty)(struct PL_Queue *self);

  _QueueState *_state;   ///< Internal struct to hold state of the Queue
} PL_Queue;


/// Representation of Queue struct (Implementation using Linkedlist)
typedef struct PL_QueueLL {
  /// Push a value to queue
  bool (* enqueue)(struct PL_QueueLL *self, PL_VType vtype, const void *data);

  /// Pop an value from queue
  /// Caller is expected to free the returned PL_Value; use pl_free_value_data()
  PL_Value (* dequeue)(struct PL_QueueLL *self);

  /// Peek at the value at front of queue
  /// Returned PL_Value is snapshop of value in Stack: Caller must not free/modify it
  PL_Value (* peek)(struct PL_QueueLL *self);

  /// No of values in Queue
  int (* length)(struct PL_QueueLL *self);

  /// Is queue empty
  bool (* is_empty)(struct PL_QueueLL *self);

  _QueueLLState *_state;   ///< Internal struct to hold state of the QueueLL
} PL_QueueLL;



// ========================
// Function Declarations
// ========================
/**
 * @brief Allocate memory for PL_QueueLL struct and initialize with values
 * @return PL_QueueLL: reference to newly created PL_Queue struct
 */
PL_QueueLL* pl_queuell_init(void);

/**
 * @breif Free the Queue along will all the Values in it
 * @param **self - reference of reference to PL_QueueLL
 */
void pl_queuell_free(PL_QueueLL **self);


/**
 * @brief Allocate memory for PL_Queue struct and initialize with values
 * @return PL_Queue: reference to newly created PL_Queue struct
 */
PL_Queue* pl_queue_init(void);

/**
 * @breif Free the Queue along will all the Values in it
 * @param **self - reference of reference to PL_Queue
 */
void pl_queue_free(PL_Queue **self);
