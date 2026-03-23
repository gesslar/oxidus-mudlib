/**
 * @file /cmds/wiz/do.c
 * @description do command to execute a number of commands and/or repeatedly
 *
 * @created 2024-07-13 - Gesslar
 * @last_modified 2024-07-13 - Gesslar
 *
 * @history
 * 2024-07-13 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(object tp, string str) {
  mixed *all_cmds = ({ });
  string *parts;
  int all_repeat = 0;
  int done;
  int it;

  if(!str || str == "") {
    return "Syntax: do cmd,cmd,cmd,...\n";
  }

  if(str[0] == '!') {
    if(sscanf(str, "!%d %s", all_repeat, str) != 2) {
      return "Invalid repeat count.\n";
    }

    str = trim(str);
  }

  parts = explode(str, ",");
  parts = map(parts, (: trim :));

  foreach(string part in parts) {
    int repeat = 1;
    string cmd = part;

    if(sscanf(part, "%d %s", repeat, cmd) != 2) {
      if(repeat < 1) {
        return "Invalid repeat count.\n";
      }

      cmd = trim(cmd);
    } else {
      cmd = trim(cmd);
    }

    all_cmds += ({ ({ repeat, cmd }) });
  }

  // Add 1 to all_repeat to account for the first command
  all_repeat++;

  while(all_repeat--) {
    foreach(mixed *cmd in all_cmds) {
      int repeat = cmd[0];
      string cmd_str = cmd[1];

      while(repeat--) {
        tp->force_me(cmd_str);
      }
    }
  }

  return 1;
}

string query_help(object tp) {
  string help;

  help = @text
Syntax: do cmd,cmd,cmd,... - Executes a list of commands.
        do 10 cmd - Executes the command 10 times.
        do 10 cmd,cmd,2 cmd,... - Executes the commands 10, 10, 2 times.
        do !10 cmd,cmd,cmd, ... - Repeats the commands until all commands have
                                  been executed 10 times.
        do !10 2 cmd,cmd,3 cmd, ... -- Repeats the commands until all commands
                                     have been executed 2, 2, 3 times.

text;

  help +=
  "Executes a list of commands and/or repeatedly. If a number is given, the "
  "command is executed that many times. If a list of commands is given, each "
  "command is executed the number of times specified. If the ! is used, all "
  "commands are executed the number of times specified, repeating as "
  "necessary to match the number of times specified for each commands.\n\n";

  return help;
}