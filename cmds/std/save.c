/**
 * @file /cmds/std/save.c
 *
 * A simple save command to persist player data.
 *
 * @created 2005-06-30 - Gwegster @ LPUniversity
 * @last_modified 2005-10-08 - Tacitus
 *
 * @history
 * 2005-06-30 - Gwegster - Created
 * 2005-10-08 - Tacitus - Last edited
 */

inherit STD_CMD;

mixed main(/** @type {STD_BODY} */ object caller,
    string _args) {
  caller->save_body();
  tell_me("Successful [save]: User saved.\n");
  return 1;
}

string query_help(object _caller) {
  return
" SYNTAX: save\n\n"
"This command will save your character data.\n\n"
"See also: quit\n";
}
