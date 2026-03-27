/**
 * @file /cmds/std/put.c
 *
 * Command to put an object into a container.
 *
 * @created 2024-08-07 - Gesslar
 * @last_modified 2024-08-07 - Gesslar
 *
 * @history
 * 2024-08-07 - Gesslar - Created
 */

inherit STD_ACT;

void setup() {
  usage_text =
"put <object> in <container>\n"
"put all <object> in <container>\n"
"put <object> in <container> here\n"
"put all <object> in <container> here\n";
  help_text =
"This command allows you to put an object into a container. You can specify a "
"specific object to put into a container, or you can put all objects of a "
"specific type into a container. You can also specify whether the container "
"is in your inventory or in the room you are in.\n\n"
"See also: get, drop\n";
}

mixed main(object tp, string str) {
  string dest, target;
  object env, ob, container;
  int all_flag = 0;
  int here_flag = 0;

  if(!str)
    return "Put what where?";

  if(sscanf(str, "all %s in %s here", target, dest) == 2) {
    all_flag = 1;
    here_flag = 1;
  } else if(sscanf(str, "all %s in %s", target, dest) == 2) {
    all_flag = 1;
  } else if(sscanf(str, "%s in %s here", target, dest) == 2) {
    here_flag = 1;
  } else if(sscanf(str, "%s in %s", target, dest) != 2) {
    return "Put what where?";
  }

  env = environment(tp);

  // Find the container
  if(here_flag) {
    container = find_target(tp, dest, env);
  } else {
    container = find_target(tp, dest, tp);
    if(!container)
      container = find_target(tp, dest, env);
  }

  if(!container)
    return "There is no container named '" + dest + "' here.";

  if(!container->is_container())
    return get_short(container) + " is not a container.";

  if(container->is_closed())
    return get_short(container) + " is closed.";

  // Handle 'put all' scenarios
  if(all_flag) {
    object *obs = find_targets(tp, target, tp, (: !$1->is_container() :));
    int put_count = 0;

    if(sizeof(obs) < 1)
      return "You do not have any " + target + ".";

    foreach(object item in obs) {
      if(item == container)
        continue;

      if(item->prevent_drop() || item->prevent_put() || container->prevent_put())
        continue;

      if(!container->can_hold_object(item))
        continue;

      if(item->move(container)) {
        tell(tp, "You could not put " + get_short(item) + " in " + get_short(container) + ".\n");
        continue;
      }

      tp->simple_action("$N $vput a $o into a $o1.", item, container);
      put_count++;
    }

    if(put_count == 0)
      return "You could not put any " + target + " in " + get_short(container) + ".";

  } else {
    // Handle 'put <object>' scenarios
    if(!(ob = find_target(tp, target, tp)))
      return "You do not have " + add_article(target) + ".";

    if(ob == container)
      return "You cannot put " + get_short(ob) + " in itself.";

    if(ob->is_container())
      return "You cannot put a container inside another container.";

    if(ob->prevent_drop() || ob->prevent_put() || container->prevent_put())
      return "You cannot put " + get_short(ob) + " in " + get_short(container) + ".";

    if(!container->can_hold_object(ob))
      return get_short(container) + " cannot hold " + get_short(ob) + ".";

    if(ob->move(container))
      return "You could not put " + get_short(ob) + " in " + get_short(container) + ".";

    tp->simple_action("$N $vput a $o into a $o1.", ob, container);
  }

  return 1;
}
