#include <simul_efun.h>

/**
 * Returns a random element from an array.
 *
 * This is a convenience wrapper around element_of() that retains the older
 * name that some mudlib code still expects.
 *
 * @deprecated Prefer element_of()
 * @param {mixed*} haystack The array to select from
 * @return {mixed} A random element from the array
 */
mixed array_item(mixed *haystack) {
  assert_arg(pointerp(haystack), 1, "Array is required.");

  return element_of(haystack);
}

/**
 * Returns a new array containing the distinct elements of the input
 * array.
 *
 * @param {mixed*} arr - An array of mixed types.
 * @returns {mixed*} A new array with distinct elements from the input array.
 */
varargs mixed *distinct_array(mixed *arr, int same_order) {
  assert_arg(pointerp(arr), 1, "Array is required.");

  if(!!same_order) {
    return reduce(arr, function(mixed *acc, mixed element) {
      return member_array(element, acc) == -1
        ? acc + ({ element })
        : acc;
    }, ({}));
  } else {
    mapping m;

    m = allocate_mapping(arr, 0);

    return keys(m);
  }
}

/**
 * Creates a new array with only unique elements from the input array while
 * preserving their first-seen order.
 *
 * @param {mixed*} arr The array to deduplicate
 * @return {mixed*} New array containing only unique elements
 */
mixed *uniq_array(mixed *arr) {
  return distinct_array(arr, 1);
}

/**
 * Returns a new array containing the elements of the input array
 * from index 0 to start-1, and from end+1 to the end of the input
 * array. If start is greater than end, the new array will contain
 * all the elements of the input array.
 *
 * @param {mixed*} arr - The input array.
 * @param {int} start - The starting index of elements to be removed.
 * @param {int} end - The ending index of elements to be removed. Defaults to
 *                      start if not specified.
 * @returns {mixed*} A new array with specified elements removed.
 */
varargs mixed *remove_array_element(mixed *arr, int start, int end) {
  if(!end) end = start;
  if(start > end) return arr;
  return arr[0..start-1] + arr[end+1..];
}

/**
 * Creates a new array excluding elements between the given indices.
 *
 * This is a compatibility wrapper around remove_array_element().
 *
 * @param {mixed*} array The source array
 * @param {int} from Starting index to exclude from
 * @param {int} [to=from] Ending index to exclude to (inclusive)
 * @return {mixed*} New array with specified elements excluded
 */
varargs mixed *exclude_array(mixed *array, int from, int to) {
  return remove_array_element(array, from, to);
}

/**
 * Inserts one array into another array at a specific position.
 *
 * @param {mixed*} array The original array
 * @param {mixed*} new_array The array to insert
 * @param {int} at The position to insert at
 * @return {mixed*} New combined array
 */
varargs mixed *merge_array(mixed *array, mixed *new_array, int at) {
  mixed *bottom, *top;
  int len;

  !pointerp(array) && array = ({});
  !pointerp(new_array) && new_array = ({});

  len = sizeof(array);
  if(at <= 0)
    return new_array + array;

  if(at >= len)
    return array + new_array;

  bottom = array[0..at-1];
  top = array[at..];

  return bottom + new_array + top;
}

/**
 * Modifies the content of an array by removing existing elements
 * and/or adding new elements. Returns a new array with the
 * modifications.
 *
 * @param {mixed*} arr - The array from which elements will be removed and to
 *                        which new elements may be added.
 * @param {int} start - The zero-based index at which to start changing the
 *                      array. If negative, it will begin that many elements
 *                      from the end.
 * @param {int} delete_count - The number of elements to remove from the array,
 *                             starting from the index specified by start. If
 *                             delete_count is 0, no elements are removed.
 * @param {mixed*} items_to_add - An array of elements to add to the array at
 *                                   the start index. Can be omitted or passed as
 *                                   null if no elements are to be added.
 * @returns {mixed*} A new array reflecting the desired modifications.
 */
