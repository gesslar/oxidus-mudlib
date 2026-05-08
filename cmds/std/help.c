/**
 * @file /cmds/std/help.c
 *
 * Command to access the help system for topics and commands.
 *
 * @created 2005-10-08 - Tacitus @ LPUniversity
 * @last_modified 2006-10-06 - Tacitus
 *
 * @history
 * 2005-10-08 - Tacitus - Created
 * 2006-10-06 - Tacitus - Last edited
 */

inherit STD_CMD;

private nosave string* HELP_PATH = ({ "/doc/general/", "/doc/game/" });
private nosave string* DEV_PATH = ({ "/doc/wiz/", "/doc/driver/efun/", "/doc/driver/apply/" });
private nosave string* ADMIN_PATH = ({ "/doc/admin/" });

#include <logs.h>

mixed main(/** @type {STD_PLAYER} */ object tp, string str) {
  /** @type {STD_CMD} */ object cmd;
  string file, *path, err, output = "";
  int i;

  if(!str)
    str = "help";

  path = tp->query_path();

  for(i = 0; i < sizeof(path); i++) {
    if(file_exists(path[i] + str + ".c")) {
      err = catch(cmd = load_object(path[i] + str));
      if(!err)
        file = cmd->query_help(tp);

      if(err)
        return _error("This is a problem with '%s'. Please inform an admin.",
          str);

      if(!file)
        return _error("The command '%s' exists but there is no help file for "
          "it. Please inform an admin.", str);

      tp->page(output);

      return 1;
    }
  }

  path = HELP_PATH;

  if(devp(tp))
    path += DEV_PATH;

  if(adminp(tp))
    path += ADMIN_PATH;

  for(i = 0; i < sizeof(path); i++) {
    if(file_exists(path[i] + str)) {
      file = read_file(path[i] + str);

      output += (file + "\n");

      tp->page(output);
      return 1;
    }
  }

  log_file(LOG_HELP, "Not found: " + str + "\n");

  return _error("Unable to find help file for: " + str);
}

string query_help(object _caller) {
  return
    "Syntax: help <topic>\n\n"
    "Whenever you need help or information regarding something in the mud, this "
    "is the place to come. This command gives you instant access to a wealth of "
    "information that will be vital to your stay here on " + mud_name() + ". "
    "Help that you want not written yet? Let us know and we'll get right on it!";
}
