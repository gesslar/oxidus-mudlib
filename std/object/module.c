/**
 * @file /std/object/module.c
 *
 * Module management for any object. Allows attaching and
 * detaching cloned module objects that provide additional
 * behaviour.
 *
 * @created 2024-07-29 - Gesslar
 * @last_modified 2026-03-29 - Gesslar
 *
 * @history
 * 2024-07-29 - Gesslar - Created
 * 2026-03-29 - Gesslar - Backported from Thresh to object level
 */

#include "include/module.h"

void remove_all_modules();

/** @type {([ string: STD_MODULE_BASE ])} */
private nosave mapping __modules = ([]);
private nosave function onDestructFunction =
  (: remove_all_modules :);

private void validModuleFileName(string moduleFile, int index) {
  assert_arg(
    stringp(moduleFile) && strlen(moduleFile),
    index,
    "`moduleFile` must be a non-empty string."
  );
}

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
 * Adds a module to this object by cloning the specified module
 * file and attaching it.
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
  validModuleFileName(moduleFile, 1);

  if(!nullp(__modules[moduleFile]))
    return ([])[0];

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

  __modules[moduleFile] = mod;

  this_object()->add_destruct(onDestructFunction);

  return mod;
}

/**
 * Returns the module object for the given module file path.
 *
 * @param {string} moduleFile - The module file path used when
 *                              adding the module
 * @returns {STD_MODULE_BASE} The module object, or 0 if not
 *                            found
 */
public object query_module(string moduleFile) {
  validModuleFileName(moduleFile, 1);

  if(!__modules[moduleFile])
    return 0;

  return __modules[moduleFile];
}

/**
 * Removes a module from this object by detaching and
 * destructing it.
 *
 * @param {string} moduleFile - The module file path used when
 *                              adding the module
 * @returns {int} 1 if removed, 0 if not found
 */
public int remove_module(string moduleFile) {
  validModuleFileName(moduleFile, 1);

  /** @type {STD_MODULE_BASE} */
  object mod = query_module(moduleFile);

  if(!mod)
    return 0;

  catch {
    catch {
      mod->detach();
      mod->remove();
    };

    if(mod)
      destruct(mod);
  };

  map_delete(__modules, moduleFile);

  return 1;
}

/**
 * Returns a copy of all attached modules.
 *
 * @returns {([ string: STD_MODULE_BASE ])} Mapping of module
 *          file paths to module objects
 */
public mapping query_modules() {
  return copy(__modules);
}

/**
 * Calls a function on an attached module and returns the result.
 *
 * @param {string} moduleFile - The module file path used when
 *                              adding the module
 * @param {string} functionName - The function to call on the
 *                                module
 * @param {mixed} [args] - Arguments to pass to the function
 * @returns {mixed} The result of the function call, or null if
 *                  the module is not found
 */
public varargs mixed module(
  string moduleFile, string functionName, mixed args...
) {
  validModuleFileName(moduleFile, 1);
  validFunctionName(functionName, 2);

  /** @type {STD_MODULE_BASE} */
  object mod = query_module(moduleFile);

  if(!mod)
    return null;

  mixed result;
  catch(result = call_if(mod, functionName, args...));

  return result;
}

/**
 * Removes all attached modules. Called automatically when the
 * object is destructed.
 */
public void remove_all_modules() {
  foreach(string moduleName, object _ in __modules)
    remove_module(moduleName);
}
