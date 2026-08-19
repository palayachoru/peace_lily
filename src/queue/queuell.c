#define _POSIX_C_SOURCE 200809L   // Enables declarations for POSIX functions and symbols

#include <stdlib.h>

#include <plily/queue.h>
#include <plily/common.h>

#include "internal.h"



/// Struct to represent the internal state of the Queue
/// This struct's members will be opaque to the consumers to prevent from editing
typedef struct _QueueLLState {
  Node *front;         ///< Represent the head of the linkedlist (remove values)
  Node *rear;          ///< Represent the tail of the linkedlist (insert values)
  size_t size;         ///< No of nodes in the linked list
} _QueueLLState;



// =====================
// Forward declarations
// =====================
static bool enqueue(QueueLL *self, VType vtype, const void *data);

static Value dequeue(QueueLL *self);

static Value peek(QueueLL *self);

static int length(QueueLL *self);

static bool is_empty(QueueLL *self);



// =====================
// Public API
// =====================
QueueLL* pl_queuell_init(void) {
  QueueLL *qll = calloc(1, sizeof(QueueLL));
  if (!qll) return NULL;

  // Initialize the _QueueLLState
  qll->_state = calloc(1, sizeof(_QueueLLState));
  if (!qll->_state) {
    free(qll);
    return NULL;
  }

  // Initialize with defaut values
  qll->_state->front = NULL;
  qll->_state->rear = NULL;
  qll->_state->size = 0;

  // Initialize the function pointers
  qll->enqueue = enqueue;
  qll->dequeue = dequeue;
  qll->peek = peek;
  qll->length = length;
  qll->is_empty = is_empty;

  return qll;
}


void pl_queuell_free(PL_QueueLL **self) {
  if (!self || !(*self)) return;

  Node *curr = (*self)->_state->front;
  Node *nxt = NULL;

  // Free the elements in queue
  while (curr) {
    nxt = curr->next;
    pl_free_node(curr);
    curr = nxt;
  }

  free( (*self)->_state );   // free the internale state
  free( (*self) );           // free the queue itself
  *self = NULL;
}



// ============================================
// Core Functions Implementation
// ============================================
static bool enqueue(QueueLL *self, VType vtype, const void *data) {
  if (!self || !data) return false;

  // Initialize a new node & update value
  Node *n = pl_new_node(vtype, data);
  if (!n) return false;

  // add new node at rear of queue (tail of linked list)
  if (self->is_empty(self)) {
    self->_state->rear = n;
    self->_state->front = n;
  }
  else {
    self->_state->rear->next = n;
    self->_state->rear = n;
  }

  self->_state->size++;
  return true;
}


static Value dequeue(QueueLL *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  Node *n = self->_state->front;
  self->_state->front = n->next;
  self->_state->size--;

  if (self->is_empty(self)) self->_state->rear = self->_state->front;

  Value val = n->value;
  free(n);

  // caller is responsible to free the Value; use pl_free_value_data()
  return val;
}


static Value peek(QueueLL *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  // the returned value is shallow snapshot of the Value in the Queue
  // Caller is not permitted to free/edit the value
  return self->_state->front->value;
}


static int length(QueueLL *self) {
  return self ? (int)self->_state->size : 0;
}


static bool is_empty(QueueLL *self) {
  return self ? self->_state->size == 0 : 0;
}