varargs mixed *splice(mixed *arr, int start, int delete_count, mixed *items_to_add) {
  mixed *before, *after;
  if(!pointerp(items_to_add))
    items_to_add = ({});

  before = arr[0..start - 1];
  after = arr[start + delete_count..];

  return before + items_to_add + after;
}

/**
 * Creates a new array with elements in reverse order. If 'in_place'
 * is non-zero, the array will be mutated in place rather than returning
 * a reversed copy.
 *
 * @param {mixed*} elements - Array to reverse
 * @param {int} [in_place] - Whether to mutate the original array.
 * @return {mixed*} New array with elements in reverse order
 *
 * @example
 * ```c
 * mixed *nums = ({1, 2, 3, 4});
 * nums = reverse_array(nums); // Returns ({4, 3, 2, 1})
 * ```
 */
varargs mixed* reverse_array(mixed *elements, int in_place) {
  mixed *arr, temp;
  int i, j, sz;

  assert_arg(pointerp(elements), 1, "Array is required");

  if(!!in_place) {
    arr = elements;
  } else {
    arr = copy(elements);
  }

  sz = sizeof(arr);
  for(i = 0, j = sz - 1; i < j; i++, j--) {
    temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
  }

  return arr;
}

/**
 * Creates a new array with elements in random order using the Fisher-Yates
 * shuffle algorithm.
 *
 * @param {mixed*} items The array to shuffle
 * @return {mixed*} New array with elements in random order
 */
mixed *array_shuffle(mixed *items) {
  mixed *arr, temp;
  int i, j;

  assert_arg(pointerp(items), 1, "Array is required");
  arr = copy(items);

  for(i = sizeof(arr) - 1; i > 0; i--) {
    j = random(i + 1);
    if(i != j) {
      temp = arr[i];
      arr[i] = arr[j];
      arr[j] = temp;
    }
  }

  return arr;
}

/**
 * Checks if all elements in the input array are of the specified
 * type. If the array is of size 0, it is considered uniform.
 *
 * @param {mixed*} arr - The array to check.
 * @param {string} type - The type to check for.
 * @returns {int} Returns 1 if all elements are of the specified type, 0
 *                 otherwise.
 */
int uniform_array(mixed *arr, string type) {
  int sz = sizeof(arr);

  if(!sz)
    return 1;

  return sizeof(filter(arr, (: typeof($1) == $2 :), type)) == sz;
}

/**
 * Calculates the sum of all integers in an array.
 *
 * @param {int*} nums Array of integers to sum
 * @return {int} The sum of all elements
 */
int array_sum(int *nums) {
  assert_arg(pointerp(nums), 1, "Invalid array");
  assert_arg(!sizeof(nums) || uniform_array(nums, "int"), 1,
             "Array must be empty or contain only integers");

  return reduce(nums, (: $1 + $2 :), 0);
}

/**
 * Returns an array filled with the specified value. If no array
 * is provided, an empty array is created. If no value is
 * provided, 0 is used as the value to fill the array with. If no
 * start index is provided, the array is filled from the end.
 *
 * @param {mixed*} arr - The array to fill.
 * @param {mixed} value - The value to fill the array with.
 * @param {int} start_index - The index at which to start filling the array. (optional)
 * @returns {mixed} The filled array.
 */
varargs mixed array_fill(mixed *arr, mixed value, int size, int start_index) {
  mixed *work;
  int len;

  if(!pointerp(arr))
    arr = ({});

  if(nullp(value))
    value = 0;

  if(nullp(size))
    error("array_fill: size is required");

  len = sizeof(arr);

  if(nullp(start_index))
    start_index = len;

  work = allocate(size);

  while(size--)
    work[size] = value;

  return arr[0..start_index-1] + work + arr[start_index..];
}

