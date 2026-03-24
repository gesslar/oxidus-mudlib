/* nuke.c

Tacitus @ LPUniversity
05-MAY-06
Standard nuke command V2

*/

/* Last edited on October 6th, 2006 by Tacitus */

#include <logs.h>

inherit STD_CMD;

void confirmNuke(string str, object caller, string user);

mixed main(/** @type {STD_PLAYER} */ object caller, string user) {
  if(!adminp(previous_object()))
    return _error("Access denied.");

  if(!user)
    return _info("Usage: nuke <player>");

  user = lower_case(user);

  if(!user_exists(user))
    return _error("User '%s' does not exist.", user);

  _question(caller, "Are you sure you want to delete " + user + "? [y/n] ", MSG_PROMPT);
  input_to("confirmNuke", 0, caller, user);

  return 1;
}

void confirmNuke(string str, object caller, string user) {
  object body;
  string *dir;

  if(str != "y" && str != "yes") {
    _info(caller, "Abort [nuke]: Aborting nuke.");
    return;
  }

  _info(caller, "Stripping user of system group memberships.");

  /** @type {OBJ_SECURITY_EDITOR} */ object security_editor = new(OBJ_SECURITY_EDITOR);

  foreach(mixed group in security_editor->listGroups()) {
    if(is_member(user, group))
      _info(caller, "* Removing from group: %s.", group);
    security_editor->disableMembership(user, group);
  }

  security_editor->writeState(0);
  security_editor->remove();

  if(body = find_player(user)) {
    _info(caller, "Disconnecting user '" + user + "'.");
    _ok(body, "You watch as your body dematerializes.");

    if(environment(body)) {
      tell_down(environment(body), "You watch as " + capitalize(user) + " dematerializes before your eyes.\n",
        0, ({ body }));
      body->remove();
    }
  }

  _info(caller, "Deleting pfile for user '%s'.", capitalize(user));

  dir = get_dir(user_data_directory(user));
  foreach(string file in dir) {
    _info(caller, "* Deleting file: %s.", file);
    if(!rm(user_data_directory(user) + file)) {
      _error(caller, "Error while deleting %s in user directory.", file);
      return;
    }
  }
  rmdir(user_data_directory(user));

  _ok(caller, "User '%s' has been removed.", capitalize(user));
  log_file(LOG_NUKE, capitalize(query_privs(caller)) + " nukes " + capitalize(user) + " on " + ctime(time()) + "\n");
}

string query_help(object _caller) {
  return
    " SYNTAX: nuke <username>\n\n"
    "This command will delete the target user's pfile, thus removing their account "
    "from " + mud_name() + ". Furthermore it will also remove their membership "
    "from all groups. This is NON REVERSABLE, so use with discretion.\n\n"
    " See also: lockdown";
}
