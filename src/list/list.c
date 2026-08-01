#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <plily/list.h>

#include "internal.h"
#include "plily/common.h"



// =====================
// Forward declarations
// =====================
static Value get(List *self, int index);

static int pl_index(List *self, VType vtype, void *data);

static bool append(List *self, VType vtype, void *data);

static bool insert(List *self, int index, VType vtype, void *data);

static bool replace(List *self, int index, VType vtype, void *data);

static Value* pop(List *self);

static bool pl_remove(List *self, VType vtype, void *data);

static bool remove_at(List *self, int index);

static void reverse(List *self);

static size_t length(List *self);

static bool is_empty(List *self);

// UTILITY FUNCTION
static bool is_shrink_limit_hit(List *self);

static bool is_resize_limit_hit(List *self);

static bool resize(List *self);

static bool shrink(List *self);


// =====================
// Public API
// =====================
List* pl_list_init(void) {
  List *lst = calloc(1, sizeof(List));
  if (!lst) return NULL;

  // Allocate the memory for the arr member
  lst->arr = calloc(MIN_ARRAY_CAPACITY, sizeof(Value));
  if (!lst->arr) { free(lst); return NULL; }

  // Initialize the List members
  lst->capacity = MIN_ARRAY_CAPACITY;
  lst->size = 0;

  // Initialize the function pointers
  lst->get = get;
  lst->index = pl_index;
  lst->append = append;
  lst->insert = insert;
  lst->replace = replace;
  lst->pop = pop;
  lst->remove = pl_remove;
  lst->remove_at = remove_at;
  lst->reverse = reverse;
  lst->length = length;
  lst->is_empty = is_empty;

  return lst;
}


void pl_list_free(List **self) {
  if (!self || !(*self)) return;

  // arr member check for not NULL and free each Value from the list
  if ( (*self)->arr ) {
    for (size_t i = 0; i < (*self)->size; i++)
      pl_free_value( &(*self)->arr[i] );
  }

  free( (*self)->arr );    // free the array of Value structs
  free( (*self) );         // free the List struct

  *self = NULL;            // to avoid dangling pointer, set it to null
}



// ============================================
// Core Functions Implementation
// ============================================
static Value get(List *self, int index) {
  if (!self || index < 0 || (size_t)index >= self->size) return (Value){0};

  return self->arr[index];
}


static int pl_index(List *self, VType vtype, void *data) {
  if (!self || !data || self->is_empty(self)) return -1;

  // Loop through each Value and check if the data matches
  for (int i = 0; i < (int)self->size; i++) {
    Value v = self->arr[i];

    // if data type matches, then check for value match
    if (v.vtype == vtype) {
      switch (vtype) {
        case PL_INT:
          if (v.as.ival == *(int *)data) return i;
          break;

        case PL_DOUBLE:
          // if absoulte difference is less than 0.000 000 001, then value match
          if (fabs(v.as.dval - *(double *)data) < 1E-9) return i;
          break;

        case PL_STR:
          if (strcmp(v.as.sval, (char *)data) == 0) return i;
          break;
      }
    }
  }

  // if we reached here, then no match found
  return -1;
}


static bool append(List *self, VType vtype, void *data) {
  if (!self || !data) return false;

  // check if the array size reached the capacity
  if ( is_resize_limit_hit(self) && !resize(self) ) return false;

  // update the value in the array
  if (!pl_update_value(&self->arr[self->size], vtype, data)) return false;

  // finally increment the size
  self->size++;
  return true;
}


static void reverse(List *self) {
  if (!self || self->size <= 1) return;

  for (size_t l = 0, r = self->size - 1; l < r; l++, r--) {
    PL_SWAP(self->arr[l], self->arr[r]);
  }
}


static size_t length(List *self) {
  return (!self ? 0 : self->size);
}


static bool is_empty(List *self) {
  return (!self ? true : self->size == 0);
}



// ============================================
// Util Functions Implementation
// ============================================
static bool is_resize_limit_hit(List *self) {
  if (!self) return false;

  // check if the no of Values reached 100% of capacity
  return (self->size == self->capacity);
}


static bool is_shrink_limit_hit(List *self) {
  if (!self) return false;

  // if the capacity is at the min, let's not shrink it (even array is empty)
  if (self->capacity <= MIN_ARRAY_CAPACITY) return false;

  // shrink trigger: size < 25% of capacity
  return ( self->size < (self->capacity / 4) );
}


static bool resize(List *self) {
  if (!self || !is_resize_limit_hit(self)) return false;

  size_t new_capacity = self->capacity * 2;

  Value *new_arr = realloc(self->arr, new_capacity * sizeof(Value));
  if (!new_arr) return false;

  // upon success, update the members
  self->arr = new_arr;
  self->capacity = new_capacity;
  return true;
}


static bool shrink(List *self) {
  if (!self || !is_shrink_limit_hit(self)) return false;

  size_t new_capacity = self->capacity / 2;
  if (new_capacity < MIN_ARRAY_CAPACITY) new_capacity = MIN_ARRAY_CAPACITY;

  Value *new_arr = realloc(self->arr, new_capacity * sizeof(Value));
  if (!new_arr) return false;

  // if realloc is succeded, then update the capacity & array ptr
  self->arr = new_arr;
  self->capacity = new_capacity;
  return true;
}
