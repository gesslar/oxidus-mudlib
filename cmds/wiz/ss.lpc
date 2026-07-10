/**
 * @file /cmds/wiz/ss.c
 * Shallow scan
 *
 * @created 2024-07-28 - Gesslar
 * @last_modified 2024-07-28 - Gesslar
 *
 * @history
 * 2024-07-28 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(object tp, string arg) {
  object ob, *inv;

  if(!arg)
    arg = "here";

  ob = get_object(arg);
  if(!ob)
    return "No such object.";

  inv = all_inventory(ob);

  tell(tp, "Shallow scanning "+ob+"\n");
  foreach(ob in inv)
    tell(tp, "  " + ob + " (" + get_short(ob) + ")\n");

  return 1;
}
