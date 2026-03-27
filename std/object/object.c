/**
 * @file /std/object/object.c
 * @description Base object implementation that provides core functionality
 *              for all game objects.
 *
 * @created 2005-04-04 - Tacitus
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * 2005-04-04 - Tacitus - Created
 * 2006-07-14 - Tacitus - Last updated
 * 2025-03-16 - GitHub Copilot - Added documentation
 */

#include <object.h>
#include <gmcp_defines.h>

inherit __DIR__ "description";
inherit __DIR__ "cooldown";
inherit __DIR__ "heartbeat";
inherit __DIR__ "id";
inherit __DIR__ "init";
inherit __DIR__ "persist";
inherit __DIR__ "setup";
inherit __DIR__ "stats";
inherit __DIR__ "weight";

inherit M_CLEAN;
inherit M_MESSAGING;

private nomask nosave function *_destruct_functions = ({});
private nomask nosave function *_reset_functions = ({});
private string real_name, short, long;
private nosave string _name;
protected nosave mixed *_create_args = ({});
private nosave string _virtual_master = 0;
private nosave object _last_location = 0;
private string _last_location_string = "";
private nosave mixed _prevent_get = 0;

/**
 * Driver apply called when the object is created.
 *
 * Sets up the object and processes the setup chain, then calls reset().
 * This function is private so only the driver can call it.
 *
 * @param {mixed} args... - Variable arguments passed during creation
 */
private varargs void create(mixed args...) {
  set_notify_destruct(1);

  _create_args = args;
  if(!real_name) {
    if(_name)
      set_real_name(_name);
    else
      _name = null;
  }

  setup_chain(_create_args...);
  reset();
}

/**
 * Initialises a virtual object.
 *
 * Driver apply called after the object is created to set up virtual object
 * properties and runs the virtual setup chain.
 */
void virtual_start() {
  virtual_setup_chain(_create_args...);
  reset();
}

/**
 * Driver apply called periodically based on the 'time to reset'
 * runtime configuration value.
 *
 * Calls reset_objects() if it exists, then processes
 * all registered reset functions.
 */
varargs void reset() {
  call_if(this_object(), "reset_objects");
  process_reset();
}

/**
 * Removes this object from the game.
 *
 * Handles recursive inventory management, moving contained objects
 * to the environment when possible, or destructing them otherwise.
 * Non-player objects are removed completely.
 *
 * @returns {int} Always returns 1
 */
int remove() {
  object *obs, ob, env = environment();

  catch(call_if(this_object(), "removing", env));

  /** @type {STD_ITEM} */ ob = first_inventory();
  while(ob) {
    int result;

    /** @type {STD_ITEM} */ object next = next_inventory(ob);

    if(env) {
      result = ob->move(env);
      if(!result) {
        ob = next;
        continue;
      }
    }

    if(!userp(ob)) {
      ob->remove();
      if(objectp(ob))
        destruct(ob);
    }

    ob = next;
  }

  obs = all_inventory(this_object());
  obs = filter(obs, (: !userp($1) :));

  if(sizeof(obs))
    filter(obs, (: destruct :));

  destruct();

  return 1;
}

/**
 * Sets the internal real name of the object.
 *
 * For interactive objects, this can only be called by admins or the object itself.
 * For living objects, this also sets the living name.
 *
 * @param {string} str - The real name to set
 * @returns {string} The name that was set, or 0 if failed
 */
string set_real_name(string str) {
  assert_arg(stringp(str) && truthy(str), 1, "Invalid argument");

  if(interactive(this_object()) &&
     !is_member(query_privs(previous_object()), "admin") &&
     previous_object() != this_object())
    return 0;

  real_name = lower_case(str);

  if(living())
    set_living_name(real_name);

  rehash_ids();

  return real_name;
}

/**
 * Returns the internal real name of the object.
 *
 * @returns {string} The real name
 */
string query_real_name() {
  return real_name;
}

/**
 * Sets the display name of the object.
 *
 * If valid_user() returns true, the name is capitalized.
 * If no real_name exists, this also sets the real_name.
 *
 * @param {string} str - The name to set
 * @returns {string} The name that was set, or 0 if failed
 */
string set_name(string str) {
  string result;

  assert_arg(stringp(str) && truthy(str), 1, "Invalid argument");

  if(valid_user(lower_case(str)))
    _name = capitalize(lower_case(str));
  else
    _name = str;

  if(!real_name) {
    result = set_real_name(_name);

    if(!result)
      return 0;
  }

  rehash_ids();

  return _name;
}

