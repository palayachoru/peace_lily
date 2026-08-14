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

static int dll_index(const DLinkedList *self, const VType vtype, const void *data);

static bool append(DLinkedList *self, const VType vtype, const void *data);

static bool insert(DLinkedList *self, int index, const VType vtype, const void *data);

static bool replace(DLinkedList *self, int index, const VType vtype, const void *data);

static Value pop(DLinkedList *self);

static bool dll_remove(DLinkedList *self, const VType vtype, const void *data);

static bool remove_at(DLinkedList *self, int index);

static void reverse(DLinkedList *self);

static inline size_t length(const DLinkedList *self);

static inline bool is_empty(const DLinkedList *self);

// UTILITY FUNCTION
static inline bool is_value_match(const Value value, const VType vtype, const void *data);

static Node* node_at(const DLinkedList *self, int index);



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
  dll->index = dll_index;
  dll->append = append;
  dll->insert = insert;
  dll->replace = replace;
  dll->pop = pop;
  dll->remove = dll_remove;
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
  Node *n = node_at(self, index);

  // The Value is borrowed not ownership transfer, so caller should not
  // free or modify the value.
  return n ? n->value : (Value){0};
}


static int dll_index(const DLinkedList *self, VType vtype, const void *data) {
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


static bool insert(DLinkedList *self, int index, const VType vtype, const void *data) {
  if (!self || index < 0 || (size_t)index > self->length(self) || !data) return false;

  // 1. insert position at the end
  if (self->length(self) == (size_t)index)
    return append(self, vtype, data);

  // create a node and update it with value
  Node *n = pl_new_node(vtype, data);
  if (!n) return false;

  if (index == 0) {
    // 2. insert at the head position
    Node *old_head = self->_state->head;

    n->prev = NULL;
    n->next = old_head;

    old_head->prev  = n;
    self->_state->head = n;
  }
  else {
    // 3. insert in-b/w positions
    Node *curr = node_at(self, index);
    if (!curr) {
      pl_free_node(n);
      return false;
    }

    // update node's references
    n->next = curr;
    n->prev = curr->prev;

    curr->prev->next = n;
    curr->prev = n;
  }

  self->_state->size++;
  return true;
}


static bool replace(DLinkedList *self, int index, const VType vtype, const void *data) {
  if (!self || self->is_empty(self) || index < 0 || (size_t)index >= self->length(self) || !data)
    return false;

  // check for valid data types
  if (vtype != PL_INT && vtype != PL_DOUBLE && vtype != PL_STR) return false;

  // get the node at the index
  Node *n = node_at(self, index);
  if (!n) return false;

  char *new_str = NULL;
  // if new value is a string, then, let's make a copy of the data
  if (vtype == PL_STR) {
    new_str = strdup( (char *)data );
    if (!new_str) return false;
  }

  // if the existing node contains str data, then free the memory
  if (n->value.vtype == PL_STR) free(n->value.as.sval);

  // based on the type update the data
  switch (vtype) {
    case PL_INT: n->value.as.ival = *(int *)data; break;
    case PL_DOUBLE: n->value.as.dval = *(double *)data; break;
    case PL_STR: n->value.as.sval = new_str; break;
  }

  n->value.vtype = vtype;
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


static bool dll_remove(DLinkedList *self, const VType vtype, const void *data) {
  if (!self || !data || self->is_empty(self)) return false;

  // get the matching node
  Node *curr = self->_state->head;
  while (curr) {
    if (is_value_match(curr->value, vtype, data)) break;
    curr = curr->next;
  }

  if (!curr) return false;   // no match found (reached the end of the list)

  // prev is Null, if match found is at the head node
  if (curr->prev == NULL) {
    self->_state->head = curr->next;

    // if only one element is present and it's being removed
    if (self->_state->head == NULL) self->_state->tail = NULL;
  }
  else {
    curr->prev->next = curr->next;

    // if curr is last node, then update the tail reference
    if (self->_state->tail == curr)
      self->_state->tail = curr->prev;
    else
      curr->next->prev = curr->prev;
  }

  pl_free_node(curr);
  self->_state->size--;
  return true;
}


static bool remove_at(DLinkedList *self, int index) {
  if (!self || self->is_empty(self) || index < 0 || (size_t)index >= self->length(self))
    return false;

  // get the node to be removed
  Node *curr = node_at(self, index);
  if (!curr) return false;

  // prev is Null, if match found is at the head node
  if (curr->prev == NULL) {
    self->_state->head = curr->next;

    // if only one element is present and it's being removed
    if (self->_state->head == NULL) self->_state->tail = NULL;
    else self->_state->head->prev = NULL;
  }
  else {
    curr->prev->next = curr->next;

    // if curr is last node, then update the tail reference
    if (self->_state->tail == curr)
      self->_state->tail = curr->prev;
    else
      curr->next->prev = curr->prev;
  }

  pl_free_node(curr);
  self->_state->size--;
  return true;
}


static void reverse(DLinkedList *self) {
  if (!self || self->length(self) <= 1) return;

  // for reversing, swap the prev <--> next reference
  Node *old_head = self->_state->head;
  Node *old_tail = self->_state->tail;

  Node *curr = old_head;
  Node *next_node = NULL;

  while (curr) {
    next_node = curr->next;           // copy of next node ref
    PL_SWAP(curr->prev, curr->next);  // swap references b/w next and prev
    curr = next_node;
  }

  self->_state->head = old_tail;
  self->_state->tail = old_head;

  if (self->_state->head) self->_state->head->prev = NULL;
  if (self->_state->tail) self->_state->tail->next = NULL;
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


static Node* node_at(const DLinkedList *self, int index) {
  if (!self || self->is_empty(self) || index < 0 || (size_t)index >= self->length(self))
    return NULL;

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

  return curr;
}
