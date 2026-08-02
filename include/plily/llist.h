#pragma once

/**
 * @file llist.h
 * @brief Single Linked List
 */

#include <stddef.h>
#include <stdbool.h>

#include <plily/common.h>



/// Representation of Single Linked List
typedef struct PL_LinkedList {
  /// Get the value at the given index
  PL_Value (* get)(struct PL_LinkedList *self, int index);

  /// Get the index of the first occurrence of the value
  int (* index)(struct PL_LinkedList *self, PL_VType vtype, void *data);

  /// Append the value at the end of the list
  bool (* append)(struct PL_LinkedList *self, PL_VType vtype, void *data);

  /// Insert the value at the given index
  bool (* insert)(struct PL_LinkedList *self, int index, PL_VType vtype, void *data);

  /// Replace the value at the given index
  bool (* replace)(struct PL_LinkedList *self, int index, PL_VType vtype, void *data);

  /// Pop the last value from the list
  PL_Value (* pop)(struct PL_LinkedList *self);

  /// Remove the first occurrence of the value
  bool (* remove)(struct PL_LinkedList *self, PL_VType vtype, void *data);

  /// Remove the value at the given index
  bool (* remove_at)(struct PL_LinkedList *self, int index);

  /// Reverse the list
  void (* reverse)(struct PL_LinkedList *self);

  /// Get the size of the list
  size_t (* length)(struct PL_LinkedList *self);

  /// Check if the list is empty
  bool (* is_empty)(struct PL_LinkedList *self);

  PL_Node *head;   ///< Reference to head node of linked list
  PL_Node *tail;   ///< Reference to tail node of linked list
  size_t size;     ///< No of nodes in linked list
} PL_LinkedList;



// ========================
// Function Declarations
// ========================
/**
 * @brief Allocate memory for PLLinkedList struct and initialize with values
 * @return PLLinkedList - reference to newly created LinkedList struct
 */
PL_LinkedList* pl_linkedlist_init(void);

/**
 * @breif Free the List along will all the element in it
 * @param **self - reference of reference to List
 */
void pl_linkedlist_free(PL_LinkedList **self);
