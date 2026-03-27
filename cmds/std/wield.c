/**
 * @file /cmds/std/wield.c
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

mixed main(/** @type {STD_BODY} */ object tp,
    string str) {
    object ob;
    mixed result;

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
