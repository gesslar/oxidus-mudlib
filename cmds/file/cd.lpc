/**
 * @file /cmds/file/cd.c
 *
 * File navigation command to change the current working
 * directory.
 *
 * @created 2005-04-05 - Byre
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2005-04-05 - Byre - Created
 * 2005-10-02 - Tacitus - Last edited
 * 2026-03-23 - Gesslar - Updated to current conventions
 */

inherit STD_CMD;

void setup() {
  usage_text = "cd [directory]";
  help_text =
"This command allows you to navigate through various "
"directories. To use this command, you simply provide "
"the directory, either an absolute path or one relative "
"to the current directory, as the argument to this "
"command.\n\n"
"Typing 'cd' without any arguments will take you to "
"your home directory.\n\n"
"See also: mkdir, rmdir, ls";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string str) {
  string test = str;

  if(!stringp(test)) {
    test = home_path(caller);
    if(!directory_exists(test))
      return _usage(caller);
  }

  test = resolve_dir(caller, test);

  if(!directory_exists(test))
    test = str;

  if(!directory_exists(test))
    return _error(caller,
      "No such directory: %s", test);

  if(!master()->valid_read(test, caller, "cd"))
    return _error(caller, "Permission denied.");

  caller->set_env("cwd", test);

  return _ok(caller,
    "Current directory set to: %s", test);
}
