#define _POSIX_C_SOURCE 200809L   // Enables declarations for POSIX functions and symbols

#include <stdlib.h>
#include <stddef.h>
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
static Value get(const LinkedList *self, int index);

static int pl_index(const LinkedList *self, const VType vtype, const void *data);

static bool append(LinkedList *self, const VType vtype, const void *data);

static bool insert(LinkedList *self, int index, const VType vtype, const void *data);

static bool replace(LinkedList *self, int idx, const VType vtype, const void *data);

static Value pop(LinkedList *self);

static bool pl_remove(LinkedList *self, const VType vtype, const void *data);

static bool remove_at(LinkedList *self, int index);

static void reverse(LinkedList *self);

static inline size_t length(const LinkedList *self);

static inline bool is_empty(const LinkedList *self);

// UTILITY FUNCTION
static inline bool is_value_match(const Value value, const VType vtype, const void *data);


// =====================
// Public API
// =====================
LinkedList* pl_linkedlist_init(void) {
  LinkedList *ll = calloc(1, sizeof(LinkedList));
  if (!ll) return NULL;

  // Initialize the LinkedListState stuct
  ll->_state = calloc(1, sizeof(_LinkedListState));
  if (!ll->_state) {
    free(ll);
    return NULL;
  }

  // Initialize with default values
  ll->_state->head = NULL;
  ll->_state->tail = NULL;
  ll->_state->size = 0;

  // Initialize the function pointers
  ll->get = get;
  ll->index = pl_index;
  ll->append = append;
  ll->insert = insert;
  ll->replace = replace;
  ll->pop = pop;
  ll->remove = pl_remove;
  ll->remove_at = remove_at;
  ll->reverse = reverse;
  ll->length = length;
  ll->is_empty = is_empty;

  return ll;
}


