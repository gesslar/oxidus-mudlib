/**
 * @file /cmds/adm/master.c
 * @description Command to reload core system files.
 *
 * @created 2024-07-17 - Gesslar
 * @last_modified 2024-07-17 - Gesslar
 *
 * @history
 * 2024-07-17 - Gesslar - Created
 */

inherit STD_CMD;

private nosave nomask mixed *actions;

void setup() {
  actions = ({
    ({"Destruct simul_efun object",(: destruct(find_object(SIMUL_OB)) :)}),
    ({"Load simul_efun object",(: load_object(SIMUL_OB) :)}),
    ({"Destruct master object",(: destruct(master()) :)}),
    ({"Destruct master/valid",(: destruct(find_object("/adm/obj/master/valid")) :)}),
    ({"Load master/valid",(: load_object("/adm/obj/master/valid") :)}),
    ({"Load master object",(: load_object("/adm/obj/master") :)}),
    ({"Rehash system configuration",(: CONFIG_D->rehash_config() :)}),
  });
}

mixed main(/** @type {STD_PLAYER} */ object _caller, string _arguments) {
  string err = "";

  if(!adminp(previous_object()))
    return _error("Access denied.");

  foreach(mixed *action in actions) {
    mixed result;

    _info(action[0]);

    result = catch(action[1]());
    if(result)
      err += result;
  }

  if(sizeof(err))
    return _error("Error when reloading system objects:\n\n"
      "%s\n"
      "Refer to {{bl1}}{{ul1}}%scatch{{res}}",
      err,
      log_dir()
    );
  else
    return _ok("System objects updated successfully.");
}

string query_help(object _caller) {
  return
" SYNTAX: master\n\n"
"This command reloads core system files. No argument is taken.\n";
}
