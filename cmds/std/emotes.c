/**
 * @file /cmds/std/emotes.c
 *
 * List all available emotes.
 *
 * @created 2006-06-28 - Parthenon @ LPUniversity
 * @last_modified 2006-06-28 - Parthenon
 *
 * @history
 * 2006-06-28 - Parthenon - Created
 */

private string *singledEmotes = ({});

private void fixArray(string *arr);
private void printEmotes(string *emotes);

mixed main(object _tp, string _arg) {
  string *emotes;

  emotes = ({});
  emotes = SOUL_D->query_emotes();

  if(!emotes)
    return "Error [emotes]: There are no emotes "
      "available\n";

  singledEmotes = ({});
  fixArray(emotes);

  tell_me("\nAvailable emotes:\n");

  printEmotes(singledEmotes);

  return 1;
}

private void printEmotes(string *emotes) {
  int i, numFullRows, numExtras, index;
  int newLine = 0, rowCount = 0, columnCount = 0;
  int *indexesPrinted = ({});
  int tmp = 1, needToAdd;
  string *allEmotes = SOUL_D->query_emotes();

  numFullRows = sizeof(emotes) / 4;
  numExtras = sizeof(emotes) % 4;

  if(numExtras)
    needToAdd = 1;

  for(i = 0; i < sizeof(emotes); i++) {
    if(newLine >= 4) {
      tell_me("\n");
      newLine = 0;
      rowCount++;
      columnCount = 0;
    }

    if(sizeof(indexesPrinted) >= 4) {
      index = indexesPrinted[columnCount] + rowCount;

      if(index > sizeof(emotes) - 1)
        continue;

      tell(this_player(), sprintf("%-15s", emotes[index]));
    } else {
      if(i == 0) {
        index = 0;
      } else {
        if(needToAdd) {
          if(numExtras) {
            index = rowCount * columnCount +
              numFullRows * columnCount + tmp;
            numExtras--;
            tmp++;
          } else {
            index = rowCount * columnCount +
              numFullRows * columnCount + tmp;
          }
        } else {
          index = rowCount * columnCount +
            numFullRows * columnCount;
        }
      }

      tell(this_player(), sprintf("%-15s", emotes[index]));

      indexesPrinted += ({ (index) });
    }

    newLine++;
    columnCount++;
  }

  tell_me("\n\n*Cyan* untargeted only.\n");
  tell_me("*Blue* targeted only.\n\n");
}

private int alphabetize(string arg1, string arg2) {
  return strcmp(arg1, arg2);
}

private void fixArray(string *arr) {
  int i;

  for(i = 0; i < sizeof(arr); i++) {
    if(arr[i][<2..<1] == "/t")
      arr[i] = arr[i][0..<3];

    if(member_array(arr[i], singledEmotes) == -1)
      singledEmotes += ({ arr[i] });
  }

  singledEmotes =
    sort_array(singledEmotes, "alphabetize");
}

string query_help(object _caller) {
  return
" SYNTAX: emotes\n\n"
"This command allows you to see all of the emotes "
"available for you\nto use.\n\n"
"See also: emote\n";
}
