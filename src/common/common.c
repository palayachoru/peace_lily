#define _POSIX_C_SOURCE 200809L   // Enables declarations for POSIX functions and symbols

#include <stdlib.h>
#include <string.h>

#include <plily/common.h>

#include "internal.h"

void pl_free_value_data(PL_Value value) {
  if (value.vtype != PL_STR) return;

  // free the memory allocated for string data
  free(value.as.sval);
}


void pl_free_value(Value *value) {
  if (!value || value->vtype != PL_STR) return;

  // free the memory allocated for string data
  free(value->as.sval);
  free(value);
}

Value* pl_new_value(const VType vtype, const void *data) {
  if (!data) return NULL;

  // allocate memory for Element
  Value *value = calloc(1, sizeof(Value));
  if (!value) return NULL;

  // update Element with value
  if (!pl_update_value(value, vtype, data)) {
    pl_free_value(value);
    return NULL;
  }

  return value;
}


bool pl_update_value(Value *value, const VType vtype, const void *data) {
  if (!value || !data) return false;

  switch (vtype) {
    case PL_INT: value->as.ival = *(int *)data; break;
    case PL_DOUBLE: value->as.dval = *(double *)data; break;
    case PL_STR: {
      value->as.sval = strdup((char *)data);
      if (!value->as.sval) return false;
      break;
    }
    default:
      return false;
  }

  value->vtype = vtype;
  return true;
}





Node* pl_new_node(const VType vtype, const void *data) {
  if (!data) return NULL;

  // allocate memory for Node
  Node *node = calloc(1, sizeof(Node));
  if (!node) return NULL;

  // update the members with default values
  if (!pl_update_value(&node->value, vtype, data)) {
    pl_free_node(node);
    return NULL;
  }

  node->prev = NULL;
  node->next = NULL;
  return node;
}


void pl_free_node(Node *node) {
  if (!node) return;

  // free the memory allocated for string
  if (node->value.vtype == PL_STR) free(node->value.as.sval);

  free(node);
}
