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

private string *listOfNamesInEmote;
private mixed *targetedUsers;

private void parseLiving(string arg);
private int getUsersTargeted();
private void printEmotesToTargets(string arg,
    object *targetsToPrintTo);
private object *getTargetsToPrintTo();

mixed main(object _caller, string arg) {
  object *targetsToPrintTo;

  if(!arg)
    return "Syntax: emote <message>\n";

  listOfNamesInEmote = ({});
  targetedUsers = ({});
  targetsToPrintTo = ({});

  if(strsrch(arg, "$") != -1)
    parseLiving(arg);

  targetsToPrintTo = getTargetsToPrintTo();

  if(listOfNamesInEmote)
    if(!getUsersTargeted())
      return 0;

  if(targetsToPrintTo) {
    printEmotesToTargets(arg, targetsToPrintTo);
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

private void parseLiving(string arg) {
  int i, j;
  string tmp, currItem;
  string *tmpArray;

  tmpArray = explode(arg, " ");

  for(i = 0; i < sizeof(tmpArray); i++) {
    currItem = tmpArray[i];

    if(currItem[0] == '$') {
      for(j = sizeof(currItem) - 1; j > 0; j--) {
        if((currItem[j] >= 65 && currItem[j] <= 90)
        || (currItem[j] >= 97 && currItem[j] <= 122)) {
          tmp = currItem[1..j];
          if(!listOfNamesInEmote)
            listOfNamesInEmote =
              ({ capitalize(tmp) });
          else
            listOfNamesInEmote +=
              ({ capitalize(tmp) });

          break;
        }
      }
    }
  }
}

private int getUsersTargeted() {
  mixed *userListAndPossessive = ({});
  object tmp;
  int i;

  for(i = 0; i < sizeof(listOfNamesInEmote); i++) {
    if(listOfNamesInEmote[i][<2..<1] == "'s") {
      tmp = present(
        lower_case(listOfNamesInEmote[i][0..<3]),
        environment(TP));

      if(!tmp)
        return
          notify_fail(
            capitalize(listOfNamesInEmote[i][0..<3]) +
            " is not present!\n");

      userListAndPossessive += ({ tmp, 1 });
    } else if(listOfNamesInEmote[i][<2..<1] == "s'") {
      tmp = present(
        lower_case(listOfNamesInEmote[i][0..<2]),
        environment(TP));

      if(!tmp)
        return
          notify_fail(
            capitalize(listOfNamesInEmote[i][0..<2]) +
            " is not present!\n");

      userListAndPossessive += ({ tmp, 1 });
    } else {
      tmp = present(
        lower_case(listOfNamesInEmote[i]),
        environment(TP));

      if(!tmp)
        return
          notify_fail(
            capitalize(listOfNamesInEmote[i]) +
            " is not present!\n");

      userListAndPossessive += ({ tmp, 0 });
    }
  }

  targetedUsers = userListAndPossessive;

  return 1;
}

private void printEmotesToTargets(string arg,
    object *targetsToPrintTo) {
  int i;
  string tmpEmote, tmpName, tmpEmote2;
  string currTargetName;
  object currTarget;
  object *excludeList;

  arg = replace_string(arg, "$", "");
  tmpEmote = arg;

  for(i = 0; i < sizeof(targetsToPrintTo); i++)
    if(targetsToPrintTo[i] == 0)
      targetsToPrintTo -= ({ targetsToPrintTo[i] });

  for(i = 0; i < sizeof(listOfNamesInEmote); i++) {
    tmpName = listOfNamesInEmote[i];
    if(strsrch(tmpEmote,
        lower_case(listOfNamesInEmote[i])) != -1)
      tmpEmote = replace_string(tmpEmote,
        lower_case(listOfNamesInEmote[i]), tmpName);
  }

  tmpEmote2 = tmpEmote;

  if(tmpEmote2[<1..<1] != "."
  && tmpEmote2[<1..<1] != "!"
  && tmpEmote2[<1..<1] != "?")
    tmpEmote2 += ".";

  tell_me("You emote: " + TPQCN + " " + tmpEmote2 +
    "\n");

  for(i = 0; i < sizeof(targetsToPrintTo); i++) {
    tmpEmote2 = tmpEmote;
    currTarget = targetsToPrintTo[i];
    currTargetName =
      capitalize(currTarget->query_name());

    if(targetedUsers[i + 1] == 1
    && currTargetName[<1] == 's')
      tmpEmote2 = replace_string(tmpEmote2,
        currTargetName + "'", "your");
    else if(targetedUsers[i + 1] == 1)
      tmpEmote2 = replace_string(tmpEmote2,
        currTargetName + "'s", "your");
    else
      tmpEmote2 = replace_string(tmpEmote2,
        currTargetName, "you");

    if(strsrch(tmpEmote2, TPQCN + "'s") != -1
    || strsrch(tmpEmote2, TPQCN + "'") != -1) {
      tmpEmote2 = replace_string(tmpEmote2,
        TPQCN + "'s", "his/her");
      tmpEmote2 = replace_string(tmpEmote2,
        TPQCN + "'", "his/her");
    }

    if(strsrch(tmpEmote2, TPQCN) != -1)
      tmpEmote2 = replace_string(tmpEmote2,
        TPQCN, "he/she");

    if(tmpEmote2[<1..<1] != "."
    && tmpEmote2[<1..<1] != "!"
    && tmpEmote2[<1..<1] != "?")
      tmpEmote2 += ".";

    tell(currTarget, TPQCN + " " + tmpEmote2 + "\n");
  }

  tmpEmote2 = tmpEmote;

  if(tmpEmote2[<1..<1] != "."
  && tmpEmote2[<1..<1] != "!"
  && tmpEmote2[<1..<1] != "?")
    tmpEmote += ".";

  excludeList = targetsToPrintTo + ({ TP });

  tell_them(TPQCN + " " + tmpEmote + "\n",
    excludeList);
}

private object *getTargetsToPrintTo() {
  int i;
  object *tmpArray = ({});

  for(i = 0; i < sizeof(listOfNamesInEmote); i++)
    if(member_array(
        find_living(
          lower_case(listOfNamesInEmote[i])),
        tmpArray) == -1)
      tmpArray += ({ find_living(
        lower_case(listOfNamesInEmote[i])) });

  tmpArray -= ({ TP });

  return tmpArray;
}
