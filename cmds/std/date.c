/**
 * @file /cmds/std/date.c
 *
 * Display the current date.
 *
 * @created 2005-11-10 - Tacitus @ LPUniversity
 * @last_modified 2005-11-10 - Tacitus
 *
 * @history
 * 2005-11-10 - Tacitus - Created
 */

inherit STD_CMD;

mixed main(object tp, string _str) {
  tell(tp, sprintf(" The current date is %s.\n",
    ctime(time())));
  return 1;
}

string query_help(object _caller) {
  return
"SYNTAX: date\n\n"
"This command will return the current date. It is "
"important\nto note though that the date is in mud "
"time, not in your\nlocaltime.\n";
}
