/**
 * @file /std/object/value.c
 * Handles monetary value tracking for physical objects.
 *
 * @created 2024-08-01 - Gesslar
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * 2024-08-01 - Gesslar - Created
 * 2024-08-08 - Gesslar - Added save_var()
 * 2024-08-08 - Gesslar - Changed to singular value without currency type
 * 2025-03-16 - GitHub Copilot - Updated documentation format
 */

#include <persist.h>

/**
 * Value of the object in the lowest denomination of currency.
 *
 * @type {int}
 */
protected nosave int _value = null;

/**
 * Sets the value of the object.
 *
 * The value is stored as an integer representing the cost in the lowest
 * denomination of currency.
 *
 * @param {int} value - The value to set
 */
void set_value(int value) {
  save_var("_value");

  _value = value;
}

/**
 * Returns the value of the object.
 *
 * @returns {int} The object's value in the lowest denomination of currency
 */
int query_value() {
  return _value;
}

/**
 * Adjusts the value of the object by a relative amount.
 *
 * @param {int} value - The amount to add (or subtract if negative)
 */
void adjust_value(int value) {
  _value += value;
}
