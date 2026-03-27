/**
 * @file /cmds/std/remove.c
 *
 * Command to remove worn items.
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
"Use this command to remove an item of clothing or armour that you are wearing. "
"For example, to remove a pair of boots, you would type 'remove boots'. You "
"may only remove items that you are wearing.";
    usage_text = "remove <item>";
}

mixed main(/** @type {STD_BODY} */ object tp,
    string str) {
    object ob;
    string slot;
    string *slots;
    string *items;
    int i;
    mixed result;

    if(!ob = find_target(tp, str, tp))
        return "You do not have that item.";

    slot = ob->query_slot();
    if(nullp(slot))
        return "That item cannot be removed.";

    if(tp->equipped_on(slot) != ob)
        return "You are not wearing that item.";

    result = ob->can_unequip(tp);
    if(stringp(result))
        return result;
    if(result == 0)
        return "You cannot remove that item.";

    result = ob->unequip();
    if(stringp(result))
        return result;
    if(result == 0)
        return "You cannot remove that item.";

    return 1;
}
