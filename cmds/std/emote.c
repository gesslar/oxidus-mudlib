/**
 * @file /cmds/std/emote.c
 *
 * Custom emote command with targeted name substitution.
 *
 * @created 2005-04-02 - Tacitus @ LPUniversity
 * @last_modified 2006-06-19 - Parthenon
 *
 * @history
 * 2005-04-02 - Tacitus - Created
 * 2005-10-08 - Tacitus - Last edited
 * 2006-06-19 - Parthenon - Last edited
 */

#define TP this_body()
#define TPQN this_body()->query_name()
#define TPQCN capitalize(this_body()->query_name())

private string *list_of_names_in_emote;
private mixed *targeted_users;

private void parse_living(string arg);
private int get_users_targeted();
private void print_emotes_to_targets(string arg, object *targets_to_print_to);
private object *get_targets_to_print_to();

mixed main(object _caller, string arg) {
  object *targets_to_print_to;

  if(!arg)
    return "Syntax: emote <message>\n";

  list_of_names_in_emote = ({});
  targeted_users = ({});
  targets_to_print_to = ({});

  if(strsrch(arg, "$") != -1)
    parse_living(arg);

  targets_to_print_to = get_targets_to_print_to();

  if(list_of_names_in_emote)
    if(!get_users_targeted())
      return 0;

  if(targets_to_print_to) {
    print_emotes_to_targets(arg, targets_to_print_to);
    return 1;
  }

  tell_me("You emote: " + TPQCN + " " + arg + "\n");
  tell_them(TPQCN + " " + arg + "\n");
  return 1;
}

string query_help(object _caller) {
  return
" SYNTAX: emote <string>\n\n"
"This command allows you to do custom emotes. Example, "
"if you\ntype 'emote smiles serenely' then the others "
"in the room will\nsee '" + TPQCN + " smiles "
"serenely.' You can also use people's names\nwho are "
"present in the room. If you type 'emote smiles "
"serenely\nat $parthenon' and Parthenon is present, "
"then Parthenon will see\n'" + TPQCN + " smiles "
"serenely at you.' and the room will see\n'" + TPQCN +
" smiles serenely at Parthenon.' You may also use "
"the\npossessive form of the person's name like "
"'$parthenon's' and\nParthenon will see 'your'.\n\n"
"See also: say\n";
}

private void parse_living(string arg) {
  int i, j;
  string tmp, curr_item;
  string *tmp_array;

  tmp_array = explode(arg, " ");

  for(i = 0; i < sizeof(tmp_array); i++) {
    curr_item = tmp_array[i];

    if(curr_item[0] == '$') {
      for(j = sizeof(curr_item) - 1; j > 0; j--) {
        if((curr_item[j] >= 65 && curr_item[j] <= 90)
        || (curr_item[j] >= 97 && curr_item[j] <= 122)) {
          tmp = curr_item[1..j];
          if(!list_of_names_in_emote)
            list_of_names_in_emote =
              ({ capitalize(tmp) });
          else
            list_of_names_in_emote +=
              ({ capitalize(tmp) });

          break;
        }
      }
    }
  }
}

private int get_users_targeted() {
  mixed *user_list_and_possessive = ({});
  object tmp;
  int i;

  for(i = 0; i < sizeof(list_of_names_in_emote); i++) {
    if(list_of_names_in_emote[i][<2..<1] == "'s") {
      tmp = present(
        lower_case(list_of_names_in_emote[i][0..<3]),
        environment(TP));

      if(!tmp)
        return
          notify_fail(
            capitalize(list_of_names_in_emote[i][0..<3]) +
            " is not present!\n");

      user_list_and_possessive += ({ tmp, 1 });
    } else if(list_of_names_in_emote[i][<2..<1] == "s'") {
      tmp = present(
        lower_case(list_of_names_in_emote[i][0..<2]),
        environment(TP));

      if(!tmp)
        return
          notify_fail(
            capitalize(list_of_names_in_emote[i][0..<2]) +
            " is not present!\n");

      user_list_and_possessive += ({ tmp, 1 });
    } else {
      tmp = present(
        lower_case(list_of_names_in_emote[i]),
        environment(TP));

      if(!tmp)
        return
          notify_fail(
            capitalize(list_of_names_in_emote[i]) +
            " is not present!\n");

      user_list_and_possessive += ({ tmp, 0 });
    }
  }

  targeted_users = user_list_and_possessive;

  return 1;
}

