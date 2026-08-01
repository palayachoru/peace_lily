#pragma once

/**
 * @file list.h
 * @brief List (Dynamic Hetrogenous Array)
 *
 * Dynamic array than can hold data of different data types.
 *
 * array grows   -> if no of elements reach 100% of it's total capacity
 * array shrinks -> if no of elements reach 25% of it's total capacity
 */

#include <stdbool.h>
#include <stddef.h>

#include <plily/common.h>

#define MIN_ARRAY_CAPACITY  4


/// Representation of List (Dynamic Array) struct
typedef struct PL_List {
  /// Get the value at the given index
  PL_Value (* get)(struct PL_List *self, int index);

  /// Get the index of the first occurrence of the value
  int (* index)(struct PL_List *self, PL_VType vtype, void *data);

  /// Append the value at the end of the list
  bool (* append)(struct PL_List *self, PL_VType vtype, void *data);

  /// Insert the value at the given index
  bool (* insert)(struct PL_List *self, int index, PL_VType vtype, void *data);

  /// Replace the value at the given index
  bool (* replace)(struct PL_List *self, int index, PL_VType vtype, void *data);

  /// Pop the last value from the list
  PL_Value* (* pop)(struct PL_List *self);

  /// Remove the first occurrence of the value
  bool (* remove)(struct PL_List *self, PL_VType vtype, void *data);

  /// Remove the value at the given index
  bool (* remove_at)(struct PL_List *self, int index);

  /// Reverse the list
  void (* reverse)(struct PL_List *self);

  /// Get the size of the list
  size_t (* length)(struct PL_List *self);

  /// Check if the list is empty
  bool (* is_empty)(struct PL_List *self);

  PL_Value *arr;     ///< Array of PL_Value struct
  size_t capacity;   ///< No of Values the list can hold
  size_t size;       ///< No of Values in the list
} PL_List;



// ========================
// Function Declarations
// ========================
/**
 * @brief Allocate memory for PL_List struct and initialize with values
 * @return PL_List: reference to newly created PL_List struct
 */
PL_List* pl_list_init(void);

/**
 * @breif Free the List along will all the element in it
 * @param **self - reference of reference to PLList
 */
void pl_list_free(PL_List **self);
