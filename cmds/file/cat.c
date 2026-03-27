/**
 * @file /cmds/file/cat.c
 *
 * Pages an entire file at once.
 *
 * @created 2005-10-02 - Tacitus
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2005-10-02 - Tacitus - Created
 * 2026-03-23 - Gesslar - Updated to current conventions
 */

inherit STD_CMD;

void setup() {
  usage_text = "cat <file>";
  help_text =
"This command pages through an entire file at once.";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string str) {
  int i, maxLines;

  if(!str)
    return _usage(caller);

  str = resolve_file(caller, str);

  if(!file_exists(str))
    return _error(caller, "No such file: %s", str);

  i = file_size(str);
  if(i < 0)
    return _error(caller,
      "No such file or 0-length file: %s", str);

  maxLines = file_length(str);

  tell(caller, "=== " + str + " ===\n");

  i = 0;
  while(++i <= maxLines)
    tell(caller, append(read_file(str, i, 1), "\n"));

  return 1;
}