/**
 * Returns a new array of the specified size, filled with the
 * specified value. If the array is larger than the specified size,
 * the array is truncated to the specified size.
 *
 * @param {mixed*} arr - The array to pad.
 * @param {int} size - The size of the array to create.
 * @param {mixed} value - The value to fill the array with.
 * @param {int} [beginning] - Whether to fill the array from the beginning. (optional)
 * @returns {mixed} The padded array.
 */
varargs mixed array_pad(mixed *arr, int size, mixed value, int beginning) {
  mixed *work;
  int i;
  int len;

  !pointerp(arr) && arr = ({});

  len = sizeof(arr);

  if(size <= len)
    return arr[0..size-1];

  work = allocate(size - len, value);

  while(size--)
    work[size] = value;

  if(beginning)
    return work + arr;
  else
    return arr + work;
}

/**
 * Extends an array to a specified size by padding with a value.
 *
 * @param {mixed*} arr Array to pad
 * @param {int} sz Target size
 * @param {mixed} [val=UNDEFINED] Value to pad with
 * @return {mixed*} Padded array
 */
varargs mixed *pad_array(mixed *arr, int sz, mixed val) {
  int diff, len;

  assert_arg(pointerp(arr), 1, "Array is required");
  assert_arg(intp(sz), 2, "Size is required");

  len = sizeof(arr);
  if(sz <= len)
    return arr;

  if(nullp(val))
    val = undefined;

  diff = sz - len;

  return arr + allocate(diff, val);
}

/**
 * Formats an array of strings into multiple columns for display.
 *
 * @param {string*} items Array of strings to display
 * @param {int} [col=2] Number of columns to display
 * @param {int} [width=79] Total width available for all columns
 * @return {string} Text formatted into the requested number of columns
 */
varargs string array_columns(string *items, int col, int width) {
  int colwidth, rows, size, pos, i, j;
  string ret = "", inner;

  assert_arg(pointerp(items), 1, "Array is required");

  size = sizeof(items);
  if(!size)
    return "";

  if(col < 1)
    col = 2;
  if(width < col)
    width = 79;

  colwidth = width / col;
  if(colwidth < 1)
    colwidth = 1;

  rows = size / col;
  if(size % col)
    rows++;

  for(i = 0; i < rows; i++) {
    inner = "";
    for(j = 0; j < col; j++) {
      pos = (i * col) + j;
      if(pos >= size)
        break;

      inner += sprintf("%-*s", colwidth, items[pos]);
    }

    ret += trim(inner) + "\n";
  }

  return ret;
}

string *sort_alpha(string *list) {
  return sort_array(list, 1);
}

int *sort_num(int *list) {
  return sort_array(list, 1);
}

float *sort_float(float *list) {
  return sort_array(list, 1);
}

string *reverse_sort_alpha(string *list) {
  return sort_array(list, -1);
}

int *reverse_sort_num(int *list) {
  return sort_array(list, -1);
}

float *reverse_sort_float(float *list) {
  return sort_array(list, -1);
}

/**
 * Finds the intersection of two arrays - elements that appear in both arrays.
 *
 * This function supports comparing elements with a custom comparison function,
 * which is particularly useful for complex data types like mappings or objects
 * where simple equality comparison isn't sufficient. For example, finding objects
 * with matching properties or mappings with specific key values.
 *
 * The result contains all elements from both arrays that match according to
 * the comparison logic, with duplicates removed.
 *
 * @param {mixed*} arr1 First array to compare
 * @param {mixed*} arr2 Second array to compare
 * @param {function} [f] Optional comparison function taking (array_element, search_element)
 * @return {mixed*} New array containing elements found in both input arrays
 *
 * @example
 * ```c
 * // Simple intersection of string arrays
 * string *weapons1 = ({"sword", "axe", "spear"});
 * string *weapons2 = ({"bow", "spear", "sword"});
 * // Returns ({"sword", "spear"})
 * string *common = intersection(weapons1, weapons2);
 *
 * // Intersection with custom comparison of object arrays
 * object *players1 = ({player1, player2, player3});
 * object *players2 = ({player2, player4, player5});
 * // Returns objects with matching races
 * object *matching_race = intersection(players1, players2,
 *   (: $1->query_race() == $2->query_race() :));
 *
 * // Intersection of mappings by specific property
 * mapping *items1 = ({(["type":"weapon", "name":"sword"]),
 *                    (["type":"armor", "name":"shield"])});
 * mapping *items2 = ({(["type":"potion", "name":"healing"]),
 *                    (["type":"weapon", "name":"dagger"])});
 * // Returns items of type "weapon"
 * mapping *weapons = intersection(items1, items2,
 *   (: $1["type"] == $2["type"] && $1["type"] == "weapon" :));
 * ```
 */
