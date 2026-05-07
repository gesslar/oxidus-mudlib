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

private string *singled_emotes = ({});

private void fix_array(string *arr);
private void print_emotes(string *emotes);

mixed main(object _tp, string _arg) {
  string *emotes;

  emotes = ({});
  emotes = SOUL_D->query_emotes();

  if(!emotes)
    return "Error [emotes]: There are no emotes "
      "available\n";

  singled_emotes = ({});
  fix_array(emotes);

  tell_me("\nAvailable emotes:\n");

  print_emotes(singled_emotes);

  return 1;
}

private void print_emotes(string *emotes) {
  int i, num_full_rows, num_extras, index;
  int new_line = 0, row_count = 0, column_count = 0;
  int *indexes_printed = ({});
  int tmp = 1, need_to_add;
  string *all_emotes = SOUL_D->query_emotes();

  num_full_rows = sizeof(emotes) / 4;
  num_extras = sizeof(emotes) % 4;

  if(num_extras)
    need_to_add = 1;

  for(i = 0; i < sizeof(emotes); i++) {
    if(new_line >= 4) {
      tell_me("\n");
      new_line = 0;
      row_count++;
      column_count = 0;
    }

    if(sizeof(indexes_printed) >= 4) {
      index = indexes_printed[column_count] + row_count;

      if(index > sizeof(emotes) - 1)
        continue;

      tell(this_player(), sprintf("%-15s", emotes[index]));
    } else {
      if(i == 0) {
        index = 0;
      } else {
        if(need_to_add) {
          if(num_extras) {
            index = row_count * column_count +
              num_full_rows * column_count + tmp;
            num_extras--;
            tmp++;
          } else {
            index = row_count * column_count +
              num_full_rows * column_count + tmp;
          }
        } else {
          index = row_count * column_count +
            num_full_rows * column_count;
        }
      }

      tell(this_player(), sprintf("%-15s", emotes[index]));

      indexes_printed += ({ (index) });
    }

    new_line++;
    column_count++;
  }

  tell_me("\n\n*Cyan* untargeted only.\n");
  tell_me("*Blue* targeted only.\n\n");
}

private int alphabetize(string arg1, string arg2) {
  return strcmp(arg1, arg2);
}

private void fix_array(string *arr) {
  int i;

  for(i = 0; i < sizeof(arr); i++) {
    if(arr[i][<2..<1] == "/t")
      arr[i] = arr[i][0..<3];

    if(member_array(arr[i], singled_emotes) == -1)
      singled_emotes += ({ arr[i] });
  }

  singled_emotes =
    sort_array(singled_emotes, "alphabetize");
}

string query_help(object _caller) {
  return
" SYNTAX: emotes\n\n"
"This command allows you to see all of the emotes "
"available for you\nto use.\n\n"
"See also: emote\n";
}
