/* revdev.c

 Tacitus @ LPUniversity
 15-JAN-06
 Admin command

*/

//Last edited on October 6th, 2006 by Tacitus

#include <logs.h>

inherit STD_CMD;

mixed main(/** @type {STD_PLAYER} */ object caller, string args) {
  if(!adminp(previous_object()))
    return _error("Access denied.");
  if(!args)
    return _error("revdev <user>");

  args = lower_case(args);

  /** @type {STD_PLAYER} */ object body = find_player(args);

  if(!body)
    return _error("User '%s' not online.", args);

  if(!devp(query_privs(body)))
    return _error("That user is not a developer.");

  _info("Revoking developer access for '%s'.", capitalize(body->query_real_name()));

  _ok(body, "Developer Access Revoked.");
  body->rem_path("/cmds/wiz/");
  body->rem_path("/cmds/object/");
  body->rem_path("/cmds/file/");
  body->rem_path("/cmds/adm/");

  /** @type {OBJ_SECURITY_EDITOR} */ object security_editor = new(OBJ_SECURITY_EDITOR);
  security_editor->disableMembership(query_privs(body), "developer");
  security_editor->writeState(0);
  security_editor->remove();
  body->save_body();

  _ok("User '%s' is no longer a developer.", capitalize(body->query_real_name()));

  log_file(LOG_PROMOTE, capitalize(query_privs(caller)) + " revoked "
    + body->query_real_name() + "'s developer status on " + ctime(time())
    + "\n");
  return 1;
}

string query_help(object _caller) {
  return
    " SYNTAX: revdev <user>\n\n"
    "This command will revoke a user's developer access to\n"
    "the mud by removing them from the group 'developer' and\n"
    "by removing the directories '/cmds/wiz/', '/cmds/object/'\n"
    "and '/cmds/file/' from the user's command path.\n\n"
    "See also: makedev\n";
}
