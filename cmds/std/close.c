/**
 * @file /cmds/std/close.c
 *
 * Command to close a container or door.
 *
 * @created 2024-08-08 - Gesslar
 * @last_modified 2024-08-08 - Gesslar
 *
 * @history
 * 2024-08-08 - Gesslar - Created
 */

inherit STD_ACT;

mixed main(object tp, string args) {
  string target;
  object source;
  object room;
  int here_flag;
  object ob;
  string *doors;

  if(!args)
    return "Close what?";

  room = environment(tp);
  if(sizeof(doors = room->id_door(args))) {
    object other_room;
    string door;

    if(sizeof(doors) > 1)
      return "There are multiple doors with that name.";

    door = doors[0];

    if(room->query_door_locked(door))
      return "It's locked.";

    if(room->query_door_open(door))
      room->set_door_open(door, false, true);
    else
      return sprintf("The way leading %s is already closed.", room->query_door_name(door));

    tp->simple_action("$N $vclose the $o.\n", room->query_door_name(door));

    other_room = room->query_exit_dest(door);
    if(other_room) {
      foreach(string dir in other_room->query_exit_ids()) {
        if(other_room->query_exit_dest(dir) == room) {
          other_room->set_door_open(dir, false, false);
          break;
        }
      }
    }
    return 1;
  }

  if(sscanf(args, "%s here", target) == 1)
    here_flag = 1;
  else
    target = args;

  if(here_flag) {
    if(!ob = find_target(target, room))
      return "You do not see that here.";
  } else {
    if(!ob = find_target(tp, target, tp))
      if(!ob = find_target(tp, target))
        return "You do not see " + target + " anywhere.";
  }

  if(!ob->closeable())
    return "You cannot close that.";

  if(ob->is_closed())
    return "It is already closed.";

  ob->set_closed(1);
  tp->simple_action("$N $vclose $o.\n", ob);

  return 1;
}
