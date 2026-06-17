/**
 * @file /cmds/adm/makeadmin.c
 *
 * Promotes a user to admin by adding them to the 'admin' group and
 * wiring up the admin command path. Mirrors makedev/revdev.
 *
 * @created 2026-06-01 - Gesslar
 * @last_modified 2026-06-01 - Gesslar
 *
 * @history
 * 2026-06-01 - Gesslar - Created
 */

#include <logs.h>

inherit STD_CMD;

mixed main(/** @type {STD_PLAYER} */ object caller, string args) {
  object body;

  if(!adminp(previous_object()))
    return _error("Access denied.");

  if(!args)
    return _info("Syntax: makeadmin <user>");

  args = lower_case(args);
  body = find_player(args);

  if(!body)
    return _error("User '%s' is not online.", args);

  if(adminp(query_privs(body))) {
    _info("That user is already an admin.");
    _info("Setting up command path for '%s'.", capitalize(body->query_real_name()));
    body->add_path("/cmds/wiz/");
    body->add_path("/cmds/object/");
    body->add_path("/cmds/file/");
    body->add_path("/cmds/adm/");
    body->save_body();
    return 1;
  }

  _info("Setting up admin access for '%s'.", capitalize(body->query_real_name()));

  _info(body, "Setting up admin account...");

  body->add_path("/cmds/wiz/");
  body->add_path("/cmds/object/");
  body->add_path("/cmds/file/");
  body->add_path("/cmds/adm/");

  master()->add_role(query_privs(body), "admin");
  body->save_body();

  _ok(body, "Success.");
  _ok(body, "Admin Access Granted.");

  _ok("User '%s' is now an admin.", capitalize(body->query_real_name()));
  log_file(LOG_PROMOTE, capitalize(query_privs(caller)) + " promotes "
    + body->query_real_name() + " to admin status on " + ctime(time())
    + "\n");
  return 1;
}

string query_help(object _caller) {
  return
    " SYNTAX: makeadmin <user>\n\n"
    "This command will set up a user with admin access to the mud\n"
    "by adding the user to the 'admin' group and adding '/cmds/wiz/',\n"
    "'/cmds/object/', '/cmds/file/', and '/cmds/adm/' to the user's\n"
    "command path.\n\n"
    "See also: revadmin, makedev\n";
}
