/**
 * @file /cmds/std/eq.c
 *
 * List all your equipped items.
 *
 * @created 2024-07-27 - Gesslar
 * @last_modified 2024-07-27 - Gesslar
 *
 * @history
 * 2024-07-27 - Gesslar - Created
 */

inherit STD_ACT;

mixed main(/** @type {STD_BODY} */ object tp,
    string str) {
  mapping eq, wielded;
  string slot, *slots;
  string out = "";
  int sz, max;

  eq = tp->query_equipped();
  wielded = tp->query_wielded();

  if(!sizeof(eq) && !sizeof(wielded))
    return "You are not equipping anything.";

  out += "You have equipped the following items:\n\n";

  slots = keys(wielded);
  slots += keys(eq);

  max = max(map(slots, (: strlen :)));

  foreach(slot in slots) {
    object item = wielded[slot] || eq[slot];
    out += sprintf("%*s : %s\n", max, capitalize(slot), get_short(item));
  }

  return out;
}
