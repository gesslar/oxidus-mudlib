/**
 * @file /cmds/object/dest.c
 * @description Command to destruct objects from memory.
 *
 * @created 2005-10-31 - Tacitus@LPUniversity
 * @last_modified 2024-02-04 - Gesslar
 *
 * @history
 * 2005-10-31 - Tacitus@LPUniversity - Created
 * 2006-07-19 - Parthenon - Edited
 * 2024-02-04 - Gesslar - General formatting
 */

inherit STD_CMD;

public mixed main(
  /** @type {STD_PLAYER} */ object caller, string str
) {
  string custom, tmp;
  object ob, env, room;
  int cloned;
  string shortDesc, callerName;

  callerName = caller->query_name();

  if(!str)
    str = caller->query_env("cwf");

  ob = get_object(str);
  if(ob && userp(ob) && !adminp(caller))
    return _error("You can't destruct players.");

  room = environment(caller);

  if(ob) {
    cloned = clonep(ob);
    env = environment(ob);

    if(living(ob))
      shortDesc = ob->query_name();
    else if(environment(ob))
      shortDesc = add_article(get_short(ob));
    else
      shortDesc = file_name(ob);

    if(caller->query_env("custom_dest") &&
      wizardp(caller))
      custom = caller->query_env("custom_dest");

    if(custom) {
      tmp = custom;
      tmp = replace_string(tmp, "$O", shortDesc);
      tmp = replace_string(tmp, "$N", callerName);
    }

    catch(ob->remove());
    if(ob)
      destruct(ob);

    if(ob)
      return _error("Ok that didn't work.");

    if(custom) {
      if(env == room)
        tell_them(capitalize(tmp) + "\n");
    } else {
      if(env == room)
        tell_them(
          callerName + " destructs " +
          shortDesc + ".\n"
        );
    }

    return _ok("Destructed %s.", shortDesc);
  }

  if(str[<2..<1] != ".c")
    str += ".c";

  str = resolve_path(caller->query_env("cwd"), str);

  if(ob = find_object(str)) {
    caller->set_env("cwf", str);
    ob->remove();
    if(ob)
      destruct(ob);
    return _ok(
      "Destructing master object for '%s'.", str
    );
  }

  return _error("Object '%s' not found.", str);
}

string query_help(object _caller) {
  return
"SYNTAX: dest <object/filename>\n\n"
"This command destructs an object from memory.\n\n"
"See also: clone";
}
