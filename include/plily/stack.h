#pragma once

/**
 * @file stack.h
 * @brief Stack (Using Dynamic Array)
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

#include "plily/common.h"

#define INITIAL_STACK_CAPACITY  4



/// Forward declaration: Struct to represent the state of the Stack
/// This struct is opaque to the consumers to prevent modifying the states
typedef struct _StackState _StackState;


/// Representation of Stack struct
typedef struct PL_Stack {
  // Push a value to stack
  bool (* push)(struct PL_Stack *self, PL_VType vtype, const void *data);

  // Pop a value from stack
  // Caller is expected to free the returned PL_Value; use pl_free_value_data()
  PL_Value (* pop)(struct PL_Stack *self);

  // Peek at the value at top of stack
  // Returned PL_Value is snapshop of value in Stack: Caller must not free/modify it
  PL_Value (* peek)(struct PL_Stack *self);

  // No of Value in the stack
  int (* length)(struct PL_Stack *self);

  // Is the stack empty?
  bool (* is_empty)(struct PL_Stack *self);

  _StackState *_state;    ///< Internal struct to hold state of the State
} PL_Stack;



// ========================
// Function Declarations
// ========================
/**
 * @brief Allocate memory for PL_Stack struct and initialize with values
 * @return PL_Stack: reference to newly created PL_List struct
 */
PL_Stack* pl_stack_init(void);

/**
 * @breif Free the Stack along will all the Values in it
 * @param **self - reference of reference to PL_Stack
 */
void pl_stack_free(PL_Stack **self);
