/**
 * @file /std/room/zone.c
 * @description Room zone module
 *
 * @created 2024/02/04 - Gesslar
 * @last_modified 2024/02/04 - Gesslar
 *
 * @history
 * 2024/02/04 - Gesslar - Created
 */

#include "/std/object/include/object.h"

/** @type {STD_ZONE} */
private nosave object __zone;

void set_zone(mixed z) {
  assert_arg((stringp(z) && truthy(z)) || objectp(z), 1, "Zone must be a string or a zone object.");

  if(stringp(z)) {
    z = find_path(z);
    __zone = load_object(z);
  } else {
    __zone = z;
  }

  if(!objectp(__zone))
    error("Invalid zone object: " + z);

  if(!__zone->is_zone())
    error("Invalid zone object: " + z);

  __zone->add_room(this_object());
}

string query_zone_name() {
  return objectp(__zone)
    ? __zone->query_zone_name()
    : "Unknown";
}

object query_zone() {
  return __zone;
}
