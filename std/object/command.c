/**
 * @file /std/object/command.c
 * @description Replacement for add_action, allowing for mixed results
 *              and more flexible command handling.
 *
 * @created 2024-03-03 - Gesslar
 * @last_modified 2025-03-29 - Gesslar
 *
 * @history
 * 2024-03-03 - Gesslar - Created
 * 2025-03-16 - GitHub Copilot - Added documentation
 * 2025-03-29 - Gesslar - Converted to camelCase coding standards
 */

#include "/std/living/include/alias.h"
#include "/std/living/include/pager.h"
#include <command.h>

private nosave mapping cmdHandlers = ([]);
private string *cmdPaths = ({});
nosave string *cmdHistory = ({});

/**
 * @description Adds a command handler to this object.
 * @param {string|string*} command - Command name or array of
 *                                   command names
 * @param {function|string} action - Function pointer or method
 *                                   name to handle command
 * @errors If action function does not exist
 * @errors If command or action parameters are invalid types
 */
public void addCommand(mixed command, mixed action) {
  if(stringp(command)) {
    removeCommand(command);
    if(stringp(action)) {
      if(!function_exists(action))
        error("addCommand: No such function " +
          action + " in " + file_name() + ".\n");

      cmdHandlers[command] = action;
    } else if(valid_function(action)) {
      cmdHandlers[command] = action;
    } else {
      error("addCommand: Illegal action " +
        action + " in " + file_name() + ".\n");
    }
  } else if(pointerp(command)) {
    foreach(mixed cmd in command)
      addCommand(cmd, action);
  } else {
    error("addCommand: Illegal command " +
      command + " in " + file_name() + ".\n");
  }
}

/**
 * @description Removes one or more commands from this object.
 * @param {string|string*} command - Command name or array of
 *                                   command names
 */
public void removeCommand(mixed command) {
  if(stringp(command)) {
    map_delete(cmdHandlers, command);
  } else if(pointerp(command)) {
    foreach(mixed cmd in command)
      if(stringp(cmd))
        map_delete(cmdHandlers, cmd);
  }
}

/**
 * @description Removes all commands that use a specific action
 *              handler.
 * @param {function|string} action - Function name or pointer to
 *                                   remove
 */
public void removeCommandAll(mixed action) {
  foreach(mixed command, mixed act in cmdHandlers) {
    if(act == action) {
      map_delete(cmdHandlers, command);
      removeCommand(command);
    }
  }
}

/**
 * @description Returns the action associated with a command.
 * @param {string} command - The command to query
 * @returns {function|string|null} Action handler or null if not
 *                                 found
 */
public mixed queryCommand(string command) {
  return cmdHandlers[command];
}

/**
 * @description Returns all registered commands and their actions.
 * @returns {mapping} Copy of commands mapping
 */
public mapping queryCommands() {
  return copy(cmdHandlers);
}

public string *queryAllCommands() {
  return commands();
}

/**
 * @description Finds all commands that share the same action
 *              handler. Returns an array containing the given
 *              command and any other commands that use the same
 *              action handler.
 * @param {string} command - The command to find matches for
 * @returns {string*} Array of commands sharing the same handler,
 *                    or empty array
 */
public string *queryMatchingCommands(string command) {
  string *matches = allocate(1);

  if(nullp(cmdHandlers[command]))
    return ({});

  matches[0] = command;

  foreach(mixed cmd, mixed action in cmdHandlers) {
    if(cmd == matches[0])
      continue;

    if(action == cmdHandlers[command])
      matches += ({ cmd });
  }

  return matches;
}

/**
 * @description Initialises the commands mapping to an empty state.
 */
public void initCommands() {
  cmdHandlers = ([]);
}

/**
 * @description Evaluates a command by calling its associated
 *              action.
 * @param {STD_PLAYER} user - The object triggering the command
 * @param {string} command - The command name
 * @param {string} arg - The command arguments
 * @returns {mixed} Result of command evaluation, or null if no
 *                  handler
 */
public mixed evaluateCommand(object user, string command,
    string arg) {
  if(stringp(cmdHandlers[command]))
    return call_other(this_object(), command, user, arg);

  if(!valid_function(cmdHandlers[command]))
    return null;

  function action = cmdHandlers[command];

  return action(user, arg);
}

