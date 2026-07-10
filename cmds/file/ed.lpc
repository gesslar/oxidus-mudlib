/**
 * @file /cmds/file/ed.c
 *
 * File editor command.
 *
 * @created 2005-04-05 - Byre
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2005-04-05 - Byre - Created
 * 2006-10-06 - Tacitus - Last edited
 * 2026-03-23 - Gesslar - Updated to current conventions
 */

#include <logs.h>

inherit STD_CMD;

void setup() {
  usage_text = "ed <file>";
  help_text =
"This command lets you edit a specified file in the mud "
"editor. To access editor specific help, type 'h' while "
"in the editor.\n\n"
"See also: rm, more, mv, cp";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string file) {
  string *parts;

  if(!file) {
    if(!caller->query_env("cwf"))
      return _error(caller, "No argument supplied.");
    file = caller->query_env("cwf");
  }

  file = resolve_path(caller->query_env("cwd"), file);

  if(directory_exists(file))
    return _error(caller, "Cannot edit a directory.");

  parts = dir_file(file);
  if(!directory_exists(parts[0]))
    return _error(caller,
      "No such directory: " + parts[0]);

  caller->set_env("cwf", file);
  _info(caller, "Editing %s", file);

  caller->ed_edit(file);

  log_file(LOG_ED,
    capitalize(query_privs(caller)) + " edited "
    + file + " on " + ctime(time()) + "\n");

  return 1;
}
