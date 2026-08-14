#pragma once

/**
 * @file llist.h
 * @brief Linked List
 */

#include <stddef.h>
#include <stdbool.h>

#include <plily/common.h>



/// Forward declaration: Struct to represent the state of the LinkedList
/// This struct is opaque to the consumers to prevent modifying the states
typedef struct _LinkedListState _LinkedListState;


/// Representation of Single Linked List
typedef struct PL_LinkedList {
  /// Get the value at the given index
  PL_Value (* get)(const struct PL_LinkedList *self, int index);

  /// Get the index of the first occurrence of the value
  int (* index)(const struct PL_LinkedList *self, const PL_VType vtype, const void *data);

  /// Append the value at the end of the list
  bool (* append)(struct PL_LinkedList *self, const PL_VType vtype, const void *data);

  /// Insert the value at the given index
  bool (* insert)(struct PL_LinkedList *self, int index, const PL_VType vtype, const void *data);

  /// Replace the value at the given index
  bool (* replace)(struct PL_LinkedList *self, int index, const PL_VType vtype, const void *data);

  /// Pop the last value from the list
  PL_Value (* pop)(struct PL_LinkedList *self);

  /// Remove the first occurrence of the value
  bool (* remove)(struct PL_LinkedList *self, const PL_VType vtype, const void *data);

  /// Remove the value at the given index
  bool (* remove_at)(struct PL_LinkedList *self, int index);

  /// Reverse the list
  void (* reverse)(struct PL_LinkedList *self);

  /// Get the size of the list
  size_t (* length)(const struct PL_LinkedList *self);

  /// Check if the list is empty
  bool (* is_empty)(const struct PL_LinkedList *self);

  _LinkedListState *_state;   ///< Internal struct to hold state of the Linkedlist
} PL_LinkedList;


/// Representation of Doubly Linked List
typedef struct PL_DLinkedList {
  /// Get the value at the given index
  /// Returned 'PL_Value' is snapshot of Node's value: Caller must not free or modify it
  PL_Value (* get)(const struct PL_DLinkedList *self, int index);

  /// Get the index of the first occurrence of the value
  int (* index)(const struct PL_DLinkedList *self, const PL_VType vtype, const void *data);

  /// Append the value at the end of the list
  bool (* append)(struct PL_DLinkedList *self, const PL_VType vtype, const void *data);

  /// Insert the value at the given index
  bool (* insert)(struct PL_DLinkedList *self, int index, const PL_VType vtype, const void *data);

  /// Replace the value at the given index
  bool (* replace)(struct PL_DLinkedList *self, int index, const PL_VType vtype, const void *data);

  /// Pop the last value from the list
  PL_Value (* pop)(struct PL_DLinkedList *self);

  /// Remove the first occurrence of the value
  bool (* remove)(struct PL_DLinkedList *self, const PL_VType vtype, const void *data);

  /// Remove the value at the given index
  bool (* remove_at)(struct PL_DLinkedList *self, int index);

  /// Reverse the list
  void (* reverse)(struct PL_DLinkedList *self);

  /// Get the size of the list
  size_t (* length)(const struct PL_DLinkedList *self);

  /// Check if the list is empty
  bool (* is_empty)(const struct PL_DLinkedList *self);

  _LinkedListState *_state;   ///< Internal struct to hold state of the Linkedlist
} PL_DLinkedList;



// ========================
// Function Declarations
// ========================
/**
 * @brief Allocate memory for PL_LinkedList struct and initialize with values
 * @return PLLinkedList - reference to newly created LinkedList struct
 */
PL_LinkedList* pl_linkedlist_init(void);

/**
 * @breif Free the List along will all the element in it
 * @param **self - reference of reference to List
 */
void pl_linkedlist_free(PL_LinkedList **self);


/**
 * @brief Allocate memory for PL_DLinkedList struct and initialize with values
 * @return PL_DLinkedList - reference to newly created Doubly LinkedList struct
 */
PL_DLinkedList* pl_dlinkedlist_init(void);

/**
 * @brief Free the List along will all the element in it
 * @param **self - reference of reference to List
 */
void pl_dlinkedlist_free(PL_DLinkedList **self);
