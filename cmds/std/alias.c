/**
 * @file /cmds/std/alias.c
 *
 * Alias command for managing player command aliases.
 *
 * @created 2006-01-14 - Tacitus @ LPUniversity
 * @last_modified 2006-01-14 - Tacitus
 *
 * @history
 * 2006-01-14 - Tacitus - Created
 */

inherit STD_CMD;

private mixed listAliases(object tp, int global);

mixed main(/** @type {STD_PLAYER} */ object tp,
    string args) {
  string verb, alias;
  mapping aliases;

  if(!args)
    return listAliases(tp, 0);
  else if(args == "-g")
    return listAliases(tp, 1);

  if(sscanf(args, "%s %s", verb, alias) == 2) {
    if(verb == "alias" || verb == "unalias")
      return _error("You may not alias 'alias' or 'unalias'.");

    tp->add_alias(verb, alias);

    return _ok("Added alias `%s`\n>> %s", verb, alias);
  }

  aliases = tp->get_aliases(0);
  if(of(args, aliases)) {
    alias = aliases[args];
    return _info("%s is aliased to:\n>> %s", args, alias);
  }
  aliases = tp->get_aliases(1);
  if(of(args, aliases)) {
    alias = aliases[args];
    return _info("%s is globally aliased to:\n>> %s",
      args, alias);
  }

  return _error("No such alias `%s` defined.", args);
}

private mixed listAliases(object tp, int global) {
  mapping data = tp->get_aliases(global);
  string *sortedKeys, header, footer;
  int i, sz;
  string *out;

  if(!mapp(data))
    return "Aliases: No aliases defined.\n";

  if(global) {
    header = "Current global aliases:";
    footer = "Number of global aliases: %d";
  } else {
    header = "Current local aliases:";
    footer = "Number of local aliases: %d";
  }

  sortedKeys = keys(data);
  sz = sizeof(sortedKeys);

  if(!sz)
    return "Aliases: No aliases defined.\n";

  sortedKeys = sort_array(sortedKeys, 1);

  out = allocate(sz + 4);
  out[0] = header;
  out[1] = "";
  out[<2] = "";
  out[<1] = sprintf(footer, sz);

  for(i = 0; i < sz; i++)
    out[i + 2] = sprintf("%-10s %-20s",
      sortedKeys[i], data[sortedKeys[i]]);

  return out;
}

string query_help(object _caller) {
  return
" SYNTAX: alias [-g] <[$]verb> <alias>\n\n"
"This command allows you to display your current aliases "
"and\nadd new ones. If you would like to view your "
"current aliases,\nthen simply type 'alias'. If you'd "
"like to see your current\naliases plus global aliases "
"that affect you, provide -g as an\nargument by itself. "
"To add an alias, you must provide two arguments;\nthe "
"verb you wish to alias and the actual alias. You may "
"use $n,\nwhere n represents a number that represents "
"the index of a word as an\nargument when the alias is "
"parsed. (example: 'alias say say %2 %1'\nwill result "
"in the first and second word to be always switched "
"when\nyou type say). You can also use $*, which will "
"be replaced with all\nremaining words. If you place $ "
"in front of the verb, then all commands\nbeginning "
"with that verb will be parsed with that alias\n"
"(ex. 'alias $: emote $*' will result in any command "
"beginning with ':' to\nbe parsed as emote $*).\n\n"
"See also: unalias\n";
}
