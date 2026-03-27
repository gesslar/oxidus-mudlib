/**
 * @file /cmds/std/unalias.c
 *
 * Command to unalias a verb.
 *
 * @created 2024-10-06 - Gesslar
 * @last_modified 2024-10-06 - Gesslar
 *
 * @history
 * 2024-10-06 - Gesslar - Created
 */

inherit STD_CMD;

void setup() {
  usage_text = "unalias <alias>";
  help_text =
"This command enables you to remove an alias from your alias list. "
"Aliases are shortcuts you have set up for commands, allowing you to use "
"shorthand instead of the full command. This command allows you to remove "
"aliases you have set up, freeing up the shortcut for other uses.";
}

mixed main(object caller, string args) {
  if(!args)
    return _usage(caller);

  if(caller->remove_alias(args))
    return _ok("`%s` is no longer aliased.", args);

  return _error("`%s` is not aliased.", args);
}
