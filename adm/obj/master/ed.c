/**
 * @file /adm/obj/master/ed.c
 *
 * Ed-related master object functionality
 *
 * @created 2026-06-12 - Gesslar
 * @last_modified 2026-06-12 - Gesslar
 *
 * @history
 * 2026-06-12 - Gesslar - Created
 */

// -- driver apply
// This master apply is called by the ed() efun to resolves relative path
// names of a file to read/write, to an absolute path name.
string make_path_absolute(string file) {
  file = resolve_path(this_body()->query_env("cwd"), file);

  return file;
}

// -- driver apply
private int save_ed_setup(/** @type {STD_PLAYER} */ object user, int config) {
  user->set_ed_setup(config);

  return 1;
}

// -- driver apply
private int retrieve_ed_setup(/** @type {STD_PLAYER} */ object user) {
  return user->query_ed_setup();
}

// This doesn't actually seem to work and generates *Too long evaluation.
// Execution aborted. errors even though it isn't that complicated.
#if 0
public string get_save_file_name(string file, object who) {
    string temp, e;

    debug_message("Called from previous_object(): " + previous_object());

    e = catch {
        temp = sprintf("/tmp/%s.%d",
            who ? query_privs(who) : "unknown",
            time()
        );
    };

    if(e) {
        debug_message(sprintf("get_save_file_name error: %O", e));
        return null;
    }

    debug_message(sprintf("get_save_file_name new file: %s", temp));

    return temp;
}
#endif
