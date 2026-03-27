/**
 * @file /cmds/std/time.c
 *
 * Command to check the current game time.
 *
 * @created 2024-08-05 - Gesslar
 * @last_modified 2024-08-05 - Gesslar
 *
 * @history
 * 2024-08-05 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(object tp, string str) {
    return sprintf("It is now %s.\n", TIME_D->query_full_time());
}
