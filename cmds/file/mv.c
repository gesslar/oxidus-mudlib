/**
 * @file /cmds/file/mv.c
 *
 * Move a file or directory.
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
  usage_text = "mv <origin> <dest>";
  help_text =
"Moves a file or directory from origin to destination. "
"The origin and destination can be either absolute or "
"relative paths.\n"
"* If the destination is a directory, the origin will "
"be moved inside the directory.\n"
"* If the destination does not exist, it will be "
"created.\n"
"* If the source does not exist, an error will be "
"returned.\n"
"* If the source is a directory, the destination must "
"be a directory as well.\n"
"* If the destination is a file, an error will be "
"returned.";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string str) {
  string origin, dest;
  int result;

  if(!str || !sscanf(str, "%s %s", origin, dest))
    return _usage(caller);

  origin = resolve_path(
    caller->query_env("cwd"), origin);
  dest = resolve_path(
    caller->query_env("cwd"), dest);

  if(!master()->valid_write(origin, caller, "mv")
  || !master()->valid_write(dest, caller, "mv"))
    return _error(caller, "Permission denied.");

  result = rename(origin, dest);

  if(result < 0)
    return _error(caller, "Move failed.");

  return _ok(caller,
    "Moved %s to %s", origin, dest);
}
