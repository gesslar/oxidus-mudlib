/**
 * @file /cmds/std/who.c
 *
 * Display all currently logged-in users.
 *
 * @created 2005-04-08 - Tacitus @ LPUniversity
 * @last_modified 2006-10-04 - Tacitus
 *
 * @history
 * 2005-04-08 - Tacitus - Created
 * 2006-10-04 - Tacitus - Last edited
 */

inherit STD_CMD;

private object *addArray(object *oldArr, object *newArr);
private int sortName(object ob1, object ob2);

mixed main(object tp, string _arg) {
  string ret;
  object *list;
  object *adminArr, *devArr, *userArr;
  int i;

  adminArr = ({});
  devArr = ({});
  userArr = ({});
  ret = "";

  ret += read_file("/adm/etc/logo") + "\n\n";

  ret += sprintf("%10s  %10s\n\n",
    "Username [* editing, + in input]", "Idle");

  list = users();

  foreach(object name in list) {
    if(adminp(name)
    && name->query_real_name() != "login")
      adminArr += ({ name });
    else if(devp(name))
      devArr += ({ name });
    else
      userArr += ({ name });
  }

  adminArr = sort_array(adminArr, "sortName");
  devArr = sort_array(devArr, "sortName");
  userArr = sort_array(userArr, "sortName");

  list = ({});
  list = addArray(list, adminArr);
  list = addArray(list, devArr);
  list = addArray(list, userArr);

  for(i = 0; i < sizeof(list); i++) {
    string tag;

    if(!(string)list[i]->query_name())
      continue;

    if(list[i]->query_real_name() == "login")
      tag = "[ LOGIN ]";
    else if(adminp(list[i]))
      tag = "[ Admin ]";
    else if(devp(list[i]))
      tag = "[ Dev   ]";
    else
      tag = "[ User  ]";

    ret += sprintf(" %-s   %-15s %15s\n", tag,
      list[i]->query_name() +
      (in_edit(list[i]) ? "*" : "") +
      (in_input(list[i]) ? "+" : ""),
      query_idle(list[i]) / 60 + "m");
  }

  ret += "\n";
  tell_me(ret);
  return 1;
}

private int sortName(/** @type {STD_BODY} */ object ob1,
    /** @type {STD_BODY} */ object ob2) {
  if(ob1->query_name() > ob2->query_name())
    return 1;
  else if(ob1->query_name() < ob2->query_name())
    return -1;
  else
    return 0;
}

private object *addArray(object *oldArr, object *newArr) {
  foreach(object name in newArr)
    oldArr += ({ name });

  return oldArr;
}

string query_help(object _caller) {
  return
" SYNTAX: who\n\n"
"This command will display all the users who are "
"currently logged\ninto " + mud_name() + ". It also "
"lets you know if they are currently\nediting, in "
"input, and/or idle.\n";
}
