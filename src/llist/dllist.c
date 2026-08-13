#define _POSIX_C_SOURCE 200809L   // Enables declarations for POSIX functions and symbols

#include <stdlib.h>
#include <string.h>

#include <plily/llist.h>
#include <plily/common.h>

#include "internal.h"



/// Representation of Struct that holds the state of the LinkedList
/// This struct's members will be opaque to the consumers to prevent from editing
typedef struct _LinkedListState {
  Node *head;         ///< Reference to head node of linked list
  Node *tail;         ///< Reference to tail node of linked list
  size_t size;        ///< No of nodes in linked list
} _LinkedListState;


// =====================
// Forward declarations
// =====================
static Value get(const DLinkedList *self, int index);

static int pl_index(const DLinkedList *self, const VType vtype, const void *data);

static bool append(DLinkedList *self, const VType vtype, const void *data);

static bool insert(DLinkedList *self, int index, const VType vtype, const void *data);

static bool replace(DLinkedList *self, int idx, const VType vtype, const void *data);

static Value pop(DLinkedList *self);

static bool pl_remove(DLinkedList *self, const VType vtype, const void *data);

static bool remove_at(DLinkedList *self, int index);

static void reverse(DLinkedList *self);

static inline size_t length(const DLinkedList *self);

static inline bool is_empty(const DLinkedList *self);

// UTILITY FUNCTION
static inline bool is_value_match(const Value value, const VType vtype, const void *data);



// =====================
// Public API
// =====================
DLinkedList* pl_dlinkedlist_init(void) {
  DLinkedList *dll = calloc(1, sizeof(DLinkedList));
  if (!dll) return NULL;

  // Initialize the LinkedListState stuct
  dll->_state = calloc(1, sizeof(_LinkedListState));
  if (!dll->_state) {
    free(dll);
    return NULL;
  }

  // Initialize with default values
  dll->_state->head = NULL;
  dll->_state->tail = NULL;
  dll->_state->size = 0;

  // Initialize the function pointers
  dll->get = get;
  dll->index = pl_index;
  dll->append = append;
  dll->insert = insert;
  dll->replace = replace;
  dll->pop = pop;
  dll->remove = pl_remove;
  dll->remove_at = remove_at;
  dll->reverse = reverse;
  dll->length = length;
  dll->is_empty = is_empty;

  return dll;
}


void pl_dlinkedlist_free(PL_DLinkedList **self) {
  if (!self || !(*self)) return;

  Node *head = (*self)->_state->head;
  Node *to_remove = NULL;

  // loop through linked list and free each node
  while (head) {
    to_remove = head;
    head = head->next;

    pl_free_node(to_remove);
  }

  free( (*self)->_state ); // free the internal _LinkedListState struct
  free( (*self) );         // free the LinkedList struct
  *self = NULL;            // avoid dangling pointer issue
}



// ============================================
// Core Functions Implementation
// ============================================
static Value get(const DLinkedList *self, int index) {
  if (!self || self->is_empty(self) || index < 0 || (size_t)index >= self->length(self))
    return (Value){0};

  Node *curr = NULL;
  size_t length = self->length(self);

  // if index is in first half, then use head node to reach the node
  // and if index is in second half, use tail node to reach the node

  if ((size_t)index <= length / 2) {
    curr = self->_state->head;
    for (size_t i = 0; i < (size_t)index; i++) curr = curr->next;
  }
  else {
    curr = self->_state->tail;
    size_t steps = (length - 1) - (size_t)index;
    for (size_t i = 0; i < steps; i++) curr = curr->prev;
  }

  return curr->value;
}


static int pl_index(const DLinkedList *self, VType vtype, const void *data) {
  if (!self || !data || self->is_empty(self)) return -1;

  Node *curr = self->_state->head;

  for (int i = 0; i < (int)self->length(self); i++) {
    if (!curr) return -1;     // protection against inconsistent size

    if (is_value_match(curr->value, vtype, data)) return i;

    curr = curr->next;
  }

  // if we reach here, no match found
  return -1;
}


static bool append(DLinkedList *self, VType vtype, const void *data) {
  if (!self || !data) return false;

  // allocate new node and update it with the value
  Node *n = pl_new_node(vtype, data);
  if (!n) return false;

  // in case the linked list is empty
  if (self->is_empty(self)) {
    self->_state->head = n;
    self->_state->tail = n;
  }
  else {
    n->prev = self->_state->tail;  // update previous ref of new node
    self->_state->tail->next = n;  // add new node to the linked list
    self->_state->tail = n;        // update the tail ref
  }

  self->_state->size++;
  return true;
}


static Value pop(DLinkedList *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  Node *pnode = self->_state->tail;
  Value pvalue = pnode->value;    // take  a copy of the Value

  if (self->_state->head == self->_state->tail) {
    self->_state->head = NULL;
    self->_state->tail = NULL;
  }
  else {
    self->_state->tail = pnode->prev;
    self->_state->tail->next = NULL;
  }

  free(pnode);
  self->_state->size--;

  // if Value contains str data, then caller is expected to free it
  // use pl_free_value_data() function to free the value after use
  return pvalue;
}


static void reverse(DLinkedList *self) {
  if (!self || self->length(self) <= 1) return;

  // for reversing, swap the prev <--> next reference
  Node *old_head = self->_state->head;
  Node *old_tail = self->_state->tail;

  Node *curr = old_head;
  Node *tmp = NULL;

  while (curr) {
    tmp = curr->next;

    curr->next = curr->prev;
    curr->prev = tmp;

    curr = tmp;
  }

  self->_state->head = old_tail;
  self->_state->tail = old_head;
}


static inline size_t length(const DLinkedList *self) {
  return self ? self->_state->size : 0;
}


static inline bool is_empty(const DLinkedList *self) {
  return self->length(self) == 0;
}



// ============================================
// Util Functions Implementation
// ============================================
static inline bool is_value_match(const Value value, const VType vtype, const void *data) {
  if (!data || value.vtype != vtype) return false;

  if (vtype == PL_INT) return ( value.as.ival == *(int *)data );
  if (vtype == PL_DOUBLE) return ( value.as.dval == *(double *)data );
  if (vtype == PL_STR) return strcmp(value.as.sval, (char *)data) == 0;

  return false;
}
