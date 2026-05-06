/**
 * @file /cmds/wiz/back.c
 * Return to your previous location.
 *
 * @created 2024-09-09 - Gesslar
 * @last_modified 2024-09-09 - Gesslar
 *
 * @history
 * 2024-09-09 - Gesslar - Created
 */

mixed main(object tp, string str) {
  string loc;
  object room;
  string e;
  int result;

  loc = tp->query_last_location();

  if(!loc)
    return "You have no previous location to return to.";

  e = catch(room = load_object(loc));
  if(e)
    return "There was a problem loading your previous location.";

  result = tp->move_living(room);
  printf("Result = %d\n", result);
  if(result)
    return "You were unable to move to your previous location.\n"
            "Reason: " + MOVE_REASON[result];

  return "You return to " + room->query_short() + ".";
}
