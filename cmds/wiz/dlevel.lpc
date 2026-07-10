/**
 * @file /cmds/wiz/dlevel.c
 * Manages debug levels
 *
 * @created 2024-07-20 - Gesslar
 * @last_modified 2024-07-20 - Gesslar
 *
 * @history
 * 2024-07-20 - Gesslar - Created
 */

inherit STD_CMD;
inherit EXT_UNICODE;

varargs mixed main(object tp, string str) {
    mapping levels = debug_levels();
    string *names = sort_array(keys(levels), 1);

    if(!str) {
        foreach(string name in names)
            tell(tp, sprintf("%s - %s\n", on_off_widget(levels[name] != 0), name));

        return 1;
    }

    if(!of(str, levels))
        return _error("Invalid debug level: %s", str);

    _info("Changing debug level for %s", str);

    if(levels[str] == 0)
        set_debug_level(str);
    else
        clear_debug_level(str);

    return main(tp);
}
