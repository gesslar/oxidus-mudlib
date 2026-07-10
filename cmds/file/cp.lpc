/**
 * @file /cmds/file/cp.c
 *
 * Copy command.
 *
 * @created 2024-08-17 - Gesslar
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2024-08-17 - Gesslar - Created
 * 2026-03-23 - Gesslar - Updated to current conventions
 */

inherit STD_CMD;

void setup() {
  usage_text = "cp <original> <destination>";
  help_text =
"This command copies a file from its original location "
"to a new one. The original file will remain in its "
"original location.\n\n"
"If the destination is a directory, the file will be "
"copied into that directory. If the destination is a "
"file, it will be overwritten.\n\n"
"You may use absolute or relative paths for the source "
"and/or destination.";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string str) {
  string original, destination;

  if(!str
  || sscanf(str, "%s %s", original, destination) != 2)
    return _usage(caller);

  original = resolve_path(
    caller->query_env("cwd"), original);
  destination = resolve_path(
    caller->query_env("cwd"), destination);

  if(!file_exists(original))
    return _error(caller,
      "No such file: %s", original);

  if(directory_exists(destination)
  || destination[<1] == '/') {
    int pos;

    destination = append(destination, "/");

    pos = strsrch(original, "/", -1);
    if(pos != -1)
      destination += original[(pos + 1)..];
    else
      destination += original;
  }

  if(original == destination)
    return _error(caller,
      "Original and destination are the same.");

  if(!master()->valid_write(
      destination, this_object(), "cp"))
    return _error(caller, "Permission denied.");

  cp(original, destination) < 0
    ? _error(caller,
        "Unable to copy %s to %s.",
        original, destination)
    : _ok(caller, "%s copied to %s",
        original, destination);

  return 1;
}
