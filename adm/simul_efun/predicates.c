#include <simul_efun.h>

/**
 * Determine if an object is a room or not.
 *
 * @param {object} ob - The object to test.
 * @returns {ob as STD_ROOM} 1 if the object is a room, otherwise 0.
 */
int roomp(object ob) {
  return objectp(ob) && call_if(ob, "is_room");
}
