/**
 * @file /cmds/wiz/gauge.c
 *
 * Short description of this file and its purpose.
 *
 * @created 2025-03-10 - Gesslar
 * @last_modified 2025-03-10 - Gesslar
 *
 * @history
 * 2025-03-10 - Gesslar - Created
 */

mixed main(object tp, string cmd) {
  mapping before, after;
  int start_cost, end_cost;
  int stime, usertime, eval_cost;

  if(!cmd)
    return "usage: gauge <command>\n";

  before = rusage();

  eval_cost = tp->force_me(cmd);
  after = rusage();
  usertime = after["utime"] - before["utime"];
  stime = after["stime"] - before["stime"];

  return sprintf(
    "%d milliseconds of system time.\n"+
    "%d milliseconds of user time.\n" +
    "%d eval cost.\n",
    stime, usertime, eval_cost);
}

string query_help(object user) {
  return @text
Usage: gauge <command>

The gauge command will execute the given command, and will tell you how many
milliseconds of CPU the command took.
text;
}