void pl_linkedlist_free(PL_LinkedList **self) {
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
static Value get(const LinkedList *self, int index) {
  if (!self || index < 0 || (size_t)index >= self->length(self)) return (Value){0};

  Node *curr = self->_state->head;

  for (size_t i = 0; i < (size_t)index; i++) {
    curr = curr->next;

    if (!curr) return (Value){0};    // protection against inconsistent size
  }

  return curr->value;
}


static int pl_index(const LinkedList *self, VType vtype, const void *data) {
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


static bool append(LinkedList *self, VType vtype, const void *data) {
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
    self->_state->tail->next = n;
    self->_state->tail = n;
  }

  self->_state->size++;
  return true;
}


static bool insert(LinkedList *self, int index, const VType vtype, const void *data) {
  if (!self || index < 0 || (size_t)index > self->length(self) || !data) return false;

  // 1. insert position at the end
  if (self->length(self) == (size_t)index)
    return append(self, vtype, data);

  // create a node and update it with value
  Node *n = pl_new_node(vtype, data);
  if (!n) return false;

  if (index == 0) {
    // 2. insert at the head position
    n->next = self->_state->head;
    self->_state->head = n;
  } else {
    // 3. insert in-b/w positions
    Node *prev = self->_state->head;

    // walk to the node before insert position
    for (size_t i = 0; i < (size_t)index - 1; i++) {
      // will occur in case linked list is corrupted (size mismatch)
      if (!prev || !prev->next) { pl_free_node(n);  return false; }

      prev = prev->next;
    }

    // update the node references
    n->next = prev->next;
    prev->next = n;
  }

  self->_state->size++;
  return true;
}


static bool replace(LinkedList *self, int idx, const VType vtype, const void *data) {
  if (!self || idx < 0 || (size_t)idx > self->length(self) || !data) return false;

  // check for valid data types
  if (vtype != PL_INT && vtype != PL_DOUBLE && vtype != PL_STR) return false;

  Node *curr = self->_state->head;
  char *new_str = NULL;

  // go to node to be replaced with
  for (int i = 0; i < idx; i++) curr = curr->next;
  if (!curr) return false;

  // if new value is a string, then, let's make a copy of the data
  if (vtype == PL_STR) {
    new_str = strdup( (char *)data );
    if (!new_str) return false;
  }

  // if the existing node contains str data, then free the memory
  if (curr->value.vtype == PL_STR) free(curr->value.as.sval);

  // based on the type update the data
  switch (vtype) {
    case PL_INT: curr->value.as.ival = *(int *)data; break;
    case PL_DOUBLE: curr->value.as.dval = *(double *)data; break;
    case PL_STR: curr->value.as.sval = new_str; break;
  }

  curr->value.vtype = vtype;
  return true;
}


static bool pl_remove(LinkedList *self, const VType vtype, const void *data) {
  if (!self || !data || self->is_empty(self)) return false;

  Node *prev = NULL;
  Node *curr = self->_state->head;

  // walk till the node until we got a match
  while (curr) {
    if ( is_value_match(curr->value, vtype, data) ) break;

    prev = curr;
    curr = curr->next;
  }

  // no match found and we have reached the end of linked list
  if (!curr) return false;

  // prev will be null ,if the match found at head node
  if (prev == NULL) {
    self->_state->head = curr->next;

    // in case only one element is present and it's being removed
    if (self->_state->head == NULL) self->_state->tail = NULL;
  }
  else {
    prev->next = curr->next;

    // if curr is last node, then update tail reference
    if (self->_state->tail == curr) self->_state->tail = prev;
  }

  pl_free_node(curr);
  self->_state->size--;
  return true;
}


static bool remove_at(LinkedList *self, int index) {
  if (!self || self->is_empty(self) || index < 0 || (size_t)index >= self->length(self))
    return false;

  Node *prev = NULL;
  Node *curr = self->_state->head;

  // walk to the index's node (track prev node too)
  for (size_t i = 0; i < (size_t)index; i++) {
    prev = curr;
    curr = curr->next;
  }

  // prev will be null ,if the match found at head node
  if (prev == NULL) {
    self->_state->head = curr->next;

    // in case only one element is present and it's being removed
    if (self->_state->head == NULL) self->_state->tail = NULL;
  }
  else {
    prev->next = curr->next;

    // if curr is last node, then update tail reference
    if (self->_state->tail == curr) self->_state->tail = prev;
  }

  pl_free_node(curr);
  self->_state->size--;
  return true;
}

static Value pop(LinkedList *self) {
  if (!self || self->is_empty(self)) return (Value){0};

  Node *pop_node = self->_state->tail;
  Value pop_val = pop_node->value;      // take copy of the value

  // case 1: if only one node is present
  if (self->_state->head ==  self->_state->tail) {
    self->_state->head = NULL;
    self->_state->tail = NULL;
  }
  else {
    // case 2: if more than one element is present
    Node *curr = self->_state->head;

    // walk to the node before tail
    while (curr->next != self->_state->tail) curr = curr->next;

    curr->next = NULL;
    self->_state->tail = curr;
  }

  free(pop_node);
  self->_state->size--;

  // if Value contains str data, then caller is expected to free it
  // use pl_free_value_data() function to free the value after use
  return pop_val;
}


static void reverse(LinkedList *self) {
  if (!self || self->is_empty(self) || self->length(self) == 1) return;

  Node *old_head = self->_state->head;   // this will become tail node

  Node *prev = NULL;
  Node *curr = self->_state->head;
  Node *next = NULL;

  while (curr) {
    next = curr->next;     // preserve the next node's reference

    curr->next = prev;
    prev = curr;

    curr = next;          // update curr with preserved next node
  }

  // prev will point to head node, and curr will be null
  self->_state->head = prev;
  self->_state->tail = old_head;
}


static inline size_t length(const LinkedList *self) {
  return self ? self->_state->size : 0;
}

static inline bool is_empty(const LinkedList *self) {
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
