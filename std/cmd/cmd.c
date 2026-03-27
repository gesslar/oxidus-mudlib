/**
 * @file /std/cmd/cmd.c
 *
 * Basic command object. Provides the standard framework for
 * all commands, including help/usage text, file and directory
 * resolution, and the command entry point.
 *
 * @created 2022-08-24 - Gesslar
 * @last_modified 2024-08-16 - Gesslar
 *
 * @history
 * 2022-08-24 - Gesslar - Created
 * 2024-08-16 - Gesslar - Added resolve_dir and resolve_file
 */

inherit STD_OBJECT;

// Functions
public string help(object caller);
public string usage(object caller);
public int _usage(object tp);
public mixed main(object user, string arg);

// Variables
protected mixed help_text = (: help :);
protected mixed usage_text = (: usage :);

// Private so only the driver can call it.
private void create() {
  setup_chain();
}

/**
 * Returns the help text for this command.
 *
 * @param {STD_BODY} _caller - The player requesting help
 * @returns {string | undefined} The help text
 */
string query_help(object _caller) {

}
#if 0
string query_help(object caller) {
  string result;
  string temp;

  if(valid_function(help_text)) {
    debug_message(sprintf("%O", help_text));
    catch {
    temp = help_text(caller);
    };
  }
    else if(stringp(help_text))
    temp = help_text;
  else
    return "There is no help available on this topic.";

  result = append(temp, "\n");

  if(valid_function(usage_text))
    temp = temp(caller);
  else if(stringp(usage_text))
    temp = usage_text;

  if(temp)
    result = "Usage:\n"+append(temp, "\n") + "\n" + result;

  while(result[<1] == '\n')
    result = result[0..<2];

  return result;
}
#endif

/**
 * Resolves a file path from a player's argument. If the
 * argument matches an object in the environment, returns its
 * base name (or virtual master for virtual objects). Otherwise
 * resolves relative to the player's current working directory.
 *
 * @param {STD_BODY} tp - The caller resolving the file
 * @param {string} arg - The file path or object identifier
 * @returns {string} The resolved file path
 */
string resolve_file(object tp, string arg) {
  string file;

  // Do we have an object with this id in our inventory or our environment?
  // If so, return its basename, unless it's a virtual object, then return
  // its virtual master.


  /** @type {STD_ITEM} */ object ob = get_object(arg);
  if(objectp(ob)) {
    if(virtualp(ob)) {
      file = ob->query_virtual_master();
    } else {
      file = base_name(ob);
    }
  } else {
    // Otherwise, we need to resolve the file and return that.
    file = resolve_path(tp->query_env("cwd"), arg);
  }

  write(file+"\n");

  return file;
}

/**
 * Resolves a directory path from a player's argument. If the
 * argument matches an object in the environment, returns the
 * directory portion of its file path. Otherwise resolves
 * relative to the player's current working directory.
 *
 * @param {STD_BODY} tp - The caller resolving the directory
 * @param {string} arg - The directory path or object identifier
 * @returns {string} The resolved directory path
 */
string resolve_dir(object tp, string arg) {
  /** @type {STD_ITEM} */ object ob = get_object(arg);
  string dir;


  if(ob) {
    string *parts;
    string file;

    if(virtualp(ob))
      file = ob->query_virtual_master();
    else
      file = base_name(ob);

    parts = dir_file(file);
    dir = parts[0];
  } else {
    dir = resolve_path(tp->query_env("cwd"), arg);
  }

  return dir;
}

int _usage(object _tp) {
  return 1;
}

#if 0
int _usage(object tp) {
  string result;
  string *parts;
  int len, pos;

  if(stringp(usage_text))
    result = usage_text;
  else if(valid_function(usage_text))
    result = usage_text(tp);
  else
    return 0;

  while(result[<1] == '\n')
    result = result[0..<2];

  len = strlen(result);
  pos = strsrch(result, "\n");
  if(pos > 0 && pos < len - 1)
    result = "Usage:\n" + result;
  else
    result = "Usage: " + result;

  _info(tp, result);

  return 1;
}
#endif

/**
 * Identifies this object as a command.
 *
 * @returns {int} Always returns 1
 */
int is_command() {
  return 1;
}

/**
 * @param {STD_BODY} _caller - The calling living.
 */
string usage(object _caller) { return null; }
