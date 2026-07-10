/* echo.c

 Tacitus @ LPUniversity
 12-AUG-06
 Simple echo command

*/

// Last Change: 2024/02/04: Gesslar
// - General formatting and new code support.

inherit STD_CMD;

mixed main(object caller, string str) {
  object room = environment(caller);

  if(!str) {
    return "Syntax: echo <argument>";
  }

  tell(caller, "You echoed to the room: " + str);
  tell_down(room, str, 0, caller);

  return 1;
}

string help(object caller) {
  return
"SYNTAX: echo <string>\n\n"
"This command allows you to echo a string of text to the room you are "
"currently in. Other individuals in the room will see the argument that you "
"pass to this  exactly as you typed it with a newline (aka line break) after "
"it.\n\n"
"For example, if you type 'echo The room shakes.' then everyone else in the "
"room will see the following printed 'The room shakes.'.";
}
