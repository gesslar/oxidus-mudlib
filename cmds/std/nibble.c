/**
 * @file /cmds/std/nibble.c
 *
 * Nibble command.
 *
 * @created 2024-08-07 - Gesslar
 * @last_modified 2024-08-07 - Gesslar
 *
 * @history
 * 2024-08-07 - Gesslar - Created
 */

inherit STD_ACT;

mixed main(object tp, string str) {
    object ob;
    int uses;

    if(!ob = find_target(tp, str, tp))
        return "You don't have that.";

    if(!ob->is_food())
        return "You can't nibble on that.";

    uses = ob->query_uses();
    if(uses < 1)
        return "There is nothing left to nibble on.";

    if(!ob->nibble(tp))
        return "You couldn't nibble on that.";

    return 1;
}
