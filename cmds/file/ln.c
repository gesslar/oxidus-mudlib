/**
 * @file /cmds/file/ln.c
 *
 * Creates a file link to another file.
 *
 * @created 2006-01-15 - Tacitus
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2006-01-15 - Tacitus - Created
 * 2026-03-23 - Gesslar - Updated to current conventions
 */

inherit STD_CMD;

void setup() {
  usage_text = "ln <original file> <new reference>";
  help_text =
"This command is a very powerful tool that allows you "
"to create a file that references to another file. This "
"means when you access the new file created with this "
"command, you'll be actually accessing the original "
"file. This is a very dangerous tool as well because "
"the new file will inherit the permissions of its "
"original file, possibly allowing people to access the "
"file who wouldn't be able to access it before. For "
"this command to work, you must have read and linking "
"permissions in the directory of the original file as "
"well as write and linking permissions in the directory "
"of the new reference.";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string args) {
  string file, reference;

  if(!args)
    return _usage(caller);

  if(sscanf(args, "%s %s", file, reference) != 2)
    return _usage(caller);

  file = resolve_path(
    caller->query_env("cwd"), file);
  reference = resolve_path(
    caller->query_env("cwd"), reference);

  if(!file_exists(file))
    return _error(caller,
      "File '%s' does not exist.", file);

  if(file_exists(reference))
    return _error(caller,
      "File '%s' already exists.", reference);

  if(directory_exists(file))
    return _error(caller,
      "'%s' is a directory.", file);

  if(directory_exists(reference))
    return _error(caller,
      "'%s' is a directory.", reference);

  if(!master()->valid_link(file, reference))
    return _error(caller, "Permission denied.");

  link(file, reference) < 0
    ? _error(caller,
        "Unable to link '%s' to '%s'.",
        reference, file)
    : _ok(caller, "'%s' now linked to '%s'.",
        reference, file);

  return 1;
}
