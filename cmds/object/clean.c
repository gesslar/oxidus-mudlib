/**
 * @file /cmds/object/clean.c
 * @description Command to destroy all non-living, non-protected objects
 *              within a target object's inventory.
 *
 * @created 2005-10-28 - Icoz@LPUniversity
 * @last_modified 2024-02-04 - Gesslar
 *
 * @history
 * 2005-10-28 - Icoz@LPUniversity - Created
 * 2005-10-29 - Tacitus - QC Review and edit
 * 2024-02-04 - Gesslar - General formatting and refactor
 */

inherit STD_CMD;

public mixed main(
  /** @type {STD_PLAYER} */ object caller, string arg
) {
  object target;
  object ob, next;

  if(!arg) {
    target = caller;
  } else {
    if(arg[0] != '/')
      arg = resolve_path(caller->query_env("cwd"), arg);
    if(arg[<2..<1] != ".c")
      arg += ".c";
    if(!target)
      target = find_object(arg);
    if(!target)
      target = present(arg, caller);
    if(!target)
      target = present(arg, environment(caller));
  }

  if(!target)
    return _error("Error locating target.");

  _info(
    caller,
    "Destroying all objects in '%s'.",
    get_short(target)
  );

  ob = first_inventory(target);
  while(ob) {
    string shortDesc = get_short(ob);

    next = next_inventory(ob);
    if(ob->query_no_clean() || ob->can_clean_up()) {
      ob = next;
      continue;
    }
    if(living(ob)) {
      ob = next;
      continue;
    }

    ob->remove();
    if(ob)
      destruct(ob);

    _ok(caller, "%s destroyed.", shortDesc);
    ob = next;
  }

  return 1;
}

string query_help(object _caller) {
  return
"SYNTAX: clean <object>\n\n"
"This command will destroy all objects within another "
"object (aka the object's inventory). Your argument may "
"be an object that is in your inventory, your "
"environment, or a filename. If you do not specify an "
"object, it will remove your inventory.";
}
