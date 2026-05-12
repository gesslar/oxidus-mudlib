/**
 * @file /cmds/action/wield.c
 *
 * Wield command.
 *
 * @created 2024-08-04 - Gesslar
 * @last_modified 2024-08-04 - Gesslar
 *
 * @history
 * 2024-08-04 - Gesslar - Created
 */

inherit STD_ACT;

void setup() {
  usage_text =
"wield - List the weapons you are currently wielding.\n"
"wield <weapon> - Wield a weapon in your primary hand.\n";
  help_text =
"Wield a weapon. With no argument, lists the weapons you are "
"currently wielding. Otherwise, wields the named weapon in your "
"primary hand.\n\n"
"See also: unwield, eq\n";
}

mixed main(/** @type {STD_BODY} */ object tp, string str) {
  /** @type {STD_WEAPON} */
  object ob;
  mixed result;

  if(!str) {
    mapping wielded = tp->query_wielded();
    string *slots, slot;
    string out;
    int max;

    if(!sizeof(wielded))
      return "You are not wielding anything.";

    slots = keys(wielded);
    max = max(map(slots, (: strlen :)));

    out = "You are wielding the following weapons:\n\n";
    foreach(slot in slots)
      out += sprintf("%*s : %s\n", max, capitalize(slot),
        get_short(wielded[slot]));

    return out;
  }

  if(!ob = find_target(tp, str, tp))
    return "You do not have that item.";

  if(!ob->is_weapon())
    return "You can only wield weapons.";

  if(tp->equipped(ob))
    return "You are already wielding that weapon.";

  result = tp->can_equip(ob, "right hand");

  if(stringp(result))
    return result;

  if(result == 0)
    return "1 You cannot wield that weapon.";

  result = ob->can_equip(tp);

  if(!result)
    return "2 You cannot wield that weapon.";

  result = ob->equip(tp, "right hand");

  if(stringp(result))
    return result;

  if(result == 0)
    return "3 You cannot wield that weapon.";

  tp->simple_action("$N $vwield $o.", get_short(ob));

  return 1;
}
