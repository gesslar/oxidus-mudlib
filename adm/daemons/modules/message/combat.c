/**
 * @file /adm/daemons/modules/message/combat.c
 * @description Combat messaging module for the message daemon
 *
 * @created 2024-07-28 - Gesslar
 * @last_modified 2024-07-28 - Gesslar
 *
 * @history
 * 2024-07-28 - Gesslar - Created
 */

inherit STD_DAEMON;

private nosave mapping messages;
private nosave mapping evals;

void setup() {
  set_no_clean(1);

  messages = fromString(read_file("/adm/etc/message/combat.txt"));
  evals = allocate_mapping(keys(messages), (: keys($(messages)[$1]) :));
}

string get_message(string type, int damage) {
  mixed *options = evals[type];
  mixed mess;

  if(!options)
    return 0;

  options = filter(options, (: evaluate_number($(damage), $1) :));

  if(sizeof(options) != 1)
    return 0;

  mess = messages[type][options[0]];
  if(stringp(mess))
    return mess;

  if(pointerp(mess))
    return element_of(mess);

    return 0;
}