varargs mixed *intersection(mixed *arr1, mixed *arr2, function f) {
  mixed *res = ({ });
  mixed *arrs = ({arr1, arr2});

  // Use the smaller array for the outer loop to minimise iterations
  arrs = sort_array(arrs, (: sizeof($1) > sizeof($2) :));

  foreach(mixed elem in arrs[0]) {
    if(includes(arrs[1], elem, f) && !includes(res, elem)) {
      res += ({ elem });
    }
  }

  return res;
}

/**
 * Checks if two arrays have any elements in common (intersection is not empty).
 * This is a faster alternative to checking `sizeof(intersection(arr1, arr2)) > 0`.
 *
 * @param {mixed*} arr1 - First array to check
 * @param {mixed*} arr2 - Second array to check
 * @param {function} [compare] - Optional comparison function taking (array_element, search_element)
 * @return {int} 1 if arrays have at least one element in common, 0 otherwise
 *
 * @example
 * ```c
 * // Check if any weapons match between two arrays
 * string *weapons1 = ({"sword", "axe", "mace"});
 * string *weapons2 = ({"bow", "axe", "dagger"});
 * if(intersects(weapons1, weapons2))
 *   write("You have some matching weapons!\n");
 *
 * // With custom comparison function
 * object *players1 = ({player1, player2, player3});
 * object *players2 = ({player4, player5, player6});
 * // Check if any players share the same guild
 * if(intersects(players1, players2, (: $1->query_guild() == $2->query_guild() :)))
 *   write("Some players share the same guild!\n");
 * ```
 */
varargs int intersects(mixed *arr1, mixed *arr2, function compare) {
  // pick the smaller array for the outer loop
  mixed *small = sizeof(arr1) > sizeof(arr2) ? arr2 : arr1;
  mixed *large = small == arr1 ? arr2 : arr1;

  if(valid_function(compare)) {
    function f = compare;

    foreach(mixed a in small) {
      foreach(mixed b in large) {
        if(f(a,b))
          return 1;
      }
    }

    return 0;
  }

  // zero closures in this path: find_index + includes() only
  return find_index(small, (: includes($(large), $1) :)) > -1;
}

/**
 * Removes and returns the last element of the array.
 *
 * @param {mixed*} arr - The array from which to pop an element.
 * @returns {mixed} The last element of the array.
 */
mixed pop(mixed ref *arr) {
  mixed ret;

  ret = arr[<1];
  arr = arr[0..<2];

  return ret;
}

mixed array_pop(mixed ref *arr) {
  return pop(arr);
}

/**
 * Adds a new element to the end of the array and returns the new
 * size of the array.
 *
 * @param {mixed*} arr - The array to which to push an element.
 * @param {mixed} value - The element to push onto the array.
 * @returns {int} The new size of the array.
 */
int push(mixed ref *arr, mixed value) {
  arr += ({ value });
  return sizeof(arr);
}

int array_push(mixed ref *arr, mixed value) {
  return push(arr, value);
}

/**
 * Removes and returns the first element of the array.
 *
 * @param {mixed*} arr - The array from which to shift an element.
 * @returns {mixed} The first element of the array.
 */
mixed shift(mixed ref *arr) {
  mixed ret;

  ret = arr[0];
  arr = arr[1..];

  return ret;
}

