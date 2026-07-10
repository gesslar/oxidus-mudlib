/**
 * @file /cmds/std/su.c
 *
 * su into another living. This is in cmds/std/ so that it can be used by all
 * living objects, except only if the user is a developer.
 *
 * @created 2024-07-29 - Gesslar
 * @last_modified 2024-07-29 - Gesslar
 *
 * @history
 * 2024-07-29 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(object user, string arg) {
  object
  /** @type {STD_BODY} */     dest,
  /** @type {STD_BODY} */     su_body,
  /** @type {STD_ROOM} */     room,
  /** @type {ROOM_FREEZER} */ freezer
  ;

  room = environment(user);
  freezer = load_object(ROOM_FREEZER);

  su_body = user->query_su_body();
  if(!arg) {
    if(su_body)
      dest = su_body;
    else
      if(!devp(user))
        return 0;
      else
        return "You are not su'd into anyone or your body cannot be found.";
  } else {
    if(!devp(user))
      return 0;

    if(su_body)
      return "You are already su'd into "+user->query_name()+".";

    if(!dest = find_living(arg))
      if(!dest = present(arg, room))
        return "Cannot find " + arg + ".";

    if(!living(dest))
      return arg + " is not a living object.";

    if(!environment(dest))
      return "Cannot find " + arg + ".";

    if(dest == user)
      return "You cannot su into yourself.";
  }

  if(interactive(dest))
    return "That body is already in use.";

  if(exec(dest, user)) {
    if(su_body)
      user->clear_su_body();
    else
      dest->set_su_body(user);

    if(su_body == dest) {
      dest->move(room);
      tell(dest, "You return to your body.\n");
      dest->other_action("$N $vexit the body of $o.", user);
    } else {
      tell(dest, "You possess " + dest->query_name() + ".\n");
      dest->other_action("$O $vpossess $n.", user);
      user->move(freezer);
    }
  } else {
    tell(user, "Failed to su into " + dest->query_name() + ".\n");
  }

  return 1;
}
