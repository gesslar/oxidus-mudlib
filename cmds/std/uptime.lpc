/**
 * @file /cmds/std/uptime.c
 *
 * Display how long the MUD has been running.
 *
 * @created 2005-04-08 - Tacitus @ LPUniversity
 * @last_modified 2005-10-20 - Tacitus
 *
 * @history
 * 2005-04-08 - Tacitus - Created
 * 2005-10-20 - Tacitus - Last edited
 */

inherit STD_CMD;

mixed main(object tp, string _arg) {
  tell_me(mud_name() + " has been running since " +
    ctime(time() - uptime()) + "\n");
  return 1;
}

string query_help(object _caller) {
  return
" SYNTAX: uptime\n\n"
"This command will tell you how long the mud has been "
"running.\n";
}
