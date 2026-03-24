#include <simul_efun.h>

/**
 * Validates that an array is a valid simple map (array of key-value pairs).
 *
 * Checks that the argument is an array, and if `non_empty` is true, also
 * checks that every element is a two-element array (i.e., a key-value pair).
 *
 * @param {mixed} map The array to validate
 * @param {int} non_empty If true, requires all elements to be key-value pairs
 * @return {int} 1 if valid, 0 otherwise
 *
 * @example
 * ```c
 * mixed *pairs = ({ ({"a", 1}), ({"b", 2}) });
 * if(valid_seq(pairs, 1)) write("Valid map!\n");
 * ```
 */
varargs int valid_seq(mixed map, int non_empty) {
  if(!pointerp(map))
    return 0;

  if(non_empty)
    return sizeof(map) > 0 && every(map, (: pointerp($1) && sizeof($1) == 2 :));

  return 1;
}

/**
 * Sets a key-value pair in a sequential map. Creates new entry or updates
 * existing. This mirrors JavaScript Map.set() behavior - always succeeds.
 *
 * @param {mixed ref*} map - The sequential map to modify (array of {key, value} pairs)
 * @param {mixed} key - The key to set
 * @param {mixed} value - The value to associate with the key
 * @return {int} The size of the sequential map after the operation
 *
 * @example
 * ```c
 * mixed *usermap = ({{"alice", 25}, {"bob", 30}});
 * seq_set(ref usermap, "alice", 26); // Updates alice to 26, returns 2
 * seq_set(ref usermap, "charlie", 35); // Adds charlie, returns 3
 * ```
 */
int seq_set(mixed ref *map, mixed key, mixed value) {
  int i, sz;

  assert_arg(valid_seq(map), 1, "Sequential map is required");

  if((sz = sizeof(map)) > 0 && (i = find_index(map, (: $1[0] == $2 :), key)) != -1) {
    map[i][1] = value;

    return sz;
  }

  return push(map, ({key, value}));
}

/**
 * Gets the value associated with a key in a sequential map.
 * This mirrors JavaScript Map.get() behavior.
 *
 * @param {mixed*} map - The sequential map to search
 * @param {mixed} key - The key to look for
 * @return {mixed} The value associated with the key, or null if not found
 *
 * @example
 * ```c
 * mixed *usermap = ({{"alice", 25}, {"bob", 30}});
 * int age = seq_get(usermap, "alice"); // Returns 25
 * int unknown = seq_get(usermap, "charlie"); // Returns 0
 * ```
 */
mixed seq_get(mixed *map, mixed key) {
  int i, sz;

  assert_arg(valid_seq(map), 1, "Sequential map is required");

  for(i = 0, sz = sizeof(map); i < sz; i++) {
    if(sizeof(map[i]) >= 2 && map[i][0] == key) {
      return map[i][1];
    }
  }

  return ([])[0]; // Default return for non-existent keys
}

/**
 * Checks if a key exists in a sequential map.
 * This mirrors JavaScript Map.has() behavior.
 *
 * @param {mixed*} map - The sequential map to search
 * @param {mixed} key - The key to look for
 * @return {int} 1 if key exists, 0 otherwise
 *
 * @example
 * ```c
 * mixed *usermap = ({{"alice", 25}, {"bob", 30}});
 * if(seq_has(usermap, "alice")) {
 *   write("Alice is in the map!\n");
 * }
 * ```
 */
int seq_has(mixed *map, mixed key) {
  int i, sz;

  assert_arg(valid_seq(map), 1, "Sequential map is required");

  for(i = 0, sz = sizeof(map); i < sz; i++) {
    if(sizeof(map[i]) >= 2 && map[i][0] == key) {
      return 1;
    }
  }

  return 0;
}

