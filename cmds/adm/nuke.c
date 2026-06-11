/* nuke.c

Tacitus @ LPUniversity
05-MAY-06
Standard nuke command V2

*/

/* Last edited on October 6th, 2006 by Tacitus */

#include <logs.h>

inherit STD_CMD;

void confirm_nuke(string str, object caller, string user);

mixed main(/** @type {STD_PLAYER} */ object caller, string user) {
  if(!adminp(previous_object()))
    return _error("Access denied.");

  if(!user)
    return _info("Usage: nuke <player>");

  user = lower_case(user);

  if(!user_exists(user))
    return _error("User '%s' does not exist.", user);

  _question(caller, "Are you sure you want to delete " + user + "? [y/n] ", MSG_PROMPT);
  input_to("confirm_nuke", 0, caller, user);

  return 1;
}

void confirm_nuke(string str, object caller, string user) {
  object body;

  if(str != "y" && str != "yes") {
    _info(caller, "Abort [nuke]: Aborting nuke.");
    return;
  }

  _info(caller, "Stripping user of system group memberships.");

  /** @type {OBJ_SECURITY_EDITOR} */ object security_editor = new(OBJ_SECURITY_EDITOR);

  foreach(mixed group in security_editor->list_groups()) {
    if(is_member(user, group))
      _info(caller, "Removing from group: %s.", group);
    security_editor->disable_membership(user, group);
  }

  security_editor->write_state(0);
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

  string account_name = ACCOUNT_D->character_account(user);
  if(account_name) {
    _info(caller, "Detaching character from account '%s'.", account_name);
    ACCOUNT_D->remove_character(account_name, user);
  }

  string user_dir = user_data_directory(user);
  if(directory_exists(user_dir)) {
    mixed err;

    _info(caller, "Deleting user directory for user '%s'.", capitalize(user));
    err = catch(recursive_delete(as_directory(user_dir), true));

    if(err) {
      _error(caller, "Error while deleting user directory: %s", err);
      return;
    }
  }

  _ok(caller, "User '%s' has been removed.", capitalize(user));
  log_file(LOG_NUKE, capitalize(query_privs(caller)) + " nukes " + capitalize(user) + " on " + ctime(time()) + "\n");
}

string query_help(object _caller) {
  return
    " SYNTAX: nuke <username>\n\n"
    "This command will delete the target user's pfile, thus removing their account "
    "from " + mud_name() + ". Furthermore it will also remove their membership "
    "from all groups. This is NON REVERSABLE, so use with discretion.";
}
