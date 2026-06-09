/**
 * @file /std/object/command.c
 *
 * Replacement for add_action, allowing for mixed results and
 * more flexible command handling.
 *
 * @created 2024-03-03 - Gesslar
 * @last_modified 2026-03-29 - Gesslar
 *
 * @history
 * 2024-03-03 - Gesslar - Created
 * 2025-03-16 - GitHub Copilot - Added documentation
 * 2026-03-29 - Gesslar - Updated documentation to LPCDoc standards
 */

#include "/std/living/include/pager.h"
#include <command.h>

private nosave mapping __cmdHandlers = ([]);
private string *__cmdPaths = ({});
private nosave string *__cmdHistory = ({});

/**
 * Adds a command handler to this object.
 *
 * @param {string | string*} command - Command name or array of
 *                                     command names
 * @param {function | string} action - Function pointer or method
 *                                     name to handle command
 * @errors If action function does not exist
 * @errors If command or action parameters are invalid types
 */
public void add_command(mixed command, mixed action) {
  if(stringp(command)) {
    remove_command(command);
    if(stringp(action)) {
      if(!function_exists(action))
        error("No such function " + action + " in " + file_name() + ".\n");

      __cmdHandlers[command] = action;
    } else if(valid_function(action)) {
      __cmdHandlers[command] = action;
    } else {
      error("Illegal action " + action + " in " + file_name() + ".\n");
    }
  } else if(pointerp(command)) {
    foreach(mixed cmd in command)
      add_command(cmd, action);
  } else {
    error("Illegal command " + command + " in " + file_name() + ".\n");
  }
}

/**
 * Removes one or more commands from this object.
 *
 * @param {string | string*} command - Command name or array of
 *                                     command names
 */
public void remove_command(mixed command) {
  if(stringp(command)) {
    map_delete(__cmdHandlers, command);
  } else if(pointerp(command)) {
    foreach(mixed cmd in command)
      if(stringp(cmd))
        map_delete(__cmdHandlers, cmd);
  }
}

/**
 * Removes all commands that use a specific action handler.
 *
 * @param {function | string} action - Function name or pointer
 *                                     to remove
 */
public void remove_command_all(mixed action) {
  foreach(mixed command, mixed act in __cmdHandlers) {
    if(act == action) {
      map_delete(__cmdHandlers, command);
      remove_command(command);
    }
  }
}

/**
 * Returns the action associated with a command.
 *
 * @param {string} command - The command to query
 * @returns {function | string | undefined} Action handler, or
 *          undefined if not found
 */
public mixed query_command(string command) {
  return __cmdHandlers[command];
}

/**
 * Returns all registered commands and their actions.
 *
 * @returns {mapping} Copy of the commands mapping
 */
public mapping query_commands() {
  return copy(__cmdHandlers);
}

/**
 * Returns all commands available to this object, including
 * inherited ones from the driver.
 *
 * @returns {string*} Array of all available command names
 */
public string *query_all_commands() {
  return commands();
}

/**
 * Finds all commands that share the same action handler.
 * Returns an array containing the given command and any other
 * commands that use the same action handler.
 *
 * @param {string} command - The command to find matches for
 * @returns {string*} Array of commands sharing the same
 *          handler, or empty array
 */
public string *query_matching_commands(string command) {
  string *matches = allocate(1);

  if(nullp(__cmdHandlers[command]))
    return ({});

  matches[0] = command;

  foreach(mixed cmd, mixed action in __cmdHandlers) {
    if(cmd == matches[0])
      continue;

    if(action == __cmdHandlers[command])
      matches += ({ cmd });
  }

  return matches;
}

/**
 * Initialises the commands mapping to an empty state.
 */
public void init_commands() {
  __cmdHandlers = ([]);
}

/**
 * Evaluates a command by calling its associated action.
 *
 * @param {STD_PLAYER} user - The object triggering the command
 * @param {string} command - The command name
 * @param {string} arg - The command arguments
 * @returns {mixed} Result of command evaluation, or undefined
 *          if no handler
 */
public mixed evaluate_command(object user, string command, string arg) {
  if(stringp(__cmdHandlers[command]))
    return call_other(this_object(), __cmdHandlers[command], user, arg);

  if(!valid_function(__cmdHandlers[command]))
    return null;

  function action = __cmdHandlers[command];

  return action(user, arg);
}

/**
 * Driver apply for processing input before command parsing.
 *
 * @apply
 * @param {string} arg - The raw input string
 * @returns {string} The processed input string
 */
string process_input(string arg) {
  return arg;
}

/**
 * Returns the current command search paths.
 *
 * @returns {string*} Copy of the command paths array
 */
public string *get_path() {
  return copy(__cmdPaths);
}

/**
 * Adds a directory to the command search path. Requires admin
 * privileges or that the caller is this body.
 *
 * @param {string} str - The directory path to add
 * @returns {int} 1 on success, 0 on failure
 */
public int add_path(string str) {
  if(!adminp(previous_object()) && this_body() != this_object())
    return 0;

  if(includes(__cmdPaths, str))
    return 0;

  str = append(str, "/");

  if(!directory_exists(str))
    return 0;

  __cmdPaths = __cmdPaths || ({});
  push(ref __cmdPaths, str);

  return 1;
}

/**
 * Sets the command search paths from a colon-delimited string
 * or an array of path strings.
 *
 * @param {string | string*} path - Colon-delimited path string
 *                                  or array of paths
 */
public void set_path(mixed path) {
  string *paths;

  if(stringp(path))
    paths = explode(path, ":");
  else if(pointerp(path))
    paths = copy(path);

  filter(paths, (: add_path :));
}