/**
 * Removes a key-value pair from a sequential map.
 * This mirrors JavaScript Map.delete() behavior.
 *
 * @param {mixed ref*} map - The sequential map to modify
 * @param {mixed} key - The key to remove
 * @return {int} 1 if key was found and removed, 0 if key didn't exist
 *
 * @example
 * ```c
 * mixed *usermap = ({{"alice", 25}, {"bob", 30}, {"charlie", 35}});
 * seq_delete(ref usermap, "bob"); // Returns 1, removes bob
 * seq_delete(ref usermap, "david"); // Returns 0, key didn't exist
 * ```
 */
int seq_delete(mixed ref *map, mixed key) {
  int i, sz;

  assert_arg(valid_seq(map), 1, "Sequential map is required");

  for(i = 0, sz = sizeof(map); i < sz; i++) {
    if(sizeof(map[i]) >= 2 && map[i][0] == key) {
      eject(ref map, i);
      return 1;
    }
  }

  return 0;
}

/**
 * Removes all key-value pairs from a sequential map.
 * This mirrors JavaScript Map.clear() behavior.
 *
 * @param {mixed ref*} map - The sequential map to clear
 *
 * @example
 * ```c
 * mixed *usermap = ({{"alice", 25}, {"bob", 30}});
 * seq_clear(ref usermap); // Returns 0, map becomes ({})
 * ```
 */
void seq_clear(mixed ref *map) {
  assert_arg(valid_seq(map), 1, "Sequential map is required");

  map = ({});
}

/**
 * Gets all keys from a sequential map.
 * This mirrors JavaScript Map.keys() behavior.
 *
 * @param {mixed*} map - The sequential map to extract keys from
 * @return {mixed*} Array of all keys
 *
 * @example
 * ```c
 * mixed *usermap = ({{"alice", 25}, {"bob", 30}});
 * mixed *keys = seq_keys(usermap); // Returns ({"alice", "bob"})
 * ```
 */
mixed *seq_keys(mixed *map) {
  assert_arg(valid_seq(map), 1, "Sequential map is required");

  return map(map, (: $1[0] :));
}

/**
 * Gets all values from a sequential map.
 * This mirrors JavaScript Map.values() behavior.
 *
 * @param {mixed*} map - The sequential map to extract values from
 * @return {mixed*} Array of all values
 *
 * @example
 * ```c
 * mixed *usermap = ({{"alice", 25}, {"bob", 30}});
 * mixed *values = seq_values(usermap); // Returns ({25, 30})
 * ```
 */
mixed *seq_values(mixed *map) {
  assert_arg(valid_seq(map), 1, "Sequential map is required");

  return map(map, (: $1[1] :));
}

/**
 * Converts a mapping to a sequential map (array of key-value pairs).
 * This utility function creates a sequential map from an existing LPC
 * mapping, preserving all key-value associations. The resulting sequential
 * map maintains insertion order and can be used with the seq_* family of
 * functions.
 *
 * @param {mapping} m The mapping to convert
 * @return {mixed*} A sequential map containing all key-value pairs from the mapping
 *
 * @example
 * ```c
 * mapping stats = (["hp":100, "sp":50, "ep":75]);
 * mixed *seq = seq_from_mapping(stats);
 * // Results in: ({{"hp", 100}, {"sp", 50}, {"ep", 75}})
 *
 * // Can now use sequential map functions
 * seq_set(ref seq, "mp", 25);
 * int hp = seq_get(seq, "hp"); // Returns 100
 * ```
 */
mixed *seq_from_mapping(mapping m) {
  mixed *seq = ({});

  assert_arg(mapp(m), 1, "Source must be a mapping.");

  each(m, (: seq_set(ref $4, $1, $2) :), ref seq);

  return seq;
}

/**
 * Converts a sequential map to a mapping.
 * This utility function creates an LPC mapping from a sequential map,
 * converting the array of key-value pairs back to the standard mapping format.
 * This is useful for interoperability with existing code that expects
 * mappings.
 *
 * @param {mixed*} map The sequential map to convert (array of key-value pairs)
 * @return {mapping} A mapping containing all key-value pairs from the sequential map
 *
 * @example
 * ```c
 * mixed *seq = ({{"name", "sword"}, {"damage", 15}, {"weight", 5}});
 * mapping item_data = seq_to_mapping(seq);
 * // Results in: (["name":"sword", "damage":15, "weight":5])
 *
 * // Can now use standard mapping operations
 * string name = item_data["name"]; // Returns "sword"
 * item_data["enchanted"] = 1;
 * ```
 */