mixed array_shift(mixed ref *arr) {
  return shift(arr);
}

/**
 * Adds a new element to the beginning of the array and returns
 * the new size of the array.
 *
 * @param {mixed*} arr - The array to which to unshift an element.
 * @param {mixed} value - The element to unshift onto the array.
 * @returns {int} The new size of the array.
 */
int unshift(mixed ref *arr, mixed value) {
  arr = ({ value }) + arr;
  return sizeof(arr);
}

int array_unshift(mixed ref *arr, mixed value) {
  return unshift(arr, value);
}

/**
 * Adds a value to the end of an array only if it doesn't already exist.
 *
 * @param {mixed ref*} arr The array to modify
 * @param {mixed} value The value to add uniquely
 * @return {int} The size of the array after the operation
 */
int set_push(mixed ref *arr, mixed value) {
  if(member_array(value, arr) != -1)
    return sizeof(arr);

  return push(arr, value);
}

/**
 * Adds a value to the beginning of an array only if it doesn't already exist.
 *
 * @param {mixed ref*} arr The array to modify
 * @param {mixed} value The value to add uniquely
 * @return {int} The size of the array after the operation
 */
int set_unshift(mixed ref *arr, mixed value) {
  if(member_array(value, arr) != -1)
    return sizeof(arr);

  return unshift(arr, value);
}

/**
 * Returns a new array containing the elements of the input array
 * from the start index to the end index. If the end index is
 * negative, it will start from the end of the array.
 *
 * @param {mixed*} arr - The array to slice.
 * @param {int} start - The starting index of the slice.
 * @param {int} end - The ending index of the slice.
 * @returns {mixed*} A new array with the specified elements.
 */
varargs mixed *slice(mixed *arr, int start, int end) {
  if(nullp(arr))
    return ({});

  if(end < 0)
    end = sizeof(arr) + end;

  return arr[start..end];
}

/**
 * Reduces an array to a single value by applying a function to each element.
 * The function accumulates a result as it processes each element.
 *
 * @param {mixed*} arr Array to reduce
 * @param {function} fun Function taking (accumulator, currentValue, index, array)
 * @param {mixed} [init=arr[0]] Initial value for accumulator
 * @returns {mixed} Final accumulated value
 *
 * @example
 * ```c
 * int *nums = ({1, 2, 3, 4});
 * // Sum all numbers
 * int sum = reduce(nums, (: $1 + $2 :), 0);
 * // Multiply all numbers
 * int product = reduce(nums, (: $1 * $2 :), 1);
 * ```
 */
varargs mixed reduce(mixed *arr, function fun, mixed init, mixed arg...) {
  mixed result;
  int start, i;

  if(!pointerp(arr))
    error("Bad argument 1 to reduce");

  if(!valid_function(fun))
    error("Bad argument 2 to reduce");

  // If no initial value and array is empty, error
  if(!sizeof(arr) && nullp(init))
    error("Reduce of empty array with no initial value");

  // If no initial value, use first element
  if(nullp(init)) {
    result = arr[0];
    start = 1;
  } else {
    result = init;
    start = 0;
  }

  arg = pointerp(arg) && sizeof(arg) ? arg : ({});

  for(i = start; i < sizeof(arr); i++)
    result = fun(result, arr[i], i, arr, arg...);

  return result;
}

/**
 * Checks if every element in the array satisfies a testing rule.
 *
 * @param {mixed*} arr Array to test
 * @param {mixed} criteria Function or value to test each element
 * @returns {int} 1 if all elements pass the test, 0 if any fail
 * @errors If arguments are invalid types
 */
int every(mixed *arr, mixed criteria) {
  function fun;

  assert_arg(pointerp(arr), 1, "Array is required");
  assert_arg(!nullp(criteria), 2, "Criteria is required");

  fun = valid_function(criteria) ? criteria : (:$1 == $(criteria):);

  foreach(mixed elem in arr)
    if(!fun(elem))
      return 0;

  return 1;
}

