/**
 * @file /cmds/ghost/help.c
 * @description Help command for looking up help files and command help.
 *
 * @created 2005-10-08 - Tacitus
 * @last_modified 2006-10-06 - Tacitus
 *
 * @history
 * 2005-10-08 - Tacitus - Created
 */

inherit STD_CMD;

#include <logs.h>

#define HELP_PATH ({ "/doc/general/", "/doc/game/" })
#define DEV_PATH ({ \
  "/doc/wiz/", \
  "/doc/driver/efun/", \
  "/doc/driver/apply/" \
})

mixed main(/** @type {STD_PLAYER} */ object caller, string str) {
  string file, *path, err, output = "";
  /** @type {STD_CMD} */ object cmd;
  int width = 80;
  string border;

  if(caller->query_environ("WORD_WRAP"))
    width = caller->query_environ("WORD_WRAP");

  border = "\u255e"
    + repeat_string("\u2550", width - 2) + "\u2561\n";

  if(!str)
    str = "help";

  path = caller->query_path();

  for(int i = 0; i < sizeof(path); i++) {
    if(file_exists(path[i] + str + ".c")) {
      err = catch(cmd = load_object(path[i] + str));

      if(err)
        return _error(caller,
          "This is a problem with " + str
          + "\nPlease inform an admin."
        );

      file = cmd->query_help(caller);

      if(!file)
        return _error(caller,
          "The command " + str + " exists but "
          "there is no help file for it.\n"
          "Please inform an admin."
        );

      output += border;
      output += sprintf("%|*s\n", width, str);
      output += border + "\n";
      output += file + "\n";

      caller->page(output);
      return 1;
    }
  }

  path = HELP_PATH;
  if(devp(caller))
    path += DEV_PATH;
  if(adminp(caller))
    path += ({ "/doc/admin/" });

  for(int i = 0; i < sizeof(path); i++) {
    if(file_exists(path[i] + str)) {
      file = read_file(path[i] + str);
      output += border;
      output += "\t\t  Help file for topic '"
        + capitalize(str) + "'\n";
      output += border + "\n";
      output += file + "\n";

      caller->page(output);
      return 1;
    }
  }

  log_file(LOG_HELP, "Not found: " + str + "\n");

  return _error(caller,
    "Unable to find help file for: " + str
  );
}

string query_help(object _caller) {
  return
"SYNTAX: help <topic>\n\n"
"Whenever you need help or information regarding something "
"in the mud, this is the place to come. This command gives "
"you instant access to a wealth of information that will be "
"vital to your stay here on " + mud_name() + ". Help that "
"you want not written yet? Let us know and we'll get right "
"on it!";
}
