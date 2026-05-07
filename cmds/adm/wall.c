//wall.c

//Tacitus @ LPUniversity
//01-JULY-05
//Admin wall cmd

inherit STD_CMD;

mixed main(/** @type {STD_PLAYER} */ object caller, string str) {
  object *all_users;

  if(!adminp(previous_object()))
    return _error("Error [wall]: Access denied.");

  if(!stringp(str)) {
    _error("You must supply an argument. Syntax: wall <msg>");
    return 1;
  }

  all_users = users();

  foreach(object user in all_users) {
    tell(user, "**** System Wide Message From: "
      + capitalize(caller->query_real_name()) + " at "
      + ctime(time()) + " ****\n\n\t" + str + "\n");
  }

  return 1;
}

string query_help(object _caller) {
  return " SYNTAX: wall <msg>\n\n"
    + "This command pages a message to everybody on the MUD.\n";
}