private void print_emotes_to_targets(string arg, object *targets_to_print_to) {
  int i;
  string tmp_emot, tmp_name, tmp_emote2;
  string curr_target_name;
  object curr_target;
  object *exclude_list;

  arg = replace_string(arg, "$", "");
  tmp_emot = arg;

  for(i = 0; i < sizeof(targets_to_print_to); i++)
    if(targets_to_print_to[i] == 0)
      targets_to_print_to -= ({ targets_to_print_to[i] });

  for(i = 0; i < sizeof(list_of_names_in_emote); i++) {
    tmp_name = list_of_names_in_emote[i];
    if(strsrch(tmp_emot,
        lower_case(list_of_names_in_emote[i])) != -1)
      tmp_emot = replace_string(tmp_emot,
        lower_case(list_of_names_in_emote[i]), tmp_name);
  }

  tmp_emote2 = tmp_emot;

  if(tmp_emote2[<1..<1] != "."
  && tmp_emote2[<1..<1] != "!"
  && tmp_emote2[<1..<1] != "?")
    tmp_emote2 += ".";

  tell_me("You emote: " + TPQCN + " " + tmp_emote2 +
    "\n");

  for(i = 0; i < sizeof(targets_to_print_to); i++) {
    tmp_emote2 = tmp_emot;
    curr_target = targets_to_print_to[i];
    curr_target_name =
      capitalize(curr_target->query_name());

    if(targeted_users[i + 1] == 1
    && curr_target_name[<1] == 's')
      tmp_emote2 = replace_string(tmp_emote2,
        curr_target_name + "'", "your");
    else if(targeted_users[i + 1] == 1)
      tmp_emote2 = replace_string(tmp_emote2,
        curr_target_name + "'s", "your");
    else
      tmp_emote2 = replace_string(tmp_emote2,
        curr_target_name, "you");

    if(strsrch(tmp_emote2, TPQCN + "'s") != -1
    || strsrch(tmp_emote2, TPQCN + "'") != -1) {
      tmp_emote2 = replace_string(tmp_emote2,
        TPQCN + "'s", "his/her");
      tmp_emote2 = replace_string(tmp_emote2,
        TPQCN + "'", "his/her");
    }

    if(strsrch(tmp_emote2, TPQCN) != -1)
      tmp_emote2 = replace_string(tmp_emote2,
        TPQCN, "he/she");

    if(tmp_emote2[<1..<1] != "."
    && tmp_emote2[<1..<1] != "!"
    && tmp_emote2[<1..<1] != "?")
      tmp_emote2 += ".";

    tell(curr_target, TPQCN + " " + tmp_emote2 + "\n");
  }

  tmp_emote2 = tmp_emot;

  if(tmp_emote2[<1..<1] != "."
  && tmp_emote2[<1..<1] != "!"
  && tmp_emote2[<1..<1] != "?")
    tmp_emot += ".";

  exclude_list = targets_to_print_to + ({ TP });

  tell_them(TPQCN + " " + tmp_emot + "\n",
    exclude_list);
}

private object *get_targets_to_print_to() {
  int i;
  object *tmp_array = ({});

  for(i = 0; i < sizeof(list_of_names_in_emote); i++)
    if(member_array(
        find_living(
          lower_case(list_of_names_in_emote[i])),
        tmp_array) == -1)
      tmp_array += ({ find_living(
        lower_case(list_of_names_in_emote[i])) });

  tmp_array -= ({ TP });

  return tmp_array;
}
