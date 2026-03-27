/**
 * @file /cmds/std/colour.c
 *
 * This is the colour command that enables a player to set and
 * update their colour preferences.
 *
 * @created 2025-03-10 - Gesslar
 * @last_modified 2025-03-10 - Gesslar
 *
 * @history
 * 2025-03-10 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(/** @type {STD_PLAYER} */ object caller, string str) {
  string cmd, arg;

  if(stringp(str)) {
    if(sscanf(str, "%s %s", cmd, arg) != 2)
      cmd = str;
  } else {
    cmd = str;
  }

  switch(cmd) {
    case "on":
      caller->set_env("colour", "on");
      return _ok(caller, "Colour on.");
    case "off":
      caller->set_env("colour", "off");
      return _ok(caller, "Colour off.");
    case "list":
      return _info(caller,
        "Colour List:\n\n" + COLOUR_D->get_colour_list());
    case "show": {
      if(!arg)
        return _error(caller, "Invalid colour code.");

      if(caller->query_pref("colour") != "on")
        return _error(caller, "Colour is currently disabled.");

      int num;

      if(sscanf(arg, "%d", num) != 1)
        return _error(caller, "Invalid colour code.");

      if(num < 0 || num > 255)
        return _error(caller, "Invalid colour code.");

      string fg = "{{" + sprintf("0%'0'3d", num) + "}}";

      return _info(caller, "\n"
        "%s\'%'0'3d\' will appear like this in the "
        "foreground.{{res}}\n"
        "{{re1}}%s\'%'0'3d\' will appear like this in the "
        "background.{{res}}",
        fg, num, fg, num);
    }
    default:
      if(caller->query_pref("colour") == "on")
        return _info(caller, "Colour is currently enabled.");
      else
        return _info(caller,
          "Colour is currently disabled.");
  }
}

string query_help(object _caller) {
  return
"SYNTAX: colour [<on>|<off>|list|show [#]]\n\n"
"With no arguments, this command will tell you if you currently have "
"colour enabled or disabled. You can also toggle colour by providing the "
"arguments {{ul1}}on{{ul0}} to enable or {{ul1}}off{{ul0}} to "
"disable.\n"
"\n"
"Use the {{ul1}}list{{ul0}} argument to list all the available colours.\n"
"\n"
"Use the {{ul1}}show{{ul0}} argument to see how a specific colour code "
"will appear in the foreground and background. The argument must be a "
"number between 0 and 255.\n"
"\n"
"See also: set\n";
}
