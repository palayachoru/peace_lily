#define _POSIX_C_SOURCE 200809L   // Enables declarations for POSIX functions and symbols

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <plily/llist.h>
#include <plily/common.h>

#include "internal.h"



// =====================
// Forward declarations
// =====================
static Value get(LinkedList *self, int index);

static int pl_index(LinkedList *self, VType vtype, void *data);

static bool append(LinkedList *self, VType vtype, void *data);

static bool insert(LinkedList *self, int index, VType vtype, void *data);

static bool replace(LinkedList *self, int idx, VType vtype, void *data);

static Value pop(LinkedList *self);

static bool pl_remove(LinkedList *self, VType vtype, void *data);

static bool remove_at(LinkedList *self, int index);

static void reverse(LinkedList *self);

static size_t length(LinkedList *self);

static bool is_empty(LinkedList *self);

// UTILITY FUNCTION
static bool is_value_match(Value value, VType vtype, const void *data);


// =====================
// Public API
// =====================
LinkedList* pl_linkedlist_init(void) {
  LinkedList *ll = calloc(1, sizeof(LinkedList));
  if (!ll) return NULL;

  // Initialize Linkedlist members
  ll->head = NULL;
  ll->tail = NULL;
  ll->size = 0;

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

  Node *head = (*self)->head;
  Node *to_remove = NULL;

  // loop through linked list and free each node
  while (head) {
    to_remove = head;
    head = head->next;

    pl_free_node(to_remove);
  }

  free( (*self) );   // free the LinkedList struct
  *self = NULL;      // avoid dangling pointer issue
}



// ============================================
// Core Functions Implementation
// ============================================
static Value get(LinkedList *self, int index) {
  if (!self || index < 0 || (size_t)index >= self->size) return (Value){0};

  Node *curr = self->head;

  for (size_t i = 0; i < (size_t)index; i++) {
    curr = curr->next;

    if (!curr) return (Value){0};    // protection against inconsistent size
  }

  return curr->value;
}


static int pl_index(LinkedList *self, VType vtype, void *data) {
  if (!self || !data || self->is_empty(self)) return -1;

  Node *curr = self->head;

  for (int i = 0; i < (int)self->size; i++) {
    if (!curr) return -1;     // protection against inconsistent size

    if (is_value_match(curr->value, vtype, data)) return i;

    curr = curr->next;
  }

  // if we reach here, no match found
  return -1;
}


static bool append(LinkedList *self, VType vtype, void *data) {
  if (!self || !data) return false;

  // allocate new node and update it with the value
  Node *n = pl_new_node(vtype, data);
  if (!n) return false;

  // in case the linked list is empty
  if (self->is_empty(self)) {
    self->head = n;
    self->tail = n;
  }
  else {
    self->tail->next = n;
    self->tail = n;
  }

  self->size++;
  return true;
}


static size_t length(LinkedList *self) {
  return self ? self->size : 0;
}

static bool is_empty(LinkedList *self) {
  return self->length(self) == 0;
}



// ============================================
// Util Functions Implementation
// ============================================
static bool is_value_match(Value value, VType vtype, const void *data) {
  if (!data || value.vtype != vtype) return false;

  if (vtype == PL_INT) return ( value.as.ival == *(int *)data );
  if (vtype == PL_DOUBLE) return ( value.as.dval == *(double *)data );
  if (vtype == PL_STR) return strcmp(value.as.sval, (char *)data) == 0;

  return false;
}
