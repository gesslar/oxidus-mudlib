/**
 * @file /cmds/file/rmdir.c
 *
 * Command to remove a directory.
 *
 * @created 2024-08-16 - Gesslar
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2024-08-16 - Gesslar - Created
 * 2026-03-23 - Gesslar - Updated to current conventions
 */

inherit STD_CMD;

void setup() {
  usage_text = "rmdir <directory name>";
  help_text =
"This command permanently removes a specified directory. "
"It will not currently delete a directory that has content "
"(i.e. the directory you wish to remove must be empty "
"before it can be deleted).";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string str) {
  if(!str)
    return _usage(caller);

  str = resolve_path(caller->query_env("cwd"), str);

  if(!directory_exists(str) || file_exists(str))
    return _error(caller,
      "%s is not a directory.", str);

  if(sizeof(get_dir(str + "/")))
    return _error(caller, "%s is not empty.", str);

  if(!master()->valid_write(str, caller, "rmdir"))
    return _error(caller, "Permission denied.");

  rmdir(str)
    ? _ok(caller, "Directory removed: %s", str)
    : _error(caller,
        "Could not remove directory: %s", str);

  return 1;
}
