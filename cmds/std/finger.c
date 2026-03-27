/**
 * @file /cmds/std/finger.c
 *
 * Display information about a user.
 *
 * @created 2006-03-26 - Tacitus @ LPUniversity
 * @last_modified 2006-03-26 - Tacitus
 *
 * @history
 * 2006-03-26 - Tacitus - Created
 */

inherit STD_CMD;

mixed main(object tp, string user) {
  if(!user)
    return "Syntax: finger <user>\n";

  tell_me(FINGER_D->finger_user(user));

  return 1;
}

string query_help(object _caller) {
  return
" SYNTAX: finger <user>\n\n"
"This command will display information regarding an\n"
"existing user such as their e-mail address, last "
"login\nrank, plan, etc. This command will be able to "
"retrieve\nthe information even if they are offline.\n";
}