/**
 * @description Driver apply for processing input before command
 *              parsing.
 * @param {string} arg - The raw input string
 * @returns {string} The processed input string
 */
string process_input(string arg) {
  return arg;
}

public string *queryPath() {
  return copy(cmdPaths);
}

public int addPath(string str) {
  if(!adminp(previous_object()) && this_body() != this_object())
    return 0;

  if(includes(cmdPaths, str))
    return 0;

  str = append(str, "/");

  if(!directory_exists(str))
    return 0;

  cmdPaths = cmdPaths || ({});
  push(ref cmdPaths, str);

  return 1;
}

public void setPath(mixed path) {
  string *paths;

  if(stringp(path))
    paths = explode(path, ":");
  else if(pointerp(path))
    paths = copy(path);

  filter(paths, (: addPath :));
}

public int remPath(string str) {
  if(!adminp(previous_object()) && this_body() != this_object())
    return 0;

  if(!includes(cmdPaths, str))
    return 0;

  cmdPaths -= ({str});

  return 1;
}

public void addWizardPaths() {
  string *paths = explode_file("/adm/etc/wizard_paths");

  filter(paths, (: addPath :));
}

public void removeWizardPaths() {
  filter(queryPath(), (: remPath :));

  addStandardPaths();
}

public void addStandardPaths() {
  string *paths = explode_file("/adm/etc/standard_paths");

  filter(paths, (: addPath :));
}

public void addGhostPaths() {
  string *paths = explode_file("/adm/etc/ghost_paths");

  filter(paths, (: addPath :));
}

public nomask varargs string *queryCommandHistory(int index,
    int range) {
  if(this_body() != this_object()
      && !adminp(previous_object()))
    return ({});

  if(!index)
    return cmdHistory + ({});
  else if(range)
    return cmdHistory[index..range] + ({});
  else
    return ({ cmdHistory[index] });
}

public int commandHook(string arg) {
  string verb, err, *cmds = ({});

  object
  /** @type {STD_BODY}    */  caller,
  /** @type {STD_CMD}     */  cmd,
  /** @type {STD_ITEM}    */  ob,
  /** @type {STD_ITEM*}   */ *obs;
  int sz;
  mixed result;
  string complete;
  mixed returnValue;

  caller = this_body();

  if(interactive(caller))
    if(caller != this_object())
      return 0;

  verb = query_verb();

  if(sscanf(alias_parse(verb, arg), "%s %s", verb, arg) != 2)
    verb = alias_parse(verb, arg);

  if(arg == "")
    arg = 0;

  verb = lower_case(verb);

  if(arg)
    complete = sprintf("%s %s", verb, arg);
  else
    complete = verb;

  obs = all_inventory();
  if(environment())
    obs += ({ environment() }) + all_inventory(environment());

  obs += ({ this_object() });

  foreach(ob in obs) {
    result = ob->evaluateCommand(this_object(), verb, arg);
    result = evaluateResult(result);
    if(result == 1)
      return 1;
  }

  if(arg)
    cmdHistory += ({ verb + " " + arg });
  else
    cmdHistory += ({ verb });

  // Communication checks
  catch {
    if(environment())
      if(SOUL_D->request_emote(verb, arg))
        return 1;

    err = catch(load_object(CHAN_D));
    if(!err)
      if(CHAN_D->chat(verb, query_privs(), arg))
        return 1;
  };

  cmds = map(cmdPaths, (: $1 + $(verb) + ".c" :));
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

    returnValue = cmd->main(caller, arg);

    result = evaluateResult(returnValue);
    if(result == 1)
      return 1;

    return returnValue;
  }

  return 0;
}

public string findCommandPath(string verb) {
  string *paths = queryPath();
  string path;

  foreach(string p in paths) {
    if(file_exists(p + verb + ".c")) {
      path = p + verb;
      break;
    }
  }

  return path;
}

private nomask int evaluateResult(mixed result) {
  if(stringp(result)) {
    if(!strlen(result)) {
      return 0;
    } else {
      result = append(result, "\n");
      debug(result);
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

public int forceMe(string cmd) {
  if(this_body() != this_object()
      && !adminp(previous_object())
      && !adminp(this_caller()))
    return 0;
  else
    return command(cmd);
}