/**
 * Checks if an array includes a specific element.
 *
 * @param {mixed*} arr Array to search
 * @param {mixed} elem Element to find
 * @param {function} [f] Optional comparison function taking (array_element, search_element)
 * @returns {int} 1 if element is found, 0 if not
 */
varargs int includes(mixed *arr, mixed elem, function f) {
  int result;

  if(!pointerp(arr) || !sizeof(arr))
    return 0;

  if(valid_function(f))
    result = find_index(arr, f, elem);
  else
    result = member_array(elem, arr);

  return result != -1;
}

/**
 * Checks if a value exists in an array without caring about its position.
 *
 * @param {mixed} needle The value to search for
 * @param {mixed*} haystack The array to search in
 * @param {function} [f] Comparison function taking (elem, needle)
 * @return {int} Returns 1 if found, 0 if not found
 */
varargs int in_array(mixed needle, mixed *haystack, function f) {
  return includes(haystack, needle, f);
}

private int same_array_exact(mixed *one, mixed *two) {
  int sz;

  if(sizeof(one) != sizeof(two))
    return 0;

  sz = sizeof(one);
  while(sz--)
    if(one[sz] != two[sz])
      return 0;

  return 1;
}

/**
 * Checks if two arrays contain the same elements.
 *
 * @param {mixed*} one - First array to compare
 * @param {mixed*} two - Second array to compare
 * @param {int} [exact=0] - If 1, checks for exact match including order
 * @returns {int} 1 if arrays match, 0 if not
 */
varargs int same_array(mixed *one, mixed *two, int exact) {
  int sz_one, sz_two;

  // Quick size check first - if sizes differ, arrays can't be equal
  sz_one = sizeof(one);
  sz_two = sizeof(two);
  if(sz_one != sz_two)
    return 0;

  // No elements = arrays are equal
  if(!sz_one)
    return 1;

  if(!!exact)
    return same_array_exact(one, two);

  // Check each element exists in both arrays
  return every(one, (: includes($(two), $1) :));
}

int same_array_precisely(mixed *one, mixed *two) {
  return same_array(one, two, 1);
}

/**
 * Tests whether at least one element in the array passes the test.
 *
 * @param {mixed*} arr Array to test
 * @param {mixed} criteria Function or value to test each element
 * @returns {int} 1 if any element passes, 0 if none do
 * @errors If arguments are invalid types
 */
int some(mixed *arr, mixed criteria) {
  function fun;

  assert_arg(pointerp(arr), 1, "Array is required");
  assert_arg(!nullp(criteria), 2, "Criteria is required");

  fun = valid_function(criteria) ? criteria : (:$1 == $(criteria):);

  foreach(mixed elem in arr)
    if(fun(elem))
      return 1;

  return 0;
}

/**
 * Removes and returns an element at a specific index.
 *
 * @param {mixed*} arr - Array to modify
 * @param {int} index - Index of element to remove
 * @returns {mixed} The removed element
 */
mixed eject(mixed ref *arr, int index) {
  mixed ret;

  ret = arr[index];
  if(index == 0)
    return shift(ref arr);
  else if(index == sizeof(arr)-1)
    return pop(ref arr);

  arr = arr[0..index-1] + arr[index+1..];

  return ret;
}

mixed array_eject(mixed ref *arr, int index) {
  return eject(arr, index);
}

/**
 * Removes the first occurrence of a value from an array.
 *
 * This function finds and removes the first occurrence of a specified value
 * from an array. If the value is found, it is removed and the function returns
 * the index where it was found. If the value is not found, the array remains
 * unchanged and -1 is returned.
 *
 * @param {mixed ref*} arr - The array to modify.
 * @param {mixed} value - The value to remove from the array.
 * @returns {int} The index where the value was found and removed, or -1 if not found.
 * @errors If the first argument is not an array
 *
 * @example
 * ```c
 * mixed *fruits = ({"apple", "banana", "orange"});
 * int removed_at = eject_value(ref fruits, "banana"); // Returns 1, fruits becomes ({"apple", "orange"})
 * int not_found = eject_value(ref fruits, "grape"); // Returns -1, fruits remains unchanged
 * ```
 */
