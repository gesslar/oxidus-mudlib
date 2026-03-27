/**
 * @file /cmds/std/get.c
 *
 * Standard command to get objects from the environment or containers.
 *
 * @created 2005-10-27 - Ico2
 * @last_modified 2006-03-24 - Tacitus
 *
 * @history
 * 2005-10-27 - Ico2 - Created
 * 2005-10-28 - Tacitus - QC Review
 * 2006-03-24 - Tacitus - Last edited
 */

inherit STD_ACT;

void setup() {
  usage_text =
"get <object>\n"
"get all\n"
"get all <object>\n"
"get all from <container>\n"
"get all from <container> here\n"
"get all <object> from <container>\n"
"get all <object> from <container> here\n"
"get <object> from <container>\n"
"get <object> from <container> here\n";
  help_text =
"This command will allow you to get an object from your current environment. "
"The argument you provide, will be the name of the object you wish to get.\n\n"
"See also: drop, put\n";
}

mixed main(object tp, string arg) {
  object ob;
  object room = environment(tp);
  object source = room;
  string target = "";
  string container = "";
  int here_flag = 0;

  if(!arg)
    return "SYNTAX: get <object|all|all <id>> [from <container> [here]]\n";

  // Handle 'here' flag and identify container if applicable
  if(sscanf(arg, "%s here", arg))
    here_flag = 1;

  // Handle 'from <container>' argument
  if(sscanf(arg, "all from %s", container) == 1) {
    target = "all";
  } else if(sscanf(arg, "all %s from %s", target, container) == 2) {
    // nothing to do, target and container are already set
  } else if(sscanf(arg, "%s from %s", target, container) == 2) {
    // nothing to do, target and container are already set
  } else if(sscanf(arg, "all %s", target) != 1) {
    target = arg;
  }

  // Adjust the source based on the 'from' and 'here' arguments
  if(container != "") {
    if(here_flag) {
      source = find_target(tp, container, room);
      if(!source || living(source))
        return "There is no container named '" + container + "' here.\n";
    } else {
      source = find_target(tp, container, tp) || find_target(tp, container, room);
      if(!source || living(source))
        return "You do not have a container named '" + container + "' and there is none in the room.\n";
    }
  }

  if(source != room && source->is_closed())
    return get_short(source) + " is closed.\n";

  // Handle 'get all' cases
  if(target == "all" || sscanf(arg, "all %s", target)) {
    object *obs;
    int got = 0;

    obs = find_targets(tp, target != "all" ? target : 0, source, (: !living($1) && !$1->prevent_get() :));

    if(sizeof(obs) < 1)
      return "There is nothing to get.\n";

    foreach(object item in obs) {
      int result;
      int is_coin = item->is_coin();
      string short = get_short(item);

      result = item->move(tp);
      if(result != MOVE_OK) {
        tell(tp, "You were unable to pick up " + short + ".\n");
        continue;
      }

      if(source == room)
        tp->simple_action("$N $vpick up a $o.", short);
      else
        tp->simple_action("$N $vtake a $o from a $o1.", short, get_short(source));

      got = 1;
    }
    if(!got)
      return "You picked up nothing.\n";
  } else {
    string short;
    ob = find_target(tp, target, source);
    if(!ob)
      return "You see no '" + target + "' here.\n";

    short = get_short(ob);

    if(living(ob))
      return "You cannot pick up living things.\n";

    if(ob->prevent_get())
      return short + " cannot be picked up.\n";

    if(ob->move(tp))
      return "You were unable to pick up " + short + ".\n";

    if(source == room)
      tp->simple_action("$N $vpick up a $o.", short);
    else
      tp->simple_action("$N $vtake a $o from a $o1.", short, get_short(source));
  }

  return 1;
}
