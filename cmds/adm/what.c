/* what.c

 Tacitus @ LPUniversity
 12-AUG-06
 Displays activity info about users

*/

inherit STD_CMD;

mixed main(/** @type {STD_PLAYER} */ object caller, string _arg) {
  if(!adminp(caller))
    return 0;

  tell(caller, "=----------------------------------------------=\n");
  tell(caller, sprintf(" %-15s %-s \n", "Username", "Last Command"));
  tell(caller, "=----------------------------------------------=\n");

  foreach(/** @type {STD_PLAYER} */ object user in users()) {
    string *commandHistory = user->query_command_history();
    if(!living(user)) continue;

    if(sizeof(commandHistory) <= 0)
      tell(caller, sprintf("  %-15s %s\n",
        user->query_name(), "<none>"));
    else
      tell(caller, sprintf("  %-15s %s\n",
        user->query_name(),
        commandHistory[sizeof(commandHistory) - 1]));
  }

  tell(caller, "=----------------------------------------------=\n");

  return 1;
}

string query_help(object _caller) {
  return " SYNTAX: what\n\n"
    "This command allows you to view the last command executed by\n"
    "all users logged in. It is recommended that you review any\n"
    "privacy policy that your mud might have before using this tool.\n";
}