varargs int eject_value(mixed ref *arr, mixed value) {
  int index;

  assert_arg(pointerp(arr), 1, "Array is required");

  index = member_array(value, arr);
  if(index == -1)
    return index;

  eject(ref arr, index);

  return index;
}

varargs int array_remove(mixed ref *arr, mixed value) {
  return eject_value(arr, value);
}

/**
 * Removes all occurrences of a value from an array.
 *
 * This function finds and removes all occurrences of a specified value
 * from an array. It continues removing until no more instances of the
 * value are found. The function returns the total number of elements
 * that were removed.
 *
 * @param {mixed ref*} arr - The array to modify.
 * @param {mixed} value - The value to remove from the array.
 * @returns {int} The total number of occurrences that were removed.
 * @errors If the first argument is not an array
 *
 * @example
 * ```c
 * mixed *items = ({"apple", "banana", "apple", "orange", "apple"});
 * int removed = eject_value_all(ref items, "apple"); // Returns 3, items becomes ({"banana", "orange"})
 *
 * mixed *numbers = ({1, 2, 3, 2, 4, 2});
 * int count = eject_value_all(ref numbers, 2); // Returns 3, numbers becomes ({1, 3, 4})
 *
 * mixed *empty = ({});
 * int none = eject_value_all(ref empty, "test"); // Returns 0, array remains empty
 * ```
 */
varargs int eject_value_all(mixed ref *arr, mixed value) {
  int cnt = 0;

  while(eject_value(arr, value) != -1)
    cnt++;

  return cnt;
}

varargs void array_remove_all(mixed ref *arr, mixed value) {
  eject_value_all(arr, value);
}

/**
 * Inserts an element at a specific index.
 *
 * @param {mixed*} arr - Array to modify
 * @param {mixed} value - Value to insert
 * @param {int} index - Index where to insert
 * @returns {int} New size of array
 */
mixed insert(mixed ref *arr, mixed value, int index) {
  if(index == 0)
    return unshift(ref arr, value);
  else if(index == sizeof(arr)-1)
    return push(ref arr, value);

  arr = arr[0..index-1] + ({value}) + arr[index..];

  return sizeof(arr);
}

mixed array_insert(mixed ref *arr, mixed value, int index) {
  return insert(arr, value, index);
}

/**
 * Flattens a nested array into a single-level array.
 *
 * @param {mixed*} arr - Array to flatten
 * @returns {mixed*} Flattened array
 * @errors If argument is not an array
 */
mixed *flatten(mixed *arr) {
  int i = 0;

  assert_arg(pointerp(arr), 1, "Array is required");

  arr = copy(arr);

  while(i < sizeof(arr))
    if(pointerp(arr[i]))
      arr[i..i] = arr[i];
    else
      i++;

  return arr;
}

mixed *flatten_array(mixed *arr) {
  return flatten(arr);
}

/**
 * Returns the index of the first element that passes the test function.
 *
 * @param {mixed*} arr - Array to search
 * @param {function} fun - Test function
 * @param {mixed} [extra...] - Additional arguments to pass to test function
 * @returns {int} Index of first matching element or -1 if none found
 * @errors If arguments are invalid types
 */
varargs int find_index(mixed *arr, function fun, mixed extra...) {
  int i, sz;

  assert_arg(pointerp(arr), 1, "Array is required");
  assert_arg(valid_function(fun), 2, "Function is required");

  for(i = 0, sz = sizeof(arr); i < sz; i++)
    if(extra ? fun(arr[i], extra...) : fun(arr[i]) == 1)
      return i;

  return -1;
}

