/**
 * @file /cmds/file/more.c
 *
 * File pager that displays contents a screen at a time.
 *
 * @created 2005-12-21 - Tacitus
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2005-12-21 - Tacitus - Created
 * 2006-07-05 - Vicore - Last edited
 * 2026-03-23 - Gesslar - Updated to current conventions
 */

inherit STD_CMD;

void setup() {
  usage_text = "more <file>";
  help_text =
"This command will allow you to page files to your "
"terminal, a screen at a time. For more information "
"about using this application and its features, type "
"'help' within the pager.\n\n"
"Note: Colour is not parsed for any file ending with "
"the '.c' extension. All other files, however, will "
"render with colour.";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string file) {
  string text;

  if(!file && caller->query_env("cwf"))
    file = caller->query_env("cwf");
  else if(!file)
    return _usage(caller);

  file = resolve_file(caller, file);

  if(!file_exists(file))
    return _error(caller,
      "File '%s' does not exist.", file);

  text = read_file(file);
  text = "=== " + file + " ===\n" + text;
  caller->page(text, null, 1);

  return 1;
}
