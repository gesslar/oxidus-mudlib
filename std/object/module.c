/**
 * @file /std/object/module.c
 *
 * Module management for any object. Allows attaching and
 * detaching cloned module objects that provide additional
 * behaviour.
 *
 * Modules are registered under their `query_name()` (i.e. the
 * `moduleName` they advertise in setup), not their file path.
 * A module may opt into multi-instance attachment by overriding
 * `allows_multi()` to return 1; in that case the registry value
 * for that name is an array of objects.
 *
 * @created 2024-07-29 - Gesslar
 * @last_modified 2026-05-01 - Gesslar
 *
 * @history
 * 2024-07-29 - Gesslar - Created
 * 2026-03-29 - Gesslar - Backported from Thresh to object level
 * 2026-05-01 - Gesslar - Key by query_name(); allow_multi support
 */

#include "include/module.h"

void remove_all_modules();

/**
 * Registry of attached modules keyed by `query_name()`. Values
 * are either a single module object or an array of objects for
 * multi-instance modules.
 *
 * @type {([ string: STD_MODULE_BASE | STD_MODULE_BASE* ])}
 */
private nosave mapping __modules = ([]);

/**
 * Destruct callback bound via `addDestruct()` so that all
 * attached modules are detached and removed when this object is
 * destructed.
 *
 * @type {function}
 */
private nosave function onDestructFunction =
  (: remove_all_modules :);

/**
 * Asserts that a module name argument is a non-empty string.
 *
 * @param {string} moduleName - The candidate module name.
 * @param {int} index - The argument position used in the
 *                      `assert_arg` failure message.
 * @errors If `moduleName` is not a non-empty string.
 */
private void validModuleName(string moduleName, int index) {
  assert_arg(
    stringp(moduleName) && strlen(moduleName),
    index,
    "`moduleName` must be a non-empty string."
  );
}

/**
 * Asserts that a function name argument is a non-empty string.
 *
 * @param {string} functionName - The candidate function name.
 * @param {int} index - The argument position used in the
 *                      `assert_arg` failure message.
 * @errors If `functionName` is not a non-empty string.
 */
private void validFunctionName(
  string functionName, int index
) {
  assert_arg(
    stringp(functionName) && strlen(functionName),
    index,
    "`functionName` must be a non-empty string."
  );
}

/**
 * Detaches a module and destructs it. Both the `detach()` and
 * `remove()` calls are wrapped in nested catches so that a
 * failure in either one does not prevent the destruct from
 * happening.
 *
 * @param {STD_MODULE_BASE} mod - The module to detach and
 *                                destruct. Non-objects are
 *                                ignored.
 */
private void detachAndDestruct(object mod) {
  if(!objectp(mod))
    return;

  catch {
    catch {
      mod->detach();
      mod->remove();
    };

    if(mod)
      destruct(mod);
  };
}

/**
 * Adds a module to this object by cloning the specified module
 * file and attaching it. The module is registered under its
 * `query_name()`, not its file path.
 *
 * If no module is registered under that name, the entry is
 * created as an array when the module's `allows_multi()`
 * returns true and as a single object otherwise. If a
 * multi-instance array already exists, the new module is
 * appended. If a single-instance entry already exists, the new
 * clone is discarded and 0 is returned.
 *
 * @param {string} moduleFile - Path to the module file, without
 *                              leading "/" or trailing ".c"
 * @param {mixed} [args] - Additional arguments passed to the
 *                         module's attach/start_module
 * @returns {STD_MODULE_BASE} The attached module object, or 0
 *                            on failure
 * @errors If the module file does not exist
 * @errors If the module fails to load
 */
public varargs object add_module(
  string moduleFile, mixed args...
) {
  validModuleName(moduleFile, 1);

  string path = append(moduleFile, ".c");
  path = prepend(path, "/");
  path = replace_string(path, " ", "_");

  assert(file_exists(path),
    "Module " + path + " does not exist.");

  /** @type {STD_MODULE_BASE} */
  object mod;

  string e = catch(mod = new(path));
  if(e)
    error("Module " + moduleFile +
      " failed to load with error: " + e);

  string name = mod->query_name();
  mixed existing = __modules[name];

  if(!nullp(existing) && !pointerp(existing)) {
    mod->remove();
    return 0;
  }

  e = catch {
    int result = mod->attach(this_object(), args...);

    if(result == 0) {
      mod->remove();
      return 0;
    }
  };

  if(e) {
    mod->remove();
    return 0;
  }

  if(pointerp(existing))
    __modules[name] = array_push(ref __modules, mod);
  else if(mod->allows_multi())
    __modules[name] = ({ mod });
  else
    __modules[name] = mod;

  call_if(this_object(), "addDestruct", (:onDestructFunction:));

  return mod;
}

/**
 * Returns the module entry for the given module name. For
 * single-instance modules this is the module object; for
 * multi-instance modules it is an array of module objects.
 *
 * @param {string} moduleName - The module's `query_name()`
 * @returns {STD_MODULE_BASE | STD_MODULE_BASE *} The module
 *          entry, or 0 if not found
 */
public mixed query_module(string moduleName) {
  validModuleName(moduleName, 1);

  if(nullp(__modules[moduleName]))
    return 0;

  return __modules[moduleName];
}

/**
 * Removes all modules registered under the given name by
 * detaching and destructing each. For multi-instance modules,
 * every attached instance is removed.
 *
 * @param {string} moduleName - The module's `query_name()`
 * @returns {int} 1 if removed, 0 if not found
 */
public int remove_module(string moduleName) {
  validModuleName(moduleName, 1);

  mixed entry = query_module(moduleName);

  if(!entry)
    return 0;

  if(pointerp(entry))
    foreach(object mod in entry)
      detachAndDestruct(mod);
  else
    detachAndDestruct(entry);

  map_delete(__modules, moduleName);

  return 1;
}

/**
 * Returns a copy of all attached modules.
 *
 * @returns {([ string: STD_MODULE_BASE | STD_MODULE_BASE * ])}
 *          Mapping of module names to module objects (or arrays
 *          of objects for multi-instance modules)
 */
public mapping query_modules() {
  return copy(__modules);
}

/**
 * Calls a function on the module(s) registered under the given
 * name and returns the result. For multi-instance modules, the
 * function is called on every instance and an array of results
 * is returned.
 *
 * @param {string} moduleName - The module's `query_name()`
 * @param {string} functionName - The function to call on the
 *                                module
 * @param {mixed} [args] - Arguments to pass to the function
 * @returns {mixed} The result of the function call, an array of
 *                  results for multi-instance modules, or null
 *                  if no module is registered under that name
 */
public varargs mixed module(
  string moduleName, string functionName, mixed args...
) {
  validModuleName(moduleName, 1);
  validFunctionName(functionName, 2);

  mixed entry = query_module(moduleName);

  if(!entry)
    return null;

  if(pointerp(entry)) {
    mixed *results = ({});

    foreach(object mod in entry) {
      mixed r;
      catch(r = call_if(mod, functionName, args...));
      results += ({ r });
    }

    return results;
  }

  mixed result;
  catch(result = call_if(entry, functionName, args...));

  return result;
}

/**
 * Removes all attached modules. Called automatically when the
 * object is destructed.
 */
public void remove_all_modules() {
  foreach(string moduleName, mixed _ in __modules)
    remove_module(moduleName);
}
