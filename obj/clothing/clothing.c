/**
 * @file /obj/clothing/clothing.c
 * Base clothing inheritable for virtual clothing items
 *
 * @created 2026-05-01 - Gesslar
 * @last_modified 2026-05-01 - Gesslar
 *
 * @history
 * 2026-05-01 - Gesslar - Created
 */

inherit STD_CLOTHING;

varargs void virtual_setup(mixed args...) {
  mapping data;

  if(!args || !mapp(args[0]))
    return;

  data = args[0];

  if(!nullp(data["id"]))
    set_id(data["id"]);
  if(!nullp(data["adj"]))
    set_adj(data["adj"]);
  if(!nullp(data["name"]))
    set_name(data["name"]);
  if(!nullp(data["short"]))
    set_short(data["short"]);
  if(!nullp(data["long"]))
    set_long(data["long"]);
  if(!nullp(data["mass"]))
    set_mass(data["mass"]);
  if(!nullp(data["value"]))
    set_value(data["value"]);
  if(!nullp(data["slot"]))
    set_slot(data["slot"]);

  if(pointerp(data["additional_ids"])) {
    foreach(string id in data["additional_ids"]) {
      add_id(id);
    }
  }
}
