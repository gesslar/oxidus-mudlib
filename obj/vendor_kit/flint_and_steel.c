inherit OBJ_VENDOR_KIT;

void setup() {
  set_id(({"kit","flint and steel kit","fire kit"}));
  set_adj(({"small","leather","flint","steel"}));
  set_name("flint and steel kit");
  set_short("a small flint and steel kit");
  set_long("A modest leather pouch holding a chunk of dark flint and a "
           "C-shaped firesteel - the makings of any traveller's campfire. "
           "Strike one against the other over dry tinder and a spark will "
           "do the rest."
  );
  set_mass(20);
  set_value(15);

  __contents = ({
    "/obj/loot/flint.loot",
    "/obj/loot/firesteel.loot",
  });
}
