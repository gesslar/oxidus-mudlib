/**
 * @file /cmds/ghost/go.c
 * @description Command for moving around in the game as a ghost.
 *
 * @created 2024-02-03 - Gesslar
 * @last_modified 2024-02-03 - Gesslar
 *
 * @history
 * 2024-02-03 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(/** @type {STD_BODY} */ object tp, string arg) {
  mixed dest;
  mixed result;
  /** @type {STD_ROOM} */ object room = environment(tp);

  result = MOVE_D->allow_walk_direction(tp, room, arg);
  if(result != 1) {
    if(stringp(result))
      return result;
    else
      return "You may not move in that direction.";
  }

  dest = room->query_exit_dest(arg);
  result = MOVE_D->prevent_walk_direction(tp, room, dest);
  if(result != 0) {
    if(stringp(result))
      return result;
    else
      return "You may not move in that direction.";
  }

  if(tp->move_living(dest, arg))
    return "You are not allowed to go there.";

  MOVE_D->pre_walk_direction(tp, room, arg);
  MOVE_D->post_walk_direction(tp, dest, arg);

  return 1;
}
