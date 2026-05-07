//path.c

//Tacitus @ LPUniversity
//07-APR-05
//Wizard command

inherit STD_CMD;

mixed main(object caller, string str) {
  string action, arg, *path, ret;
  int i;

  ret = "";

  if(!str || !sscanf(str, "-%s %s", action, arg)) {
    path = this_body()->query_path();
    for(i = 0; i < sizeof(path); i++) {
      ret += path[i][0..<2] + ":";
    }

    tell_me("Current path: " + ret[0..<2] + "\n");
    return 1;
  }

  if(arg[<1..< 1] != "/") {
    arg += "/";
  }

  switch(action) {
    case "add" :
      this_body()->add_path(arg);
      break;

    case "remove" :
      this_body()->rem_path(arg);
      break;

    default :
      tell_me("Syntax: path [-add || -remove <path>]\n");
      break;
  }

  path = this_body()->query_path();

  for(i = 0; i < sizeof(path); i++) {
    ret += path[i][0..<2] + ":";
  }

  tell_me("New path: " + ret[0..<2] + "\n");

  return 1;
}

string help(object caller) {
  return " SYNTAX: path <-[add/remove]> <directory>\n\n"
    "This command allows you to modify the paths that the mud will\n"
    "look in to try and parse your commands sent to the mud. This command\n"
    "can make your user unusable if you remove important directories (such\n"
    "as the directory that contains this command). If no arguments are\n"
    "supplied, then it will tell you your current path.\n";
}
