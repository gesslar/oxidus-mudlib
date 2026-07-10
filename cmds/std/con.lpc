/**
 * @file /cmds/std/con.c
 *
 * Check the condition of a living, including yourself.
 *
 * @created 2024-07-28 - Gesslar
 * @last_modified 2024-07-28 - Gesslar
 *
 * @history
 * 2024-07-28 - Gesslar - Created
 */

inherit STD_CMD;

void setup() {
    help_text =
"This will show you the current condition of the target. Without a target, "
"it will show your own condition.\n"
"\n"
"When comparing yourself to a non-player target, it will also give you a "
"rough idea of how you compare in strength.\n";

    usage_text = "con [target]";
}

mixed main(/** @type {STD_BODY} */ object tp,
    string str) {
    object ob;
    string name;
    string target;
    string result = "";

    if(!str) {
        ob = tp;
    } else {
        object room = environment(tp);
        if(!ob = find_target(tp, str))
            return "You don't see that here.\n";
    }

    if(!living(ob))
        return ob->query_name() + " is " + ob->query_name();

    name = ob->query_name();
    if(ob == tp) {
        string *conditions = ob->query_condition_string();
        result += "You are " + conditions[0] + ".\n";
        result += "Your mind feels " + conditions[1] + ".\n";
        result += "You are feeling " + conditions[2] + ".\n";
    } else {
        if(ob->is_npc()) {
            float lvl = tp->query_level();
            float ob_lvl = ob->query_level();
            float diff = lvl - ob_lvl;

            if(diff > 5.0) {
                result += name + " is much weaker than you.\n";
            } else if(diff > 0.0) {
                result += name + " is weaker than you.\n";
            } else if(diff < -5.0) {
                result += name + " is much stronger than you.\n";
            } else if(diff < 0.0) {
                result += name + " is stronger than you.\n";
            } else {
                result += name + " is about as strong as you.\n";
            }
        }

        result += name + " is " + ob->query_condition_string()[0] + ".\n";
    }

    return result;
}
