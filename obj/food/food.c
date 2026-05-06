/**
 * @file /obj/food/food.c
 * Base food inheritable for virtual food items
 *
 * @created 2024-08-21 - Gesslar
 * @last_modified 2024-08-22 - Gesslar
 *
 * @history
 * 2024-08-21 - Gesslar - Created
 * 2024-08-22 - Gesslar - Updated to set base ID and add adjectives separately
 */

inherit STD_FOOD;

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
  if(!nullp(data["uses"]))
    set_uses(data["uses"]);

  if(pointerp(data["additional_ids"])) {
    foreach(string id in data["additional_ids"]) {
      add_id(id);
    }
  }

    // Properties handling commented out for now
    // if(!nullp(data["properties"]) && mapp(data["properties"])) {
    //     foreach(string key, mixed value in data["properties"]) {
    //         set_property(key, value);
    //     }
    // }
}

// Additional food-specific functions can be added here
