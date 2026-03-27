/**
 * @file /cmds/object/functions.c
 * @description Command to list functions contained in an object.
 *
 * @created 2006-08-16 - Parthenon@LPUniversity
 * @last_modified 2024-02-04 - Gesslar
 *
 * @history
 * 2006-08-16 - Parthenon@LPUniversity - Created
 * 2024-02-04 - Gesslar - General formatting
 */

inherit STD_CMD;

public mixed main(
  /** @type {STD_PLAYER} */ object caller, string arg
) {
  object ob;
  string *funcs, ret;

  if(!arg || arg == "")
    return _error(
      "SYNTAX: functions <object id>|<filename>"
    );

  ob = get_object(arg);

  if(!ob)
    return _error("Could not find object %s.", arg);

  funcs = functions(ob);
  funcs = distinct_array(funcs);
  funcs = sort_array(funcs, 1);

  ret = sprintf("%-#79.3s\n\n", implode(funcs, "\n"));

  caller->page(ret);

  return 1;
}

string query_help(object _caller) {
  return
"SYNTAX: functions <object>\n\n"
"This command will show the functions contained in "
"<object>. You may use the id of the object if it is in "
"your inventory or environment, otherwise you may use the "
"filename of the object to try and locate it.";
}
