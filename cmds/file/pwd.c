/**
 * @file /cmds/file/pwd.c
 *
 * Displays the current working directory and file.
 *
 * @created 2005-04-08 - Tacitus
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2005-04-08 - Tacitus - Created
 * 2026-03-23 - Gesslar - Updated to current conventions
 */

inherit STD_CMD;

void setup() {
  usage_text = "pwd";
  help_text =
"This command will inform you of your current working "
"directory and current working file. This is important, "
"as sometimes if you don't supply arguments to commands, "
"they default to your current working file or directory "
"(such as ls, ed, or update).";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string _arg) {
  tell(caller, sprintf(
    "Current working directory: %s\n"
    "Current working file: %s\n",
    caller->query_env("cwd"),
    caller->query_env("cwf")));

  return 1;
}
