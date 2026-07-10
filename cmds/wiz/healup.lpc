/**
 * @file /cmds/wiz/healup.c
 * Command for wizards to heal themselves and others.
 *
 * @created 2024-07-31 - Gesslar
 * @last_modified 2024-07-31 - Gesslar
 *
 * @history
 * 2024-07-31 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(object tp, string str) {
    object ob;

    if(!str)
        ob = tp;

    if(!ob)
        ob = get_object(str);

    if(!ob)
        return "No such living object found.\n";

    if(!living(ob))
        return "That object is not living.\n";

    ob->restore();

    if(ob == tp)
        return "You've restored yourself.\n";

    return "You've restored " + ob->query_name() + ".\n";
}
