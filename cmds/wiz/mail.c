/*

 mail.c

 Parthenon@LPuniversity
 July 24th, 2006
 mail command

 Last edited on July 24th, 2006 by Parthenon

*/

inherit STD_CMD;

mixed main(object caller, string arg) {
  object mail_client;

  mail_client = new(OBJ_MAIL_CLIENT);

  if(!mail_client) {
    return notify_fail("Error [mail]: could not find mail client\n");
  }

  mail_client->move(this_body());
  mail_client->start_mail(arg);

  return 1;
}

string help(object caller) {
  return " SYNTAX: mail [<target1>, <target2>, ..., <targetN>]\n\n" +
    "This command will start a mail session for you. If you wish to go\n"+
    "straight to mailing a specific person then you may use the optional\n"+
    "target argument to begin a mail to that person. If you wish to mail\n"+
    "multiple users at once, then seperate each name by a comma. You can\n"+
    "also mail to groups if you surround the group name with paranthesis.\n";
}
