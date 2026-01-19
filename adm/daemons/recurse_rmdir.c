/**
 * @file /adm/daemons/recurse_rmdir.c
 * @description This daemon is used to recursively remove directories.
 *              BE VERY CAREFUL WITH THIS DAEMON. IT WILL IRRETRIEVABLY DELETE
 *              EVERYTHING IN THE DIRECTORY YOU SPECIFY.
 *
 * @created 2024-07-28 - Gesslar
 * @last_modified 2024-07-28 - Gesslar
 *
 * @history
 * 2024-07-28 - Gesslar - Created
 */

#include <origin.h>

inherit STD_DAEMON;

public nomask void recurse_rmdir(string dir);
private nomask void handle_delete(string contents);

private nosave nomask string *allowed = ({});

nomask void setup() {
    allowed = mudConfig("ALLOW_RECURSE_RMDIR");
}

public nomask void recurse_rmdir(string dir) {
    string *contents;
    object prev = previous_object();

    if(!stringp(dir) || dir == "")
        return;

    if(origin() != ORIGIN_LOCAL)
        if(!of(base_name(prev), allowed))
            return;

    if(!directory_exists(dir))
        return;

    if(!of(base_name(prev), allowed))
        return;

    reset_eval_cost();

    dir = append(dir, "/");

    log_file("system/recurse_rmdir", sprintf("%s: %s\n", ctime(), dir));

    contents = get_dir(dir);
    contents -= ({ ".", ".." });
    contents = map(contents, (: $(dir) + $1 :));

    while(sizeof(contents) > 0) {
        string file = contents[0];
        contents = contents[1..];
        handle_delete(file);
    }

    log_file("system/recurse_rmdir", sprintf("Removed directory: %s\n", dir));
    rmdir(dir);
}

private nomask void handle_delete(string file) {
    int is_dir = directory_exists(file);

    if(is_dir)
        recurse_rmdir(file);
    else {
        log_file("system/recurse_rmdir", sprintf("Removed file: %s\n", file));
        rm(file);
    }
}
