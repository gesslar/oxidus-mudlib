/**
 * @file /cmds/std/reply.c
 *
 * Command to reply to the last person who sent you a tell.
 *
 * @created 2006-05-05 - Tacitus @ LPUniversity
 * @last_modified 2006-09-18 - Tricky
 *
 * @history
 * 2006-05-05 - Tacitus - Created
 * 2006-08-11 - Parthenon - Last edited
 * 2006-09-10 - Tricky - Last edited
 * 2006-09-18 - Tricky - Last edited
 */

inherit STD_CMD;

void setup() {
  usage_text = "reply <message>";
  help_text =
"This command will send a tell to the last person who "
"sent you a tell. If they are online, they will receive "
"the message.";
}

mixed main(object tp, string message) {
  object target;
  string who = tp->query_tell_reply();
  string name, tname;

  if(!who || !strlen(who))
    return "Nobody to reply to.";

  if(!message)
    return "Reply with what?";

  target = find_player(who);

  if(!objectp(target)) {
    tp->set_tell_reply("");
    return "You cannot seem to find " +
      capitalize(who) + ".";
  }

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

  target->set_tell_reply(query_privs(tp));

  return 1;
}
