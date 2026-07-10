/**
 * @file /cmds/wiz/unload.c
 * This command will unload a master object
 *
 * @created 2024-08-13 - Gesslar
 * @last_modified 2024-08-13 - Gesslar
 *
 * @history
 * 2024-08-13 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(object tp, string arg) {
    string file, *files;
    object ob;
    int unloaded = 0;

    if(!arg)
        return _info("Usage: unload <file>");

    files = get_files(tp->query_env("cwd"), arg);
    if(!sizeof(files))
        return _info("No files found matching: %s", arg);

    foreach(file in files) {
        ob = find_object(file);

        if(!ob)
            continue;

        if(sizeof(present_players(ob))) {
            _info("Skipping %s.", file);
            continue;
        }

        catch(ob->remove());
        if(ob)
            destruct(ob);
        unloaded++;

        _ok("Unloaded %s.", file);
    }

    return _ok("%d files unloaded.", unloaded);
}
