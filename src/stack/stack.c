#define _POSIX_C_SOURCE 200809L   // Enables declarations for POSIX functions and symbols

#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <plily/stack.h>
#include <plily/common.h>

#include "internal.h"



/// Struct to represent the internal state of the Stack
/// This struct's members will be opaque to the consumers to prevent from editing
typedef struct _StackState {
  PL_Value *arr;       ///< Array of PL_Value structs
  size_t top;          ///< Represent the top Value at the stack
  size_t capacity;     ///< No of Values the list can hold
} _StackState;


// =====================
// Forward declarations
// =====================
static bool push(Stack *self, VType vtype, const void *data);

static Value pop(Stack *self);

static Value peek(Stack *self);

static int length(Stack *self);

static bool is_empty(Stack *self);

// UTILITY FUNCTION
static bool is_shrink_limit_hit(Stack *st);

static bool is_resize_limit_hit(Stack *st);

static bool resize(Stack *st);

static bool shrink(Stack *st);



// =====================
// Public API
// =====================
Stack* pl_stack_init(void) {
  Stack *st = calloc(1, sizeof(Stack));
  if (!st) return NULL;

  // Initialize the StackState struct
  st->_state = calloc(1, sizeof(_StackState));
  if (!st->_state) {
    free(st);
    return NULL;
  }

  // Initialize the array
  st->_state->arr = calloc(INITIAL_STACK_CAPACITY, sizeof(Value));
  if (!st->_state->arr) {
    free(st->_state);
    free(st);
    return NULL;
  }

  // Initialize the default values
  st->_state->capacity = INITIAL_STACK_CAPACITY;
  st->_state->top = 0;

  // Initialize the function pointers
  st->push = push;
  st->pop = pop;
  st->peek = peek;
  st->length = length;
  st->is_empty = is_empty;

  return st;
}


void pl_stack_free(PL_Stack **self) {
  if (!self || !(*self)) return;

  Stack *st = *self;

  // if the Value struct contains string, then free it
  for (size_t i = 0; i < st->_state->top; i++) {
    if (st->_state->arr[i].vtype == PL_STR)
      free(st->_state->arr[i].as.sval);
  }

  free(st->_state->arr);    // free the heap allocated array of Values
  free(st->_state);         // free the heap allocated StackState struct
  free(st);                 // finally free the Stack itself

  *self = NULL;
}



// ============================================
// Core Functions Implementation
// ============================================
static bool push(Stack *self, VType vtype, const void *data) {
  if (!self || !data) return false;

  // before pushing value, check if we need to resize
  if (is_resize_limit_hit(self) && !resize(self)) return false;

  // top always points to vacant space
  // so add value and then increment top
  if (!pl_update_value(&self->_state->arr[self->_state->top], vtype, data)) return false;

  self->_state->top++;
  return true;
}


static Value pop(Stack *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  // as top always points to vacant space
  // decrement the top first and free the memory
  size_t idx = --self->_state->top;
  Value popped = self->_state->arr[idx];

  // clear the data at the removed slot
  self->_state->arr[idx] = (Value){0};

  // shrink the stack, if no of elements is lower then 25%
  if (is_shrink_limit_hit(self)) shrink(self);

  // if Value contain string data, caller has to free it
  // use pl_free_node_data() to free the allocation
  return popped;
}


static Value peek(Stack *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  // The returned Value is snapshot of Value in stack: caller
  // is not recommended to free/edit the Value
  return (self->_state->arr[self->_state->top - 1]);
}


static int length(Stack *self) {
  return (self ? (int)self->_state->top : 0);
}


static bool is_empty(Stack *self) {
  return (self ? self->_state->top == 0 : false);
}



// ============================================
// Util Functions Implementation
// ============================================
static bool is_shrink_limit_hit(Stack *st) {
  if (!st) return false;

  // if capacity is at MIN, then not shrink the array (even array is empty)
  if (st->_state->capacity <= INITIAL_STACK_CAPACITY) return false;

  // shrink trigger: top < 25% of capacity
  return (st->_state->top < (st->_state->capacity / 4));
}


static bool is_resize_limit_hit(Stack *st) {
  return (st ? st->_state->top >= st->_state->capacity : false);
}


static bool resize(Stack *st) {
  if (!st) return false;

  size_t old_capacity = st->_state->capacity;
  size_t new_capacity = old_capacity * 2;

  Value *enlarged_arr = realloc(st->_state->arr, new_capacity * sizeof(Value));
  if (!enlarged_arr) return false;

  // clear the newly added memory block (to remove heap garbage)
  memset(&enlarged_arr[old_capacity], 0, (new_capacity - old_capacity) * sizeof(Value));

  // finally update the capacity & array
  st->_state->arr = enlarged_arr;
  st->_state->capacity = new_capacity;
  return true;
}


static bool shrink(Stack *st) {
  if (!st) return false;

  size_t old_capacity = st->_state->capacity;
  size_t new_capacity = old_capacity / 2;
  if (new_capacity < INITIAL_STACK_CAPACITY) new_capacity = INITIAL_STACK_CAPACITY;

  Value *shrinked_arr = realloc(st->_state->arr, new_capacity * sizeof(Value));
  if (!shrinked_arr) return false;

  // finally update the capacity & array
  st->_state->arr = shrinked_arr;
  st->_state->capacity = new_capacity;
  return true;
}
