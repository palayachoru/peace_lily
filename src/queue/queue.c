#include <stdlib.h>
#include <string.h>

#include <plily/queue.h>

#include "internal.h"



/// Struct to represent the internal state of the Queue
/// This struct's members will be opaque to the consumers to prevent from editing
typedef struct _QueueState {
  Value *arr;       ///< Array of PL_Value struct
  size_t front;     ///< Pointer to front of queue
  size_t rear;      ///< Pointer to rear of queue
  size_t capacity;  ///< Capacity of the queue
} _QueueState;



// =====================
// Forward declarations
// =====================
static bool enqueue(Queue *self, VType vtype, const void *data);

static Value dequeue(Queue *self);

static const Value* peek(Queue *self);

static int length(Queue *self);

static bool is_empty(Queue *self);

// UTIL FUNCTIONS
static bool is_full(Queue *q);

static bool resize(Queue *q);


// =====================
// Public API
// =====================
Queue* pl_queue_init(void) {
  Queue *q = calloc(1, sizeof(Queue));
  if (!q) return NULL;

  // INitialize the QueueState
  q->_state = calloc(1, sizeof(_QueueState));
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
  q->enqueue = enqueue;
  q->dequeue = dequeue;
  q->peek = peek;
  q->length = length;
  q->is_empty = is_empty;

  return q;
}


void pl_queue_free(PL_Queue **self) {
  if (!self || !(*self)) return;

  Queue *q = (*self);
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
static bool enqueue(Queue *self, VType vtype, const void *data) {
  if (!self || !data) return false;

  if (is_full(self) && !resize(self)) return false;

  _QueueState *state = self->_state;

  // rear always points to vacant space, so add value and increment rear
  if (!pl_update_value(&state->arr[state->rear], vtype, data)) return false;

  state->rear = (state->rear + 1) % state->capacity;
  return true;
}


static Value dequeue(Queue *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  size_t idx = self->_state->front;

  // copy of element to be removed
  Value popped = self->_state->arr[idx];

  // clear the date at the removed spot
  self->_state->arr[idx] = (Value){0};

  // update the front's index position
  self->_state->front = (idx + 1) % self->_state->capacity;

  // if Value contain string data, caller has to free it
  // use pl_free_node_data() to free the allocation
  return popped;
}


static const Value* peek(Queue *self) {
  if (!self || self->is_empty(self)) return NULL;

  // The returned Value is snapshot of Value in stack: caller
  // is not recommended to free/edit the Value
  return &self->_state->arr[self->_state->front];
}


static int length(Queue *self) {
  if (!self || !self->_state) return 0;

  _QueueState *state = self->_state;

  return (int)( (state->rear + state->capacity - state->front) % state->capacity );
}


static bool is_empty(Queue *self) {
  return (!self || !self->_state || self->_state->front == self->_state->rear);
}



// ============================================
// Util Functions Implementation
// ============================================
static bool is_full(Queue *q) {
  if (!q || !q->_state || q->_state->capacity < 2) return false;

  _QueueState *state = q->_state;

  return (state->rear + 1) % state->capacity == state->front ;
}


static bool resize(Queue *q) {
  if (!q || !q->_state || q->_state->capacity < 2) return false;

  size_t old_cap = q->_state->capacity;
  size_t new_cap = old_cap * 2;

  // allocate a new array with double the size
  Value *new_arr = calloc(new_cap, sizeof(Value));
  if (!new_arr) return false;

  size_t len = (size_t)q->length(q);

  _QueueState *state = q->_state;

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
