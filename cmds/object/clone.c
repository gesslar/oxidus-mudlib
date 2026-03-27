/**
 * @file /cmds/object/clone.c
 * @description Command to clone objects into the game world.
 *
 * @created 2005-10-31 - Tacitus@LPUniversity
 * @last_modified 2006-07-19 - Parthenon
 *
 * @history
 * 2005-10-31 - Tacitus@LPUniversity - Created
 * 2006-07-19 - Parthenon - Edited
 */

inherit STD_CMD;

public mixed main(
  /** @type {STD_PLAYER} */ object caller, string str
) {
  object ob, dest, env;
  string err, custom, tmp, shortDesc, fileName;
  int result;

  if(!str)
    str = caller->query_env("cwf");

  if(!str)
    return _error("SYNTAX: clone <filename>");

  env = environment(caller);

  if(ob = get_object(str)) {
    if(virtualp(ob))
      str = ob->query_virtual_master();
    else
      str = base_name(ob);

    _info(caller, "Making a copy of %O.", ob);
    ob = null;
  }

  str = resolve_path(caller->query_env("cwd"), str);

  err = catch(ob = new(str));

  if(stringp(err))
    return _error(
      "An error was encountered when cloning the "
      "object:\n%s", err
    );

  if(!ob)
    return _error("Unable to clone the object.");

  shortDesc = get_short(ob);
  fileName = file_name(ob);
  dest = caller;

  result = ob->move(dest);

  if(result == MOVE_TOO_HEAVY) {
    ob->move(env);
    caller->set_env("cwf", str);
    return _ok("%s was moved to the room.", shortDesc);
  }

  if(result == MOVE_NO_DEST)
    return _error("The object has no destination.");

  if(result == MOVE_NOT_ALLOWED)
    return _error(
      "You are not allowed to carry the object."
    );

  if(result == MOVE_DESTRUCTED)
    return _error("The object has been destructed.");

  if(caller->query_env("custom_clone") && wizardp(caller))
    custom = caller->query_env("custom_clone");

  if(custom) {
    tmp = custom;
    tmp = replace_string(tmp, "$O", shortDesc);
    tmp = replace_string(tmp, "$N", caller->query_name());
    tell_them(capitalize(tmp) + "\n");
  } else {
    tell_them(
      capitalize(caller->query_name()) +
      " creates " + shortDesc + ".\n"
    );
  }

  _ok(
    "%s cloned to %s (%s).",
    fileName, get_short(dest), file_name(dest)
  );

  caller->set_env("cwf", str);

  return 1;
}

string query_help(object _caller) {
  return
"SYNTAX: clone <file>\n\n"
"This command produces a clone of a file.\n\n"
"See also: dest";
}
