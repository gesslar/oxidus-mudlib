/**
 * @file /cmds/std/drop.c
 *
 * Standard command to drop objects from inventory.
 *
 * @created 2005-10-27 - Ico2
 * @last_modified 2006-06-27 - Tacitus
 *
 * @history
 * 2005-10-27 - Ico2 - Created
 * 2005-10-28 - Tacitus - QC Review
 * 2006-06-27 - Tacitus - Last edited
 */

inherit STD_ACT;

void setup() {
  usage_text =
"drop <object>\n"
"drop all\n"
"drop all <object>\n";
  help_text =
"This command will allow you to drop an object you are currently holding onto "
"the ground. The argument you provide, will be the name of the object you "
"wish to drop.\n\n"
"See also: get, put\n";
}

mixed main(object tp, string arg) {
  object room = environment(tp);

  if(!arg)
    return "Drop what?";

  if(arg == "all") {
    object *inv = find_targets(tp, 0, tp);

    if(!sizeof(inv))
      return "You don't have anything in your inventory.\n";

    foreach(object item in inv) {
      if(item->prevent_drop())
        tp->my_action("$N $vcannot drop $p $o.", item);
      else if(item->move(room))
        tp->my_action("$N could not drop $p $o.", item);
      else
        tp->my_action("$N $vdrop $p $o.", item);
    }
  } else if(sscanf(arg, "all %s", arg)) {
    object *inv = find_targets(tp, arg, tp);

    if(!sizeof(inv))
      return "You don't have any '" + arg + "' in your inventory.\n";

    foreach(object item in inv) {
      if(item->prevent_drop())
        tp->my_action("$N $vcannot drop $p $o.", item);
      else if(item->move(room))
        tp->simple_action("$N could not drop $p $o.", item);
      else
        tp->simple_action("$N $vdrop $p $o.", item);
    }
  } else {
    object ob = find_target(tp, arg, tp);
    string name;

    if(!ob)
      return "You don't have a '" + arg + "' in your inventory.\n";

    name = ob->query_real_name();

    if(ob->prevent_drop())
      return "You cannot drop " + name + ".\n";

    if(ob->move(room))
      return "You could not drop " + name + ".\n";

    tp->simple_action("$N $vdrop $p $o.", ob);
  }

  return 1;
}
