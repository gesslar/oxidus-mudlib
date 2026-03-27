/**
 * @file /cmds/std/quit.c
 *
 * Quit command to save and disconnect from the MUD.
 *
 * @created 2005-04-06 - Tacitus @ LPUniversity
 * @last_modified 2005-10-08 - Tacitus
 *
 * @history
 * 2005-04-06 - Tacitus - Created
 * 2005-10-08 - Tacitus - Last edited
 */

#include <logs.h>

inherit STD_CMD;

mixed main(/** @type {STD_BODY} */ object caller, string _arg) {
  object su_body;

  if(su_body = caller->query_su_body()) {
    if(exec(su_body, caller)) {
      caller->clear_su_body();
      su_body->move(environment(caller));
      tell(su_body, "You return to your body.\n");
      su_body->other_action("$N $vexit the body of $o.", caller);
    } else {
      tell(caller, "Failed to return to your body.\n");
      return 1;
    }

    // This is a hack because otherwise the privileges won't work
    // for saving player data, because this_interactive() will be
    // the object that called quit, instead of the player object.
    call_out_walltime((:$(su_body)->force_me("quit") :), 0.01);
    return 1;
  }

  tell(caller, "Thank you for visiting " + mud_name() + ".\n");
  caller->exit_world();
  log_file(LOG_LOGIN, capitalize(caller->query_real_name()) + " logged out from " +
    query_ip_number(caller) + " on " + ctime(time()) + "\n");
  destruct(caller);
  return 1;
}

string query_help(object _caller) {
  return
" SYNTAX: quit\n\n"
"This command will save your character and disconnect "
"you from the mud.\n\n"
"See also: save\n";
}
