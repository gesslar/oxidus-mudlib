/**
 * @file /cmds/adm/revadmin.c
 *
 * Revokes a user's admin access by removing them from the 'admin'
 * group and stripping the admin command path. An admin who is not an
 * owner may not revoke an owner. Mirrors revdev.
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
  if(!adminp(previous_object()))
    return _error("Access denied.");
  if(!args)
    return _error("revadmin <user>");

  args = lower_case(args);

  /** @type {STD_PLAYER} */ object body = find_player(args);

  if(!body)
    return _error("User '%s' not online.", args);

  if(!adminp(query_privs(body)))
    return _error("That user is not an admin.");

  // An owner outranks a plain admin: only an owner may revoke an owner.
  if(ownerp(body) && !ownerp(caller))
    return _error("Only an owner may revoke another owner's access.");

  _info("Revoking admin access for '%s'.", capitalize(body->query_real_name()));

  _ok(body, "Admin Access Revoked.");
  body->rem_path("/cmds/adm/");

  // makeadmin grants the developer command paths too. Strip them on
  // revocation unless the user is independently a developer, since
  // developer is a separate group from admin.
  if(!devp(query_privs(body))) {
    body->rem_path("/cmds/wiz/");
    body->rem_path("/cmds/object/");
    body->rem_path("/cmds/file/");
  }

  /** @type {OBJ_SECURITY_EDITOR} */ object security_editor = new(OBJ_SECURITY_EDITOR);
  security_editor->disable_membership(query_privs(body), "admin");
  security_editor->write_state(0);
  security_editor->remove();
  body->save_body();

  _ok("User '%s' is no longer an admin.", capitalize(body->query_real_name()));

  log_file(LOG_PROMOTE, capitalize(query_privs(caller)) + " revoked "
    + body->query_real_name() + "'s admin status on " + ctime(time())
    + "\n");
  return 1;
}

string query_help(object _caller) {
  return
    " SYNTAX: revadmin <user>\n\n"
    "This command will revoke a user's admin access to the mud by\n"
    "removing them from the group 'admin' and removing '/cmds/adm/'\n"
    "from the user's command path. An admin who is not an owner may\n"
    "not revoke an owner.\n\n"
    "See also: makeadmin, revdev\n";
}
