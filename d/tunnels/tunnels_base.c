/**
 * @file /d/village/tunnels/tunnels_base.c
 * @description This is the inheritable for the virtual tunnels rooms.
 *
 * @created 2024-08-25 - Gesslar
 * @last_modified 2024-08-25 - Gesslar
 *
 * @history
 * 2024-08-25 - Gesslar - Created
 */

inherit STD_ROOM;

void repopulate();

/** @type {STD_NPC*} */
private nosave object *mobs = ({});
private nosave string *mob_files = ({});
private nosave float spawn_chance = 8.0;

void setup() {
  set_light(0);
  set_terrain("tunnels");
}

void virtual_setup(mixed args...) {
  string file = args[0];

  set_zone("twisting_tunnels");

  __DIR__ "tunnels_daemon"->setup_exits(this_object(), file);
  __DIR__ "tunnels_daemon"->setup_short(this_object(), file);
  __DIR__ "tunnels_daemon"->setup_long(this_object(), file);

  add_reset((: repopulate :));

  mob_files = ({
    "/mob/rat",
    "/mob/bat",
    "/mob/mole",
    "/mob/centipede",
  });
}

void repopulate() {
  string file;

  mobs -= ({ 0 });

  foreach(object mob in mobs) {
    if(objectp(mob)) {
      mob->simple_action("$N $vscurry away into the darkness.");
      mob->remove();
    }
  }

  if(random_float(100.0) < spawn_chance) {
    file = element_of(mob_files);
    mobs += ({ add_inventory(file) });
    mobs->simple_action("$N $vappear from the shadows.");
  }
}
