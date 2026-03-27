/**
 * @file /cmds/std/tell.c
 *
 * Command to send a message to another player.
 *
 * @created 2005-06-30 - Gwegster @ LPUniversity
 * @last_modified 2006-10-06 - Tricky
 *
 * @history
 * 2005-06-30 - Gwegster - Created
 * 2006-10-06 - Tricky - Last edited
 */

inherit STD_CMD;

void setup() {
  usage_text = "tell <player> [:]message";
  help_text =
"This command will send a message to the specified "
"player, if they are online.\n\n"
"You may also emote to another player using the "
"tell <player> :<message> syntax.\n\n";
}

mixed main(object tp, string str) {
  string who, message;
  object target;
  string name, tname;

  if(!str)
    return _usage(tp);

  if(sscanf(str, "%s %s", who, message) != 2)
    return _usage(tp);

  if(!message)
    return _usage(tp);

  target = find_living(lower_case(who));

  if(!objectp(target))
    return "You cannot seem to find " +
      capitalize(who) + ".\n";

  if(target == tp) {
    tp->simple_action("$N $vstart talking to $r.");
    return 1;
  }

  name = tp->query_name();
  tname = target->query_name();

  if(message[0] == ':') {
    tell(tp, sprintf("From afar, you %s\n",
      message[1..]));
    tell(target, sprintf("From afar, %s %s\n",
      tname, message[1..]));
  } else {
    tell(tp, sprintf("You tell %s: %s\n",
      tname, message));
    tell(target, sprintf("%s tells you: %s\n",
      name, message));
  }

  target->set_tell_reply(name);

  return 1;
}