/**
 * Removes a directory from the command search path. Requires
 * admin privileges or that the caller is this body.
 *
 * @param {string} str - The directory path to remove
 * @returns {int} 1 on success, 0 on failure
 */
public int rem_path(string str) {
  if(!adminp(previous_object()) && this_body() != this_object())
    return 0;

  if(!includes(__cmdPaths, str))
    return 0;

  __cmdPaths -= ({str});

  return 1;
}

/**
 * Adds wizard-specific command paths from the wizard paths
 * configuration file.
 */
public void add_wizard_path() {
  string *paths = explode_file("/adm/etc/wizard_paths");

  filter(paths, (: add_path :));
}

/**
 * Removes all current command paths and restores standard
 * paths only.
 */
public void remove_wizard_paths() {
  filter(get_path(), (: rem_path :));

  add_standard_paths();
}

/**
 * Adds the standard command paths from the standard paths
 * configuration file.
 */
public void add_standard_paths() {
  string *paths = explode_file("/adm/etc/standard_paths");

  filter(paths, (: add_path :));
}

/**
 * Adds ghost-specific command paths from the ghost paths
 * configuration file.
 */
public void add_ghost_paths() {
  string *paths = explode_file("/adm/etc/ghost_paths");

  filter(paths, (: add_path :));
}

/**
 * Returns command history entries. Requires the caller to be
 * the owning body or an admin.
 *
 * @param {int} [index] - Starting index into the history
 * @param {int} [range] - End index for a range of entries
 * @returns {string*} Array of command history entries
 */
public nomask varargs string *query_command_history(int index,
    int range) {
  if(this_body() != this_object()
      && !adminp(previous_object()))
    return ({});

  if(!index)
    return __cmdHistory + ({});
  else if(range)
    return __cmdHistory[index..range] + ({});
  else
    return ({ __cmdHistory[index] });
}

/**
 * Main command dispatch hook. Resolves and executes commands
 * by checking object handlers, emotes, channels, and command
 * paths in order.
 *
 * @param {string} arg - The raw command arguments
 * @returns {int} 1 if the command was handled, 0 otherwise
 */
public int command_hook(string arg) {
  string verb, err, *cmds = ({});

  object
  /** @type {STD_BODY}    */  caller,
  /** @type {STD_CMD}     */  cmd,
  /** @type {STD_ITEM}    */  ob,
  /** @type {STD_ITEM*}   */ *obs;
  int sz;
  mixed result;
  string complete;
  mixed return_value;

  caller = this_body();

  if(interactive(caller))
    if(caller != this_object())
      return 0;

  verb = query_verb();

  if(arg == "")
    arg = 0;

  verb = lower_case(verb);

  if(arg)
    complete = sprintf("%s %s", verb, arg);
  else
    complete = verb;

  string aliased = ALIAS_D->resolve_alias(complete, this_object());

  if(aliased) {
    arg = 0;

    sscanf(aliased, "%s %s", verb, arg) || verb = aliased;
  }

  obs = all_inventory();
  if(environment())
    obs += ({ environment() }) + all_inventory(environment());

  // This object should be parsed last.
  obs += ({ this_object() });

  foreach(ob in obs) {
    result = ob->evaluate_command(this_object(), verb, arg);
    result = evaluate_result(result);
    if(result == 1)
      return 1;
  }

  if(arg)
    __cmdHistory += ({ verb + " " + arg });
  else
    __cmdHistory += ({ verb });

  // Communication checks
  catch {
    // err = catch(load_object(CHAN_D));
    // if(!err) {
      // if(CHAN_D->chat(verb, query_privs(), arg))
      //   return 1;
    // }
  };

  cmds = map(__cmdPaths, (: $1 + $(verb) + ".c" :));
  cmds = filter(cmds, (: file_exists :));

  sz = sizeof(cmds);
  if(sz > 0) {
    if(sz > 1) {
      tell("Ambiguous command.\n");
      return 1;
    }

    /** @type {STD_ROOM} */ object room = environment();

    if(room && room->valid_exit(verb)) {
      arg = verb;
      verb = "go";
    }

    err = catch(cmd = load_object(cmds[0]));

    if(err) {
      tell_me("Error: Command " + verb +
        " non-functional.\n");
      tell_me(err);
      return 1;
    }

    return_value = cmd->main(caller, arg);

    result = evaluate_result(return_value);
    if(result == 1)
      return 1;

    return return_value;
  }

  return 0;
}

/**
 * Finds the full file path for a command verb by searching
 * the command paths.
 *
 * @param {string} verb - The command verb to look up
 * @returns {string | undefined} The full path to the command
 *          file without extension, or undefined if not found
 */
public string find_command_path(string verb) {
  string *paths = get_path();
  string path;

  foreach(string p in paths) {
    if(file_exists(p + verb + ".c")) {
      path = p + verb;
      break;
    }
  }

  return path;
}

private nomask int evaluate_result(mixed result) {
  if(stringp(result)) {
    if(!strlen(result)) {
      return 0;
    } else {
      result = append(result, "\n");
      page(result);
      return 1;
    }
  } else if(pointerp(result)) {
    if(!sizeof(result)) {
      return 0;
    } else {
      page(result);
      return 1;
    }
  }

  return result;
}

/**
 * Forces this object to execute a command. Requires the caller
 * to be this body or an admin.
 *
 * @param {string} cmd - The command string to execute
 * @returns {int} Result of the command execution, or 0 if
 *          permission denied
 */
public int force_me(string cmd) {
  if(this_body() != this_object()
      && !adminp(previous_object())
      && !adminp(this_caller()))
    return 0;
  else
    return command(cmd);
}
