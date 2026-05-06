/**
 * @file /cmds/wiz/sc.c
 * Deep scan
 *
 * @created 2024-07-28 - Gesslar
 * @last_modified 2024-07-28 - Gesslar
 *
 * @history
 * 2024-07-28 - Gesslar - Created
 */

inherit STD_CMD;

string deep_scan(object tp, object *inv, int indent);

mixed main(object tp, string arg) {
  object ob, *inv;
  string str;

  if(!arg)
    arg = "here";

  ob = get_object(arg);
  if(!ob)
    return "No such object.";

  inv = all_inventory(ob);

  str = "Deep scanning "+ob+"\n";
  str += deep_scan(tp, inv, 2);

  return str;
}

string deep_scan(object tp, object *inv, int indent) {
  object ob;
  string str;

  str = "";

  foreach(ob in inv) {
    object *new_inv;

    str += repeat_string(" ", indent) + ob + " (" + get_short(ob) + ")\n";
    new_inv = all_inventory(ob);
    if(sizeof(new_inv))
      str += deep_scan(tp, new_inv, indent + 2);
  }

  return str;
}
