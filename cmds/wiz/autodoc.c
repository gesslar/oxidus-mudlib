/**
 * @file /cmds/wiz/autodoc.c
 * @description Command to interface with the autodoc system.
 *
 * @created 2024-07-14 - Gesslar
 * @last_modified 2024-07-14 - Gesslar
 *
 * @history
 * 2024-07-14 - Gesslar - Created
 */

#include <daemons.h>

inherit STD_CMD;

mixed main(object tp, string str) {
  string cmd, arg;
  mixed result;

  if(!str)
    return query_help(tp);

  if(sscanf(str, "%s %s", cmd, arg) != 2)
    cmd = str;

  switch(cmd) {
    case "scan": {
      result = AUTODOC_D->autodoc_scan();
      if(stringp(result))
        return _error(result);
      else
        return 1;
    }

    default:
      return _error("Invalid command to autodoc.");
  }
}

string query_help(object _tp) {
  return
"Syntax: autodoc scan\n\n"
"Scans the mudlib for all objects in the {{bl1}}AUTODOC_SOURCE_DIRS{{bl0}} "
"mud_config and generates documentation for them.\n";
}
