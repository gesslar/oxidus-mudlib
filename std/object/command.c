/**
 * @file /std/object/command.c
 * @description Replacement for add_action, allowing for mixed results and more
 *              flexible command handling.
 *
 * @created 2024-03-03 - Gesslar
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * 2024-03-03 - Gesslar - Created
 * 2025-03-16 - GitHub Copilot - Added documentation
 */

#include "/std/living/include/alias.h"
#include "/std/living/include/pager.h"
#include <command.h>

private nosave mapping _commands = ([]);

/**
 * Adds a command handler to this object.
 *
 * @param {string|string*} command - Command name or array of command names
 * @param {function|string} action - Function pointer or method name to handle command
 * @errors If action function does not exist
 * @errors If command or action parameters are invalid types
 */
void add_command(mixed command, mixed action) {
  if(stringp(command)) {
    remove_command(command);
    if(stringp(action)) {
      if(!function_exists(action)) {
        error("add_command: No such function " + action + " in " + file_name() + ".\n");
      }
      _commands[command] = action;
    } else if(valid_function(action)) {
      _commands[command] = action;
    } else {
      error("add_command: Illegal action " + action + " in " + file_name() + ".\n");
    }
  } else if(pointerp(command)) {
    foreach(mixed cmd in command) {
      add_command(cmd, action);
    }
  } else {
    error("add_command: Illegal command " + command + " in " + file_name() + ".\n");
  }
}

/**
 * Removes one or more commands from this object.
 *
 * @param {string|string*} command - Command name or array of command names
 */
void remove_command(mixed command) {
  mixed action;

  if(stringp(command)) {
    action = _commands[command];
    map_delete(_commands, command);
  } else if(pointerp(command)) {
    foreach(mixed cmd in command) {
      if(stringp(cmd)) {
        action = _commands[cmd];
        map_delete(_commands, cmd);
      }
    }
  }
}

/**
 * Removes all commands that use a specific action handler.
 *
 * @param {function|string} action - Function name or pointer to remove
 */
void remove_command_all(mixed action) {
  foreach(mixed command, mixed act in _commands) {
    if(act == action) {
      map_delete(_commands, command);
      remove_command(command);
    }
  }
}

/**
 * Returns the action associated with a command.
 *
 * @param {string} command - The command to query
 * @returns {function|string|null} Action handler or null if not found
 */
mixed query_command(string command) {
  return _commands[command];
}

/**
 * Returns all registered commands and their actions.
 *
 * @returns {mapping} Copy of commands mapping
 */
mapping query_commands() {
  return copy(_commands);
}

string *query_all_commands() {
  return commands();
}

/**
 * Finds all commands that share the same action handler.
 *
 * Returns an array containing the given command and any other commands
 * that use the same action handler.
 *
 * @param {string} command - The command to find matches for
 * @returns {string*} Array of commands sharing the same handler, or empty array
 */
string *query_matching_commands(string command) {
  string *matches = allocate(1);

  if(nullp(_commands[command]))
    return ({});

  matches[0] = command;

  foreach(mixed cmd, mixed action in _commands) {
    if(cmd == matches[0])
      continue;

    if(action == _commands[command])
      matches += ({ cmd });
  }

  return matches;
}

/**
 * Initialises the commands mapping to an empty state.
 */
void init_commands() {
  _commands = ([]);
}

/**
 * Evaluates a command by calling its associated action.
 *
 * @param {STD_PLAYER} user - The object triggering the command
 * @param {string} command - The command name
 * @param {string} arg - The command arguments
 * @returns {mixed} Result of command evaluation, or null if no handler
 */
mixed evaluate_command(object user, string command, string arg) {
  if(stringp(_commands[command]))
    return call_other(this_object(), command, user, arg);

  if(!valid_function(_commands[command]))
    return null;

  function action = _commands[command];

  return action(user, arg);
}

string process_input(string arg) {
  return arg;
}

private string *_path = ({});
nosave string *_command_history = ({});

string *query_path() {
  return copy(_path);
}

int add_path(string str) {
  if(!adminp(previous_object()) && this_body() != this_object())
    return 0;

  if(includes(_path, str))
    return 0;

  str = append(str, "/");

  if(!directory_exists(str))
    return 0;

  _path = _path || ({});
  push(ref _path, str);

  return 1;
}

public void set_path(mixed path) {
  string *paths;

  if(stringp(path))
    paths = explode(path, ":");
  else if(pointerp(path))
    paths = copy(path);

  filter(paths, (: add_path :));
}

int rem_path(string str) {
  if(!adminp(previous_object()) && this_body() != this_object())
    return 0;

  if(!includes(_path, str))
    return 0;

  _path -= ({str});

  return 1;
}

void add_wizard_paths() {
  string *paths = explode_file("/adm/etc/wizard_paths");

  filter(paths, (: add_path :));
}

void remove_wizard_paths() {
  filter(query_path(), (: rem_path :));

  add_standard_paths();
}

void add_standard_paths() {
  string *paths = explode_file("/adm/etc/standard_paths");

  filter(paths, (: add_path :));
}

void add_ghost_paths() {
  string *paths = explode_file("/adm/etc/ghost_paths");

  filter(paths, (: add_path :));
}

nomask varargs string *query_command_history(int index, int range) {
  if(this_body() != this_object() && !adminp(previous_object()))
    return ({});

  if(!index)
    return _command_history + ({});

  else if(range)
    return _command_history[index..range] + ({});

  else
    return ({ _command_history[index] });
}

int command_hook(string arg) {
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
    result = ob->evaluate_command(this_object(), verb, arg);
    result = evaluate_result(result);
    if(result == 1)
      return 1;
  }

  if(arg)
    _command_history += ({ verb + " " + arg });
  else
    _command_history += ({ verb });

  // Communication checks
  catch {
    if(environment())
      if(SOUL_D->request_emote(verb, arg)) return 1;

    err = catch(load_object(CHAN_D));
    if(!err)
      if(CHAN_D->chat(verb, query_privs(), arg))
        return 1;
  };

  cmds = map(_path, (: $1 + $(verb) + ".c" :));
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
      tell_me("Error: Command " + verb + " non-functional.\n");
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

public string find_command_path(string verb) {
  string *paths = query_path();
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

int force_me(string cmd) {
  if(
        this_body() != this_object()
    && !adminp(previous_object())
    && !adminp(this_caller()))
    return 0;
  else
    return command(cmd);
}