mapping seq_to_mapping(mixed *map) {
  assert_arg(valid_seq(map), 1, "Sequential map is required");

  return allocate_mapping(seq_keys(map), seq_values(map));
}

/**
 * @deprecated Use valid_seq() instead.
 * Legacy wrapper for valid_seq().
 *
 * @param {mixed} map - The array to validate
 * @param {int} [non_empty] - If true, requires all elements to be
 *                             key-value pairs
 * @returns {int} 1 if valid, 0 otherwise
 */
varargs int valid_smap(mixed map, int non_empty) {
  return valid_seq(map, non_empty);
}

/**
 * @deprecated Use seq_set() instead.
 * Legacy wrapper for seq_set().
 *
 * @param {mixed ref*} map - The sequential map to modify
 * @param {mixed} key - The key to set
 * @param {mixed} value - The value to associate with the key
 * @returns {int} The size of the sequential map after the operation
 */
int smap_set(mixed ref *map, mixed key, mixed value) {
  return seq_set(map, key, value);
}

/**
 * @deprecated Use seq_get() instead.
 * Legacy wrapper for seq_get().
 *
 * @param {mixed*} map - The sequential map to search
 * @param {mixed} key - The key to look for
 * @returns {mixed} The value associated with the key, or null if not
 *                   found
 */
mixed smap_get(mixed *map, mixed key) {
  return seq_get(map, key);
}

/**
 * @deprecated Use seq_has() instead.
 * Legacy wrapper for seq_has().
 *
 * @param {mixed*} map - The sequential map to search
 * @param {mixed} key - The key to look for
 * @returns {int} 1 if key exists, 0 otherwise
 */
int smap_has(mixed *map, mixed key) {
  return seq_has(map, key);
}

/**
 * @deprecated Use seq_delete() instead.
 * Legacy wrapper for seq_delete().
 *
 * @param {mixed ref*} map - The sequential map to modify
 * @param {mixed} key - The key to remove
 * @returns {int} 1 if key was found and removed, 0 if not
 */
int smap_delete(mixed ref *map, mixed key) {
  return seq_delete(map, key);
}

/**
 * @deprecated Use seq_clear() instead.
 * Legacy wrapper for seq_clear().
 *
 * @param {mixed ref*} map - The sequential map to clear
 */
void smap_clear(mixed ref *map) {
  seq_clear(map);
}

/**
 * @deprecated Use seq_keys() instead.
 * Legacy wrapper for seq_keys().
 *
 * @param {mixed*} map - The sequential map to extract keys from
 * @returns {mixed*} Array of all keys
 */
mixed *smap_keys(mixed *map) {
  return seq_keys(map);
}

/**
 * @deprecated Use seq_values() instead.
 * Legacy wrapper for seq_values().
 *
 * @param {mixed*} map - The sequential map to extract values from
 * @returns {mixed*} Array of all values
 */
mixed *smap_values(mixed *map) {
  return seq_values(map);
}

/**
 * @deprecated Use seq_from_mapping() instead.
 * Legacy wrapper for seq_from_mapping().
 *
 * @param {mapping} m - The mapping to convert
 * @returns {mixed*} A sequential map of key-value pairs
 */
mixed *smap_from_mapping(mapping m) {
  return seq_from_mapping(m);
}

/**
 * @deprecated Use seq_to_mapping() instead.
 * Legacy wrapper for seq_to_mapping().
 *
 * @param {mixed*} map - The sequential map to convert
 * @returns {mapping} A mapping of all key-value pairs
 */
mapping smap_to_mapping(mixed *map) {
  return seq_to_mapping(map);
}
