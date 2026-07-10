/**
 * @file /cmds/object/work.c
 *
 * Change work directory to a specific object's or your environment.
 *
 * @created 2026-05-06 - Gesslar
 * @last_modified 2026-05-06 - Gesslar
 *
 * @history
 * 2026-05-06 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(/** @type {STD_PLAYER} */ object tp, string arg) {
  if(!arg) {
    if(!environment(tp)) {
      return _error("You have no environment.");
    }

    string cwd = query_directory(environment(tp));
    if(file_size(cwd) != -2)
      return _error("No such directory: %s", cwd);

    tp->set_env("cwd", cwd);

    return _ok("Working directory → %s", cwd);
  }

  object ob = get_object(arg);
  string cwd = query_directory(ob);

  if(file_size(cwd) != -2)
    return _error("No such directory: %s", cwd);

  tp->set_env("cwd", cwd);

  return _ok("Working directory → %s", cwd);
}
