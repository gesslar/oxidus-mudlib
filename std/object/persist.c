/* Do not remove the headers from this file! see /USAGE for more info. */

/**
 * @file /std/object/persist.c
 * @description Provides persistence capabilities allowing objects to save and
 *              restore their state to/from strings or files.
 *
 * @created Unknown - Beek (Nightmare)
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * Unknown date - Beek - Original implementation for Nightmare
 * Unknown date - Rust - Ported to current system
 * 2025-03-16 - GitHub Copilot - Added documentation
 */

#include <item.h>
#include <object.h>
#include <persist.h>

private nosave int save_recurse;
private nosave mixed *saved_vars = ({ });

/**
 * Marks variables to be included when the object is saved.
 *
 * Variables must be explicitly marked for saving using this function.
 *
 * @param {mixed*} vars - Variable names to mark for saving
 */
protected void save_var(mixed *vars...) {
  saved_vars = distinct_array(({ saved_vars..., vars... }));
}

// ###Security problem here - Beek. What is it used for anyway?
/**
 * Returns the list of variables that will be saved.
 *
 * @returns {string*} Array of variable names marked for saving
 */
public string *get_saved_vars() { return copy(saved_vars); }

/**
 * Sets whether saving should recursively include inventory items.
 *
 * @param {int} flag - 1 to enable recursive saving, 0 to disable
 */
protected void set_save_recurse(int flag) { save_recurse = flag; }

/**
 * Saves object state to a string representation.
 *
 * Creates a mapping with special keys:
 * - #vars#: Contains all saved variables and their values
 * - #base_name#: The object's base name
 * - #inventory#: Inventory objects (if recursep is true)
 *
 * @param {int} recursep - Whether to recursively save inventory objects
 * @returns {string} Serialized object state
 */
varargs string save_to_string(int recursep) {
  string *elements, element;
  mapping map = ([]);
  mapping tmp;

  event(({ this_object() }), "saving");

//### setting a property based on a function arg?  Gross.
  set_save_recurse(recursep);

  if(sizeof(elements = distinct_array(copy(saved_vars)) - ({ 0 }))) {
    tmp = ([ ]);

    foreach(element in elements)
      tmp[element] = fetch_variable(element);

    if(sizeof(tmp))
      map["#vars#"] = tmp;
  }

  map["#base_name#"] = base_name();

  /** @type {STD_ITEM*} */ object *obs = all_inventory();

  if(save_recurse)
    map["#inventory#"] = obs->save_to_string(1) - ({ 0 });

  return save_variable(map);
}

/**
 * Loads object state from a string representation.
 *
 * Restores variables, creates inventory items, and moves them
 * into the appropriate containers.
 *
 * @param {mixed} value - String representation or mapping of object state
 * @param {int} recurse - Recursion level (used internally)
 * @throws If object creation fails
 */
void load_from_string(mixed value, int recurse) {
  mixed data;
  mixed val;
  string key;
  string obj;
  object ob;

  if(!mapp(value))
    data = restore_variable(value);
  else
    data = value;

  foreach(key, val in data) {
    if(key == "#vars#") {
      string var;
      mixed vval;

      foreach(var, vval in val) {
        if(member_array(var, saved_vars) != -1)
          store_variable(var, vval);
        else
          continue;
      }
    }
  }

  if(data["#inventory#"]) {
    foreach(obj in data["#inventory#"]) {
      mixed result;

      val = restore_variable(obj);
      result = catch(ob = new(val["#base_name#"]));
      if(intp(result) && result == 0)
        ob->load_from_string(val, recurse + 1);
      else
        throw(result);
    }
  }

  // We don't want to move the object we're restoring into the
  // previous_object() (happens at the end)...  When you restored the body,
  // the body moved into the user object, but couldn't move out! -- Rust
  if(recurse > 1) {
    if(move(previous_object())) {
      object env = environment(previous_object());
      if(env) {
        if(move(env))
          remove();
        else
          remove();
      }
    }
  }

  event(({this_object()}), "restored");
}
