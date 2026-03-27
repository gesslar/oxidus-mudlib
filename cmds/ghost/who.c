/**
 * @file /cmds/ghost/who.c
 * @description Who command displaying currently connected users.
 *
 * @created 2005-04-08 - Tacitus
 * @last_modified 2006-10-04 - Tacitus
 *
 * @history
 * 2005-04-08 - Tacitus - Created
 */

inherit STD_CMD;

private int sortName(
  /** @type {STD_BODY} */ object ob1,
  /** @type {STD_BODY} */ object ob2
);
private object *addArray(object *oldArr, object *newArr);

mixed main(object caller, string _arg) {
  string ret = "";
  object *list;
  object *adminArr = ({});
  object *devArr = ({});
  object *userArr = ({});

  ret += read_file("/adm/etc/logo") + "\n\n";
  ret += sprintf("%10s  %10s\n\n",
    "Username [* editing, + in input]", "Idle"
  );

  list = users();

  foreach(object ob in list) {
    if(adminp(ob) && ob->query_real_name() != "login")
      adminArr += ({ ob });
    else if(devp(ob))
      devArr += ({ ob });
    else
      userArr += ({ ob });
  }

  adminArr = sort_array(adminArr, (: sortName :));
  devArr = sort_array(devArr, (: sortName :));
  userArr = sort_array(userArr, (: sortName :));

  list = ({});
  list = addArray(list, adminArr);
  list = addArray(list, devArr);
  list = addArray(list, userArr);

  for(int i = 0; i < sizeof(list); i++) {
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
      list[i]->query_name()
      + (in_edit(list[i]) ? "*" : "")
      + (in_input(list[i]) ? "+" : ""),
      query_idle(list[i]) / 60 + "m"
    );
  }

  ret += "\n";
  tell(caller, ret);

  return 1;
}

private int sortName(
    /** @type {STD_BODY} */ object ob1,
    /** @type {STD_BODY} */ object ob2) {
  if(ob1->query_name() > ob2->query_name())
    return 1;
  else if(ob1->query_name() < ob2->query_name())
    return -1;

  return 0;
}

private object *addArray(object *oldArr, object *newArr) {
  foreach(object ob in newArr)
    oldArr += ({ ob });

  return oldArr;
}

string query_help(object _caller) {
  return
"SYNTAX: who\n\n"
"This command will display all the users who are currently "
"logged into " + mud_name() + ". It also lets you know if "
"they are currently editing, in input, and/or idle.";
}
