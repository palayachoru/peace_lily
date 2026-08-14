#pragma once

#include <stdbool.h>

// =======
// MACROS
// =======

// do..while block is used not to provide looping but, it's
// a trick used to consider all the statments in one block
#define PL_SWAP(a, b) \
  do { \
    __typeof__(a) _tmp = (a); \
    (a) = (b);  \
    (b) = _tmp; \
  } while (0)



// ===================
// TYPE DECLERATIONS
// ===================

/// Enum to identify type of value in the Element's union
typedef enum { PL_INT, PL_DOUBLE, PL_STR } PL_VType;


/// Representation of Single Value struct
typedef struct PL_Value{
  union {                ///< holds data of any one type (8 bytes)
    int ival;
    double dval;
    char *sval;
  } as;                  ///< reads nicely: value->as.ival

  PL_VType vtype;        ///< to know type of data in the union (4 bytes)
} PL_Value;


/// Representation of Double Linked PL_Node struct
/// NOTE: The 'prev' member is not used in single linked list
typedef struct PL_Node {
  struct PL_Node *prev;  ///< Pointer to previous node in chain
  struct PL_Node *next;  ///< Pointer to next node in chain
  PL_Value value;        ///< Data member of the PL_Node
} PL_Node;



// ========================
// Function Declarations
// ========================
PL_Value* pl_new_value(const PL_VType vtype, const void *data);

bool pl_update_value(PL_Value *value, const PL_VType vtype, const void *data);

void pl_free_value_data(PL_Value v);  // use this for stack allocated PL_Value

void pl_free_value(PL_Value *v);      // use this for heap allocated PL_Value


PL_Node* pl_new_node(const PL_VType vtype, const void *data);

void pl_free_node(PL_Node *node);
