#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <plily/queue.h>

#include "internal.h"

/**
 * front and rear starts at index 0
 *
 * for enqueqe_rear - add value and increment rear
 * for enqueue_front - decrement front and add value
 * for dequeue_front - remove value and increment front
 * for dequeue_rear - decrement rear and remove value
 *
 * empty queue: front == rear
 * full queue: rear + 1 == front
 */


/// Struct to represent the internal state of the Deque
/// This struct's members will be opaque to the consumers to prevent from editing
typedef struct _DequeState {
  Value *arr;       ///< Array of PL_Value struct
  size_t front;     ///< Pointer to front of queue
  size_t rear;      ///< Pointer to rear of queue
  size_t capacity;  ///< Capacity of the queue
} _DequeState;



// =====================
// Forward declarations
// =====================
static bool enqueue_rear(Deque *self, VType vtype, const void *data);

static Value dequeue_front(Deque *self);

static const Value* peek_front(Deque *self);


static bool enqueue_front(Deque *self, VType vtype, const void *data);

static Value dequeue_rear(Deque *self);

static const Value* peek_rear(Deque *self);


static int length(Deque *self);

static bool is_empty(Deque *self);

// UTIL FUNCTIONS
static inline bool is_full(Deque *q);

/// will decrement the index by one and warps if underflows (less than 0)
static inline size_t decrement_by_1(size_t idx, size_t capacity);

/// will increment the index by one and warps if overflows (high than capacity)
static inline size_t increment_by_1(size_t idx, size_t capacity);

static bool resize(Deque *q);



// =====================
// Public API
// =====================
Deque* pl_deque_init(void) {
  Deque *q = calloc(1, sizeof(Deque));
  if (!q) return NULL;

  // INitialize the QueueState
  q->_state = calloc(1, sizeof(_DequeState));
  if (!q->_state) {
    free(q);
    return NULL;
  }

  // Initialize the Array
  q->_state->arr = calloc(INITIAL_QUEUE_CAPACITY, sizeof(Value));
  if (!q->_state->arr) {
    free(q->_state);
    free(q);
    return NULL;
  }

  // Initialize the members with default values
  q->_state->front = 0;
  q->_state->rear = 0;
  q->_state->capacity = INITIAL_QUEUE_CAPACITY;

  // Initialize the function pointers
  q->enqueue_rear = enqueue_rear;
  q->dequeue_front = dequeue_front;
  q->peek_front = peek_front;
  q->enqueue_front = enqueue_front;
  q->dequeue_rear = dequeue_rear;
  q->peek_rear = peek_rear;
  q->length = length;
  q->is_empty = is_empty;

  return q;
}


void pl_deque_free(PL_Deque **self) {
  if (!self || !(*self)) return;

  Deque *q = (*self);
  size_t len = (size_t)q->length(q);
  size_t front = q->_state->front;
  size_t capacity = q->_state->capacity;

  // start from front's position and increment it until end of length
  for (size_t i = 0; i < len; i++) {
    size_t idx = (front + i) % capacity;

    pl_free_value_data(q->_state->arr[idx]);
  }

  free(q->_state->arr);
  free(q->_state);
  free(q);

  (*self) = NULL;
}



// ============================================
// Core Functions Implementation
// ============================================
static bool enqueue_rear(Deque *self, VType vtype, const void *data) {
  if (!self || !self->_state || !data) return false;

  if (is_full(self) && !resize(self)) return false;

  _DequeState *state = self->_state;

  // rear always points to vacant space, so add value and increment rear
  if (!pl_update_value(&state->arr[state->rear], vtype, data)) return false;

  state->rear = increment_by_1(state->rear, state->capacity);
  return true;
}


static bool enqueue_front(Deque *self, VType vtype, const void *data) {
  if (!self || !self->_state || !data) return false;

  if (is_full(self) && !resize(self)) return false;

  _DequeState *state = self->_state;

  // decrement front and add then add the value
  size_t new_front = decrement_by_1(state->front, state->capacity);

  if (!pl_update_value(&state->arr[new_front], vtype, data)) return false;

  state->front = new_front;
  return true;
}


static Value dequeue_front(Deque *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  _DequeState *state = self->_state;

  // copy of element to be removed
  Value popped = self->_state->arr[state->front];

  // clear the date at the removed spot
  self->_state->arr[state->front] = (Value){0};

  // update the front's index position (also handles wrapping)
  state->front = increment_by_1(state->front, state->capacity);

  // if Value contain string data, caller has to free it
  // use pl_free_node_data() to free the allocation
  return popped;
}


static Value dequeue_rear(Deque *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  _DequeState *state = self->_state;

  // decrement the rear and then remove the value
  state->rear = decrement_by_1(state->rear, state->capacity);

  // copy of element to be removed
  Value popped = self->_state->arr[state->rear];

  // clear the date at the removed spot
  self->_state->arr[state->rear] = (Value){0};

  // if Value contain string data, caller has to free it
  // use pl_free_node_data() to free the allocation
  return popped;
}


static const Value* peek_front(Deque *self) {
  if (!self || self->is_empty(self)) return NULL;

  // The returned Value is snapshot of Value in stack: caller
  // is not recommended to free/edit the Value
  return &self->_state->arr[self->_state->front];
}


static const Value* peek_rear(Deque *self) {
  if (!self || self->is_empty(self)) return NULL;

  // rear always points to vacant space
  // decrement rear pointer and return the value
  size_t rear_idx = decrement_by_1(self->_state->rear, self->_state->capacity);

  // The returned Value is snapshot of Value in stack: caller
  // is not recommended to free/edit the Value
  return &self->_state->arr[rear_idx];
}


static int length(Deque *self) {
  if (!self || !self->_state) return 0;

  _DequeState *state = self->_state;

  return (int)( (state->rear + state->capacity - state->front) % state->capacity );
}


static bool is_empty(Deque *self) {
  return (!self || !self->_state || self->_state->front == self->_state->rear);
}




// ============================================
// Util Functions Implementation
// ============================================
static bool is_full(Deque *q) {
  if (!q || !q->_state || q->_state->capacity < 2) return false;

  _DequeState *state = q->_state;

  return (state->rear + 1) % state->capacity == state->front ;
}

static inline size_t decrement_by_1(size_t idx, size_t capacity) {
  assert(capacity > 0);
  assert(idx < capacity);

  return idx == 0 ? capacity - 1 : idx - 1;
}


static inline size_t increment_by_1(size_t idx, size_t capacity) {
  assert(capacity > 0);
  assert(idx < capacity);

  return (idx + 1) % capacity;
}


static bool resize(Deque *q) {
  if (!q || !q->_state || q->_state->capacity < 2) return false;

  size_t old_cap = q->_state->capacity;
  size_t new_cap = old_cap * 2;

  // allocate a new array with double the size
  Value *new_arr = calloc(new_cap, sizeof(Value));
  if (!new_arr) return false;

  size_t len = (size_t)q->length(q);
  _DequeState *state = q->_state;

  size_t front2end = state->capacity - state->front;
  if (front2end > len) front2end = len;

  // copy: from front to end of the old array
  memcpy(new_arr, state->arr + state->front, sizeof(Value) * front2end);

  // copy: from beginning to element present
  // in case of not-warped: beginning to rear
  memcpy(new_arr + front2end, state->arr, sizeof(Value) * (len - front2end));

  free(state->arr);
  state->arr = new_arr;
  state->front = 0;
  state->rear = len;
  state->capacity = new_cap;
  return true;
}
