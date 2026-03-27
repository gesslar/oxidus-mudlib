/**
 * @file /cmds/file/tail.c
 *
 * Command to print the last 20 lines of a file.
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
  usage_text = "tail <file>";
  help_text =
"This command prints the last 20 lines of a specified "
"file.";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string file) {
  string *out;

  if(!file)
    return _usage(caller);

  file = resolve_file(caller, file);

  if(!file_exists(file))
    return _error(caller,
      "File does not exist: %s", file);

  out = explode(tail(file, 20), "\n");
  out = ({ "=== " + file + " ===" }) + out;

  return out;
}
