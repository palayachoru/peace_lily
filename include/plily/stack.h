#pragma once

/**
 * @file stack.h
 * @brief Stack (Using Dynamic Array & Linked List)
 *
 * top == 0 | stack empty
 * top always points at the vacant space
 *
 * top == capacity | stack full
 * if so, then resize (double the size of stack)
 *
 * if stack shrinks to 25% of it's total capacity, then shrink it by half
 */

#include <stdbool.h>
#include <stddef.h>

#include <plily/common.h>

#define INITIAL_STACK_CAPACITY  4



/// Forward declaration: Struct to represent the state of the Stack
/// This struct is opaque to the consumers to prevent modifying the states
typedef struct _StackState _StackState;
typedef struct _StackLLState _StackLLState;


/// Representation of Stack struct (Implemented using Dynamic Array)
typedef struct PL_Stack {
  /// Push a value to stack
  bool (* push)(struct PL_Stack *self, PL_VType vtype, const void *data);

  /// Pop a value from stack
  /// Caller is expected to free the returned PL_Value; use pl_free_value_data()
  PL_Value (* pop)(struct PL_Stack *self);

  /// Peek at the value at top of stack
  /// Returned PL_Value is snapshop of value in Stack: Caller must not free/modify it
  PL_Value (* peek)(struct PL_Stack *self);

  /// No of Value in the stack
  int (* length)(struct PL_Stack *self);

  /// Is the stack empty?
  bool (* is_empty)(struct PL_Stack *self);

  _StackState *_state;    ///< Internal struct to hold state of the Stack
} PL_Stack;


/// Representation of Stack Struct (Implemented using LinkedList)
typedef struct PL_StackLL {
  /// Push a value to stack
  bool (* push)(struct PL_StackLL *self, PL_VType vtype, const void *data);

  /// Pop a value from stack
  /// Caller is expected to free the returned PL_Value; use pl_free_value_data()
  PL_Value (* pop)(struct PL_StackLL *self);

  /// Peek at the value at top of stack
  /// Returned PL_Value is snapshop of value in Stack: Caller must not free/modify it
  PL_Value (* peek)(struct PL_StackLL *self);

  /// No of Value in the stack
  int (* length)(struct PL_StackLL *self);

  /// Is the stack empty?
  bool (* is_empty)(struct PL_StackLL *self);

  _StackLLState *_state;    ///< Internal struct to hold state of the StackLL
} PL_StackLL;



// ========================
// Function Declarations
// ========================
/**
 * @brief Allocate memory for PL_Stack struct and initialize with values
 * @return PL_Stack: reference to newly created PL_Stack struct
 */
PL_Stack* pl_stack_init(void);

/**
 * @breif Free the Stack along will all the Values in it
 * @param **self - reference of reference to PL_Stack
 */
void pl_stack_free(PL_Stack **self);


/**
 * @brief Allocate memory for PL_StackLL struct and initialize with values
 * @return PL_StackLL: reference to newly created PL_StackLL struct
 */
PL_StackLL* pl_stackll_init(void);

/**
 * @breif Free the Stack along will all the Values in it
 * @param **self - reference of reference to PL_StackLL
 */
void pl_stackll_free(PL_StackLL **self);
