/**
 * @file /cmds/wiz/reset.c
 * Resets a target object
 *
 * @created 2024-07-23 - Gesslar
 * @last_modified 2024-07-23 - Gesslar
 *
 * @history
 * 2024-07-23 - Gesslar - Created
 */

mixed main(object tp, string arg) {
    object ob;

    if(!arg)
        arg = "here";

    ob = get_object(arg);

    if(!objectp(ob))
        return _error("Unable to find %s.", arg);

    call_if(ob, "reset");

    return _ok("Reset called on %s (%s).", get_short(ob), file_name(ob));
}
