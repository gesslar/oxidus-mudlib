/* which.c

 Tacitus @ LPUniversity
 05-Oct-06
 Scans command path for command

*/

inherit STD_CMD;

mixed main(object caller, string args) {
  string *command_path = this_body()->query_path();
  mixed *actions = previous_object()->query_commands();
  mapping aliases = this_body()->get_aliases(1);
  int i, is_located = 0;

  if(!args) {
    return notify_fail("Error: Syntax: which <verb/command>\n");
  }

  for(i = 0; i < sizeof(command_path); i++) {
    if(file_exists(command_path[i] + args + ".c")) {
      is_located = 1;
      tell_me(command_path[i] + args + "\n");
    }
  }

  for(i = 0; i < sizeof(actions); i++) {
    if(actions[i][0] == args) {
      is_located = 1;
      tell_me("Local:  " + actions[i][2] + "\n");
    }
  }

  if(mapp(aliases) && aliases[args]) {
    is_located = 1;
    tell_me("Alias: " + args + " -> " + aliases[args] + "\n");
  }

  if(member_array(args, SOUL_D->query_emotes()) != -1) {
    is_located = 1;
    tell_me("Soul: " + args + "\n");
  }

  if(member_array(args + "/t", SOUL_D->query_emotes()) != -1) {
    is_located = 1;
    tell_me("Targetted Soul: " + args + "\n");
  }

  if(environment(previous_object())->valid_exit(args)) {
    is_located = 1;
    tell_me("Local Exit: " +
      environment(this_body())->query_exit(args) + "\n");
  }

  if(!is_located) {
    return notify_fail("Error: '" + args
      + "' not found in " +  implode(command_path, ", ")
       + " nor via a local add_action, alias, soul, or exit.\n");
  }

  return 1;
}

string help(object caller) {
  return " SYNTAX: which <verb/command>\n\n"
    "This command will search through your command path, local\n"
    "and global aliases, through local verbs added via add_action,\n"
    "souls, and exits for the specified command (aka verb).\n\n"
    " See also: path, alias\n";
}
