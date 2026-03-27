/**
 * @file /cmds/std/give.c
 *
 * Command to give an item to another living.
 *
 * @created 2005-10-28 - Ico2 @ LPUniversity
 * @last_modified 2006-05-05 - Tacitus
 *
 * @history
 * 2005-10-28 - Ico2 - Created
 * 2005-10-29 - Tacitus - QC Review
 * 2006-05-05 - Tacitus - Last edited
 */

inherit STD_CMD;

void setup() {
  help_text =
"This command will allow you to give an item that you "
"are currently holding to another living in your "
"current environment.";
  usage_text = "give <item> to <target>";
}

mixed main(/** @type {STD_BODY} */ object tp, string arg) {
  string targetname;
  object room;
  string obname;
  object ob;
  object target;

  if(!arg)
    return "Give what to whom?";

  if(!sscanf(arg, "%s to %s", obname, targetname))
    return "Give what to whom?";

  room = environment(tp);
  if(!room)
    return "You are nowhere.";

  if(!target = find_target(tp, targetname))
    return "You don't see " + targetname + " here.";

  if(!living(target))
    return "You can't give anything to that.";

  ob = find_target(tp, obname, tp);
  if(!objectp(ob))
    return "You don't see " + add_article(obname) +
      " here.";

  if(ob->prevent_transfer(target, room))
    return "You can't give that to " +
      target->query_name() + ".";

  if(ob->move(target))
    return "You can't give that to " +
      target->query_name() + ".";

  tp->targetted_action("$N $vgive $p $o to $t.",
    target, get_short(ob));

  return 1;
}
