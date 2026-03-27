/**
 * @file /cmds/std/version.c
 *
 * Display version information about the MUD software.
 *
 * @created 2006-05-05 - Tacitus @ LPUniversity
 * @last_modified 2006-05-05 - Tacitus
 *
 * @history
 * 2006-05-05 - Tacitus - Created
 */

inherit STD_CMD;

mixed main(object tp, string _args) {
  tell(tp, mud_name() + " is running " + lib_name() +
    " " + lib_version() + " (" + baselib_name() + " " +
    baselib_version() + ")\n");
  tell(tp, "on " + driver_version() + " on " +
    arch() + ".\n");

  return 1;
}

string query_help(object _caller) {
  return
" SYNTAX: version\n\n"
"This command returns version information about the "
"software running this mud.";
}
