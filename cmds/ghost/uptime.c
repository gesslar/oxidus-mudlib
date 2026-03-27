/**
 * @file /cmds/ghost/uptime.c
 * @description Uptime command displaying how long the mud has been running.
 *
 * @created 2005-04-08 - Tacitus
 * @last_modified 2005-10-20 - Tacitus
 *
 * @history
 * 2005-04-08 - Tacitus - Created
 */

inherit STD_CMD;

mixed main(object caller, string _arg) {
  tell(caller,
    mud_name() + " has been running since "
    + ctime(time() - uptime()) + "\n"
  );

  return 1;
}

string query_help(object _caller) {
  return
"SYNTAX: uptime\n\n"
"This command will tell you how long the mud has been "
"running.";
}
