/**
 * @file /std/user/visibility.c
 * Object for determining whether a living can see things
 *
 * @created 2024-07-23 - Gesslar
 * @last_modified 2024-07-23 - Gesslar
 *
 * @history
 * 2024-07-23 - Gesslar - Created
 */

int can_see(object target) {
    if(!objectp(target))
        return 0;

    if(target->query_invis())
        return 0;

    if(target->query_hidden())
        return 0;

    return 1;
}
