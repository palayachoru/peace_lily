
#include <stdlib.h>

#include <plily/stack.h>
#include "plily/common.h"

#include "internal.h"



/// Struct to represent the internal state of the Stack
/// This struct's members will be opaque to the consumers to prevent from editing
typedef struct _StackLLState {
  Node *top;            ///< Points to the Head node of Linked list
  size_t size;          ///< No of values in the stack
} _StackLLState;



// =====================
// Forward declarations
// =====================
static bool push(StackLL *self, VType vtype, const void *data);

static Value pop(StackLL *self);

static Value peek(StackLL *self);

static int length(StackLL *self);

static bool is_empty(StackLL *self);



// =====================
// Public API
// =====================
StackLL* pl_stackll_init(void) {
  StackLL *sll = calloc(1, sizeof(StackLL));
  if (!sll) return NULL;

  // Initialize StackLLState struct
  sll->_state = calloc(1, sizeof(_StackLLState));
  if (!sll->_state) {
    free(sll);
    return NULL;
  }

  // Initialize the default values
  sll->_state->size = 0;
  sll->_state->top = NULL;

  // Initialize function pointers
  sll->push = push;
  sll->pop = pop;
  sll->peek = peek;
  sll->is_empty = is_empty;
  sll->length = length;

  return sll;
}


void pl_stackll_free(StackLL **self) {
  if (!self || !(*self)) return;

  Node *curr = (*self)->_state->top;
  Node *nxt = NULL;

  while (curr) {
    nxt = curr->next;    // note reference to next node
    pl_free_node(curr);  // free the node
    curr = nxt;          // update curr to noted next ref
  }

  free( (*self)->_state );  // free the heap allocated StackLLState struct
  free( (*self) );          // finally free the Stack itself
  *self = NULL;             // to prevent dangling pointer issue, set to NULL
}



// ============================================
// Core Functions Implementation
// ============================================
static bool push(StackLL *self, VType vtype, const void *data) {
  if (!self || !data) return false;

  // Initialize new Node and update it with value
  Node *n = pl_new_node(vtype, data);
  if (!n) return false;

  // Add the new node at the head of the linked list
  n->next = self->_state->top;
  self->_state->top = n;

  self->_state->size++;
  return true;
}


static Value pop(StackLL *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  Node *n = self->_state->top;
  self->_state->top = n->next;
  self->_state->size--;

  Value val = n->value;
  free(n);

  // caller is responsible to free the Value; use pl_free_value_data()
  return val;
}


static Value peek(StackLL *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  // the returned value is shallow snapshot of the Value in the Stack
  // Caller is not permitted to free/edit the value
  return self->_state->top->value;
}

static int length(StackLL *self) {
  return self ? (int)self->_state->size : 0;
}


static bool is_empty(StackLL *self) {
  return self ? self->_state->size == 0 : 0;
}
