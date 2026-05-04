/**
 * @file /d/village/field/field_base.c
 * @description This is the inheritable for the virtual field rooms.
 *
 * @created 2024-02-04 - Gesslar
 * @last_modified 2024-02-04 - Gesslar
 *
 * @history
 * 2024-02-04 - Gesslar - Created
 */

inherit STD_ROOM;

void repopulate();

private nosave string *mob_files = ({});
/** @type {STD_NPC} */
private nosave object *mobs = ({});
private nosave float spawn_chance = 15.0;

void virtual_setup(mixed args...) {
  string file = args[0];

  set_zone("lush_field");

  __DIR__ "field_daemon"->setup_exits(this_object());
  __DIR__ "field_daemon"->setup_short(this_object(), file);
  __DIR__ "field_daemon"->setup_long(this_object(), file);

  add_reset((: repopulate :));

  set_terrain("grass");

  mob_files = ({
    "/mob/wild_boar",
    "/mob/field_mouse",
    "/mob/grasshopper",
    "/mob/field_rabbit",
  });
}

void repopulate() {
  /** @type {STD_NPC} */ object mob;
  string file;

  mobs = filter(mobs, (: nullp :));

  foreach(mob in mobs) {
    if(objectp(mob)) {
      mob->simple_action("$N $vwander away.");
      mob->remove();
    }
  }

  if(random_float(100.0) < spawn_chance) {
    file = element_of(mob_files);
    /** @lpc-ignore - I have no {@see https://github.com/jlchmura/lpc-language-server/issues/317} */
    mobs += ({ /** @type {STD_NPC} */ (add_inventory(file)) });
    mobs->simple_action("$N $varrive.");
  }
}
