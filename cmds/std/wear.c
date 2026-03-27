/**
 * @file /cmds/std/wear.c
 *
 * Wear command.
 *
 * @created 2024-07-26 - Gesslar
 * @last_modified 2024-07-26 - Gesslar
 *
 * @history
 * 2024-07-26 - Gesslar - Created
 */

inherit STD_ACT;

void setup() {
    help_text =
"This command enables you to wear an item of clothing or armour. You must "
"possess the item in your inventory to wear it. The item will have specific "
"slots that it can be worn in. For example, a pair of boots might have a slot "
"of 'feet'. To wear the boots, you would type 'wear boots'. You may not wear "
"more than one item in the same slot.";
   usage_text = "wear <item>";
}

mixed main(/** @type {STD_BODY} */ object tp,
    string str) {
    object ob;
    string slot;
    string slots;
    string *items;
    mixed result;
    int i;

    if(!ob = find_target(tp, str, tp))
        return "You do not have that item.";

    if(!ob->is_armour() && !ob->is_clothing())
        return "You can only wear clothing and armour.";

    slot = ob->query_slot();
    if(nullp(slot))
        return "That item cannot be worn.";

    if(!tp->module("race", "query_equipment_slots", slot))
        return "You cannot wear something of that type.";

    if(tp->equipped_on(slot))
        return "You are already wearing something in that slot.";

    result = ob->can_equip(slot, tp);
    if(stringp(result))
        return result;
    if(result == 0)
        return "1 You cannot wear that item.";

    result = ob->equip(tp, slot);
    if(stringp(result))
        return result;
    if(result == 0)
        return "2 You cannot wear that item.";

    return "You wear the "+get_short(ob)+".";
}
