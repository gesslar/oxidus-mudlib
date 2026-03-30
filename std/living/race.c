/**
 * @file /std/user/race.c
 * @description Race stuff
 *
 * @created 2024-07-25 - Gesslar
 * @last_modified 2024-07-25 - Gesslar
 *
 * @history
 * 2024-07-25 - Gesslar - Created
 */

#include <race.h>
#include <module.h>

private nosave string racialBodies = DIR_STD_MODULES "race/";
private nomask nosave string _race;
private string gender;

public string set_race(string race) {
  string modulePath = "std/modules/race/" + race;
  object mod;

  if(!file_exists(racialBodies + race + ".c")) {
    _race = race;
    return _race;
  }

  if(query_module(modulePath))
    error("Race body module has already been applied");

  mod = add_module(modulePath);

  if(!objectp(mod))
    error("Failed to add race module.");

  _race = mod->query_race();

  return _race;
}

public string query_race() {
  return _race ||
    module("std/modules/race/" + _race, "query_race");
}

public nomask void set_gender(string g) {
    gender = g;
}

public nomask string query_gender() {
    return gender;
}

public nomask void adjust_living() {
}

public nomask void living_adjust_attributes() {
}

public nomask void living_adjust_vitals() {
}
