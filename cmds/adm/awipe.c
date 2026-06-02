/* awipe.c

Account wipe command. Mirrors nuke, but operates on an account, removing the
account along with every character associated with it.

*/

#include <logs.h>

inherit STD_CMD;

void confirm_awipe(string str, object caller, string account);

mixed main(/** @type {STD_PLAYER} */ object caller, string account) {
  if(!adminp(previous_object()))
    return _error("Access denied.");

  if(!account)
    return _info("Usage: awipe <account>");

  account = lower_case(account);

  if(!valid_account(account))
    return _error("Account '%s' does not exist.", account);

  _question(caller, "Are you sure you want to delete account " + account +
    " and all of its characters? [y/n] ", MSG_PROMPT);
  input_to("confirm_awipe", 0, caller, account);

  return 1;
}

void confirm_awipe(string str, object caller, string account) {
  if(str != "y" && str != "yes") {
    _info(caller, "Abort [awipe]: Aborting account wipe.");
    return;
  }

  /** @type {string*} */ string *characters = ACCOUNT_D->account_characters(account) || ({});

  _info(caller, "Wiping account '%s' with %d character(s).", account, sizeof(characters));

  // First pass: strip every character of its group memberships and persist the
  // result before doing any destructive filesystem work, so a later failure
  // can't leave the changes unwritten or leak the editor clone.
  /** @type {OBJ_SECURITY_EDITOR} */ object security_editor = new(OBJ_SECURITY_EDITOR);

  foreach(string user in characters) {
    _info(caller, "Stripping character '%s' of system group memberships.", capitalize(user));

    foreach(mixed group in security_editor->list_groups()) {
      if(is_member(user, group))
        _info(caller, "Removing from group: %s.", group);
      security_editor->disable_membership(user, group);
    }
  }

  security_editor->write_state(0);
  security_editor->remove();

  // Second pass: disconnect any logged-in bodies and delete user directories.
  foreach(string user in characters) {
    object body;

    if(body = find_player(user)) {
      _info(caller, "Disconnecting user '" + user + "'.");
      _ok(body, "You watch as your body dematerializes.");

      if(environment(body)) {
        tell_down(environment(body), "You watch as " + capitalize(user) + " dematerializes before your eyes.\n",
          0, ({ body }));
        body->remove();
      }
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
  }

  string account_file = account_file(account);

  _info(caller, "Removing account '%s'.", account);
  ACCOUNT_D->remove_account(account);

  if(file_exists(account_file)) {
    _info(caller, "Deleting account file.");
    rm(account_file);
  }

  _ok(caller, "Account '%s' has been wiped.", account);
  log_file(LOG_AWIPE, capitalize(query_privs(caller)) + " awipes account " + account + " on " + ctime(time()) + "\n");
}

string query_help(object _caller) {
  return
    " SYNTAX: awipe <account>\n\n"
    "This command will delete the target account along with every character "
    "associated with it, removing each character's pfile and group memberships "
    "from " + mud_name() + ". This is NON REVERSABLE, so use with discretion.\n\n"
    " See also: nuke, lockdown";
}