/**
 * Returns the first element that passes the test function.
 *
 * @param {mixed*} arr - Array to search
 * @param {function} fun - Test function
 * @param {mixed} [extra...] - Additional arguments to pass to test function
 * @returns {mixed} First matching element or null if none found
 * @errors If arguments are invalid types
 */
varargs mixed find(mixed *arr, function fun, mixed extra...) {
  int index;

  assert_arg(pointerp(arr), 1, "Array is required");
  assert_arg(valid_function(fun), 2, "Function is required");

  index = find_index(arr, fun, extra...);

  return index != -1 ? arr[index] : null;
}

/**
 * Checks if an index is within the valid range of an array.
 *
 * @param {int} index - Index to check
 * @param {mixed*} arr - Array to check against
 * @returns {int} 1 if index is valid, 0 if not
 */
int in_range(int index, mixed *arr) {
  if(!sizeof(arr))
    return 0;

  return clamped(index, 0, sizeof(arr)-1);
}

/**
 * Executes a function for each element in an array or mapping.
 *
 * For arrays, the function is called with (element, index, array, ...extra).
 * For mappings, the function is called with (key, value, mapping, ...extra).
 */
varargs void each(mixed src, function fun, mixed extra...) {
  assert_arg(pointerp(src) || mapp(src), 1, "Array or mapping required.");
  assert_arg(valid_function(fun), 2, "Function is required.");

  if(pointerp(src)) {
    int i, sz;

    for(i = 0, sz = sizeof(src); i < sz; i++) {
      extra ? fun(src[i], i, src, extra...) : fun(src[i], i, src);
    }
  }

  if(mapp(src)) {
    mixed key, val;

    foreach(key, val in src) {
      extra ? fun(key, val, src, extra...) : fun(key, val, src);
    }
  }
}

/**
 * Finds and returns the first non-null result of applying a function to each
 * element in an array.
 *
 * Iterates over the array, calling the provided function for each element. If
 * the function returns a non-null value for any element, that value is
 * immediately returned. If no such value is found, returns UNDEFINED.
 *
 * @param {mixed*} src - The array to search
 * @param {function} fun - The function to apply to each element (called as
 *                         fun(element, index, array, size, ...extra))
 * @param {mixed...} [extra] - Optional extra arguments passed to the function
 * @return {mixed} The first non-null result, or UNDEFINED if none found
 */
varargs mixed eval_first(mixed *src, function fun, mixed extra...) {
  int i, sz;

  assert_arg(pointerp(src), 1, "Array is required.");
  assert_arg(valid_function(fun), 2, "Function is required.");

  if((sz = sizeof(src)) > 0) {
    for(; i < sz; i++) {
      mixed result = extra
        ? fun(src[i], i,  src, sz, extra...)
        : fun(src[i], i, src, sz);

      if(!nullp(result))
        return result;
    }
  }
}

/**
 * Finds and returns the last non-null result of applying a function to each
 * element in an array.
 *
 * Iterates over the array in reverse order, calling the provided function for
 * each element. If the function returns a non-null value for any element, that
 * value is immediately returned. If no such value is found, returns UNDEFINED.
 *
 * @param {mixed*} src The array to search
 * @param {function} fun The function to apply to each element (called as fun(element, index, array, size, ...extra))
 * @param {mixed...} [extra] Optional extra arguments passed to the function
 * @return {mixed} The last non-null result, or UNDEFINED if none found
 *
 * @example
 * ```c
 * // Find the last string starting with 'a'
 * string *words = ({"cat", "apple", "ant", "dog"});
 * string last_a = eval_last(words, (: sscanf($1, "a%*s") ? $1 : 0 :)); // Returns "ant"
 * ```
 */
varargs mixed eval_last(mixed *src, function fun, mixed extra...) {
  assert_arg(pointerp(src), 1, "Array is required.");
  assert_arg(valid_function(fun), 2, "Function is required.");

  return eval_first(reverse_array(src), fun, extra);
}
