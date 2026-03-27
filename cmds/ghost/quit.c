/**
 * @file /cmds/ghost/quit.c
 * @description Quit command for ghosts to disconnect from the mud.
 *
 * @created 2005-04-06 - Tacitus
 * @last_modified 2005-10-08 - Tacitus
 *
 * @history
 * 2005-04-06 - Tacitus - Created
 */

#include <logs.h>

inherit STD_CMD;

mixed main(/** @type {STD_BODY} */ object caller, string _arg) {
  caller->exit_world();
  tell(caller, "Thank you for visiting " + mud_name() + ".\n");
  caller->save_body();
  log_file(LOG_LOGIN,
    capitalize(caller->query_real_name())
    + " logged out from " + query_ip_number(caller)
    + " on " + ctime(time()) + "\n"
  );
  destruct(caller);

  return 1;
}

string query_help(object _caller) {
  return
"SYNTAX: quit\n\n"
"This command will save your character and disconnect you "
"from the mud.\n\n"
"See also: save";
}
