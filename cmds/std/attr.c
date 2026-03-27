/**
 * @file /cmds/std/attr.c
 *
 * Attribute command.
 *
 * @created 2024-07-31 - Gesslar
 * @last_modified 2024-07-31 - Gesslar
 *
 * @history
 * 2024-07-31 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(/** @type {STD_BODY} */ object tp,
    string _str) {
    string *attrs, attr;
    string out = "";

    attrs = mudConfig("ATTRIBUTES");

    foreach(attr in attrs) {
        out += sprintf("%-15s: %d\n",
            capitalize(attr),
            tp->query_attribute(attr)
        );
    }

    return out;
}