/**
 * Returns the display name of the object.
 *
 * @returns {string} The name
 */
string query_name() {
  return _name;
}

/**
 * Resolves a relative path against the object's directory.
 *
 * @param {string} path - The path to resolve
 * @returns {string} The absolute path
 */
string find_path(string path) {
  assert_arg(stringp(path) && truthy(path), 1, "Invalid argument");
  return resolve_path(query_directory(), path);
}

/**
 * Sets the virtual master for this object.
 *
 * Used for virtual object management.
 *
 * @param {string} str - The virtual master path
 */
void set_virtual_master(string str) {
  assert_arg(stringp(str) && truthy(str), 1, "Invalid argument");
  _virtual_master = str;
}

/**
 * Returns the virtual master for this object.
 *
 * @returns {string} The virtual master path
 */
string query_virtual_master() {
  return _virtual_master;
}

/**
 * Called by the driver before the object is destructed.
 *
 * Updates capacity and mass of the environment, triggers GMCP events,
 * and runs the destruct chain and functions.
 */
void on_destruct() {
  /** @type {STD_ITEM} */ object env = environment();

  if(env) {
    if(!env->ignore_capacity())
      env->adjust_fill(-query_mass());
    if(!env->ignore_mass())
      env->adjust_mass(query_mass());

    event(env, "gmcp_item_remove", env);
  }

  process_destruct();
  unsetup_chain();
}

/**
 * Registers a function to be called during object reset.
 *
 * @param {function} f - The function to call
 */
void add_reset(function f) {
  assert_arg(valid_function(f), 1, "Invalid argument");

  if(!of(f, _reset_functions))
    _reset_functions += ({ f });
}

/**
 * Removes a previously registered reset function.
 *
 * @param {function} f - The function to remove
 */
void remove_reset(function f) {
  assert_arg(valid_function(f), 1, "Invalid argument");

  if(of(f, _reset_functions))
    _reset_functions -= ({ f });
}

/**
 * Processes all registered reset functions.
 *
 * Called during object reset.
 */
void process_reset() {
  foreach(function f in _reset_functions)
    if(valid_function(f))
      catch(f());
}

/**
 * Returns all registered reset functions.
 *
 * @returns {function*} Array of reset functions
 */
function *query_reset_functions() {
  return _reset_functions;
}

/**
 * Registers a function to be called during object destruction.
 *
 * @param {function} f - The function to call
 * @returns {int} 1 if successfully added, 0 if not
 */
int add_destruct(function f) {
  assert_arg(valid_function(f), 1, "Invalid argument.");

  if(!of(f, _destruct_functions)) {
    _destruct_functions += ({ f });
    return 1;
  }

  return 0;
}

/**
 * Removes a previously registered destruct function.
 *
 * @param {function} f - The function to remove
 * @returns {int} 1 if successfully removed, 0 if not
 */
int remove_destruct(function f) {
  assert_arg(valid_function(f), 1, "Invalid argument.");

  if(of(f, _destruct_functions)) {
    _destruct_functions -= ({ f });
    return 1;
  }

  return 0;
}

/**
 * Sets the last location of this object.
 *
 * Stores both the object reference and its filename.
 *
 * @param {object} ob - The location object
 */
void set_last_location(object ob) {
  assert_arg(objectp(ob), 1, "Invalid argument.");

  _last_location = ob;
  _last_location_string = file_name(ob);
}

/**
 * Returns the last location object.
 *
 * @returns {object} The last location object
 */
object last_location() {
  return _last_location;
}

/**
 * Returns the filename of the last location.
 *
 * @returns {string} The last location filename
 */
string query_last_location() {
  return _last_location_string;
}

/**
 * Processes all registered destruct functions.
 *
 * Called during object destruction.
 */
void process_destruct() {
  foreach(function f in _destruct_functions)
    catch(f());
}

/**
 * Sets whether this object can be picked up.
 *
 * @param {int|function} val - Boolean or function to determine if pickup is prevented
 */
void set_prevent_get(mixed val) {
  assert_arg(valid_function(val) || (!nullp(val) && intp(val)), 1, "Invalid argument.");

  if(intp(val))
    _prevent_get = !!val;

  _prevent_get = val;
}

/**
 * Checks if this object can be picked up.
 *
 * If _prevent_get is a function, it will be called with this object.
 *
 * @returns {int} 1 if pickup is prevented, 0 if allowed
 */
int query_prevent_get() {
  int result;

  if(!valid_function(_prevent_get))
    return _prevent_get;

  function f = _prevent_get;

  return f(this_object());
}
