/**
 * @file /std/room/exits.c
 * @description Exits are the connections between rooms.
 *
 * @created 2024-09-13 - Gesslar
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * 2024-09-13 - Gesslar - Created
 * 2025-03-16 - GitHub Copilot - Added documentation
 */

#include <exits.h>

private nosave mapping _exits = ([]);
private nosave mapping _pre_exit_funcs = ([]);
private nosave mapping _post_exit_funcs = ([]);

/**
 * Sets the exits for a room, replacing any existing exits.
 *
 * @param {mapping} exit - Mapping of exit directions to destination paths
 * @returns {mapping} The new exits mapping (copy)
 */
mapping set_exits(mapping exit) {
  _exits = exit;

  return query_exits();
}

/**
 * Returns a copy of all exits.
 *
 * @returns {mapping} A copy of the exits mapping
 */
mapping query_exits() {
  return copy(_exits);
}

/**
 * Returns all exit directions/identifiers.
 *
 * @returns {string*} Array of exit direction strings
 */
string *query_exit_ids() {
  return keys(_exits);
}

/**
 * Returns the destination path for a specific exit.
 *
 * If the exit destination is a function, it will be evaluated.
 * Relative paths are resolved against the room's directory.
 *
 * @param {string} id - The exit direction
 * @returns {string} The destination path, or null if exit doesn't exist
 */
string query_exit(string id) {
  if(nullp(_exits[id]))
    return null;

  mixed dest = _exits[id];
  if(stringp(dest))
    return resolve_path(query_directory(), dest);

  if(!valid_function(dest))
    return null;

  function f = dest;

  return resolve_path(query_directory(), f());
}

/**
 * Returns the destination room object for a specific exit.
 *
 * @param {string} id - The exit direction
 * @param {int} [loaded=0] - If true, only returns already loaded objects
 * @returns {object} The destination room object, or null if not found
 */
varargs object query_exit_dest(string id, int loaded) {
  mixed dest = _exits[id];

  if(!dest)
    return null;

  if(valid_function(dest)) {
    function f = dest;
    dest = f();
  }

  if(stringp(dest)) {
    dest = resolve_path(query_directory(), dest);

    if(loaded)
      dest = find_object(dest);
    else
      dest = load_object(dest);
  }

  if(!objectp(dest))
    return null;

  return dest;
}

/**
 * Returns all destination room objects.
 *
 * @param {int} [loaded=0] - If true, only returns already loaded objects
 * @returns {object*} Array of destination objects
 */
varargs object *query_exit_dests(int loaded) {
  object *dests = ({ });
  string *exits = query_exit_ids();

  foreach(string exit in exits) {
    object dest = query_exit_dest(exit, loaded);

    if(objectp(dest))
      dests += ({ dest });
  }

  return dests;
}

/**
 * Checks if an exit exists.
 *
 * @param {string} exit - The exit direction to check
 * @returns {int} 1 if the exit exists, 0 if not
 */
int valid_exit(string exit) {
  return !nullp(_exits[exit]);
}

/**
 * Removes an exit.
 *
 * @param {string} id - The exit direction to remove
 * @returns {mapping} The updated exits mapping (copy)
 */
mapping remove_exit(string id) {
  if(nullp(_exits[id]))
    return query_exits();

  map_delete(_exits, id);

  return query_exits();
}

/**
 * Adds a new exit, replacing any existing exit with the same direction ID.
 *
 * @param {string} id - The exit direction
 * @param {string} path - The destination path
 * @returns {mapping} The updated exits mapping (copy)
 */
mapping add_exit(string id, string path) {
  _exits[id] = path;

  return query_exits();
}

/**
 * Checks if a pre-exit function exists for an exit direction.
 *
 * @param {string} dir - The exit direction
 * @returns {int} 1 if a pre-exit function exists, 0 if not
 */
int has_pre_exit_func(string dir) {
  return !nullp(_pre_exit_funcs[dir]);
}

/**
 * Checks if a post-exit function exists for an exit direction.
 *
 * @param {string} dir - The exit direction
 * @returns {int} 1 if a post-exit function exists, 0 if not
 */
int has_post_exit_func(string dir) {
  return !nullp(_post_exit_funcs[dir]);
}

/**
 * Adds a function to be called before an exit is used.
 *
 * If an existing function is already set for this direction, it is replaced.
 *
 * @param {string} dir - The exit direction
 * @param {mixed} func - Function pointer or method name to call
 */
void add_pre_exit_func(string dir, mixed func) {
  map_delete(_pre_exit_funcs, dir);

  if(!valid_function(dir) && !stringp(func))
    return;

  _pre_exit_funcs[dir] = func;
}

/**
 * Adds a function to be called after an exit is used.
 *
 * If an existing function is already set for this direction, it is replaced.
 *
 * @param {string} dir - The exit direction
 * @param {mixed} func - Function pointer or method name to call
 */
void add_post_exit_func(string dir, mixed func) {
  map_delete(_post_exit_funcs, dir);

  if(!valid_function(dir) && !stringp(func))
    return;

  _post_exit_funcs[dir] = func;
}

/**
 * Removes a pre-exit function.
 *
 * @param {string} dir - The exit direction
 */
void remove_pre_exit_func(string dir) {
  map_delete(_pre_exit_funcs, dir);
}

/**
 * Removes a post-exit function.
 *
 * @param {string} dir - The exit direction
 */
void remove_post_exit_func(string dir) {
  map_delete(_post_exit_funcs, dir);
}

/**
 * Returns the pre-exit function for an exit.
 *
 * @param {string} dir - The exit direction
 * @returns {mixed} Function pointer or method name, or null if none exists
 */
mixed query_pre_exit_func(string dir) {
  return _pre_exit_funcs[dir];
}

/**
 * Returns the post-exit function for an exit.
 *
 * @param {string} dir - The exit direction
 * @returns {mixed} Function pointer or method name, or null if none exists
 */
mixed query_post_exit_func(string dir) {
  return _post_exit_funcs[dir];
}

/**
 * Evaluates the pre-exit function for an exit.
 *
 * @param {string} dir - The exit direction
 * @param {object} who - The object trying to use the exit
 */
void evaluate_pre_exit_func(string dir, object who) {
  if(stringp(_pre_exit_funcs[dir])) {
   catch(call_other(this_object(), _pre_exit_funcs[dir], who));

   return;
  }

  if(valid_function(_pre_exit_funcs[dir])) {
    function f = _pre_exit_funcs[dir];

    return f(who);
  }
}

/**
 * Evaluates the post-exit function for an exit.
 *
 * @param {string} dir - The exit direction
 * @param {object} who - The object that used the exit
 */
void evaluate_post_exit_func(string dir, object who) {
  if(stringp(_post_exit_funcs[dir])) {
    catch(call_other(this_object(), _pre_exit_funcs[dir], who));

    return;
  }

  if(valid_function(_post_exit_funcs[dir])) {
    function f = _post_exit_funcs[dir];

    return f(who);
  }
}
