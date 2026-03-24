/* makedev.c

 Tacitus @ LPUniversity
 14-JAN-06
 Admin Command

*/

/* Last edited on October 6th, 2006 by Tacitus */

#include <logs.h>

inherit STD_CMD;

mixed main(/** @type {STD_PLAYER} */ object caller, string args) {
  object body;
  object security_editor;
  string path;

  if(!adminp(previous_object()))
    return _error("Access denied.");

  if(!args)
    return _info("Syntax: makedev <user>");

  args = lower_case(args);
  body = find_player(args);

  if(!body)
    return _error("User '%s' is not online.", args);

  if(devp(query_privs(body))) {
    _info("That user is already a developer.");
    _info("Setting up command path for '%s'.", capitalize(body->query_real_name()));
    body->add_path("/cmds/wiz/");
    body->add_path("/cmds/object/");
    body->add_path("/cmds/file/");
    return 1;
  }

  _info("Setting up developer access for '%s'.", capitalize(body->query_real_name()));

  _info(body, "Setting up developer account...");

  path = home_path(body->query_real_name());
  assure_dir(path);
  assure_dir(path + "public");
  assure_dir(path + "private");
  cp("/d/std/workroom.c", path + "workroom.c");
  body->add_path("/cmds/wiz/");
  body->add_path("/cmds/object/");
  body->add_path("/cmds/file/");
  security_editor = new(OBJ_SECURITY_EDITOR);
  security_editor->enableMembership(query_privs(body), "developer");
  security_editor->writeState(0);
  _ok(body, "Success.");
  _ok(body, "Developer Access Granted.");

  _ok("User '%s' is now a developer.", capitalize(body->query_real_name()));
  log_file(LOG_PROMOTE, capitalize(query_privs(caller)) + " promotes "
    + body->query_name() + " to developer status on " + ctime(time())
    + "\n");
  return 1;
}

string query_help(object _caller) {
  return
    " SYNTAX: makedev <user>\n"
    "This command will set up a user with developer access to\n"
    "the mud by adding the user to the 'developer' group, creating\n"
    "them a home directory, and adding '/cmds/wiz/', '/cmds/object/',\n"
    "and '/cmds/file/' to the user's directory.\n\n"
    "See also: revdev\n";
}
