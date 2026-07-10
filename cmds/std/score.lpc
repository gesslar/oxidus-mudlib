/**
 * @file /cmds/std/score.c
 *
 * Score command.
 *
 * @created 2024-08-06 - Gesslar
 * @last_modified 2024-08-06 - Gesslar
 *
 * @history
 * 2024-08-06 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(/** @type {STD_BODY} */ object tp,
    string _str) {
    string result;

    result = sprintf("You are %s, a level %d %s.\n",
        capitalize(tp->query_real_name()),
        to_int(tp->query_level()),
        tp->query_race());

    result += sprintf("You have %.2f hp, %.2f sp, and %.2f mp.\n",
        tp->query_hp(),
        tp->query_sp(),
        tp->query_mp());

    result += sprintf("You have %s experience points and require %s "
        "to advance.\n",
        add_commas(tp->query_xp()),
        add_commas(tp->query_tnl()));

    return result;
}
