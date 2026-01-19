/**
 * @file /std/object/item.c
 * @description Base implementation for physical game items that can be
 *              manipulated, carried, and moved between containers.
 *
 * @created 2024-07-27 - Gesslar
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * 2024-07-27 - Gesslar - Created
 * 2025-03-16 - GitHub Copilot - Added documentation
 */

#include <gmcp_defines.h>

inherit STD_OBJECT;
inherit STD_VALUE;

private nosave mapping _spawn_info = ([]);

/**
 * Sets the spawn information for this item.
 *
 * This records details about when and where the item was created.
 *
 * @param {mapping} info - Mapping of spawn information
 */
void set_spawn_info(mapping info) {
  _spawn_info = info;
}

/**
 * Adds a single piece of spawn information.
 *
 * @param {string} key - The information identifier
 * @param {mixed} value - The information value
 */
void add_spawn_info(string key, mixed value) {
  _spawn_info[key] = value;
}

/**
 * Retrieves a specific piece of spawn information.
 *
 * @param {string} key - The information identifier
 * @returns {mixed} The requested spawn information
 */
mixed query_spawn_info(string key) {
  return _spawn_info[key];
}

/**
 * Returns a copy of all spawn information.
 *
 * @returns {mapping} Copy of the spawn information mapping
 */
mapping query_all_spawn_info() {
  return copy(_spawn_info);
}

/**
 * Determines if the item can be moved to a destination.
 *
 * Performs checks for destination validity, capacity constraints,
 * and container permissions.
 *
 * @param {object|string} dest - Destination object or filename
 * @returns {int} Move result code (MOVE_OK or an error code)
 */
int allow_move(mixed dest) {
  /** @type {STD_CONTAINER} */
  object ob;
  /** @type {STD_CONTAINER} */
  object env;

  if(stringp(dest))
    catch(ob = load_object(dest));
  else if(objectp(dest))
    ob = dest;

  if(!objectp(ob))
    return MOVE_NO_DEST;

  if(environment() == ob)
    return MOVE_ALREADY_THERE;

  if(!ob->can_receive(this_object()))
    return MOVE_NOT_ALLOWED;

  if(mudConfig("USE_MASS"))
    if(!dest->ignore_capacity())
      if(query_mass() + dest->query_fill() > dest->query_capacity())
        return MOVE_TOO_HEAVY;

  if(env)
    if(!env->can_release(this_object()))
      return 0;

  return MOVE_OK;
}

/**
 * Reverts mass and fill adjustments when a move fails.
 *
 * Used internally to maintain consistency in container capacities.
 *
 * @param {mixed*} rollback - Array of rollback information
 */
private void roll_back(mixed *rollback) {
  int i = sizeof(rollback);

  while(i--) {
    if(objectp(rollback[i][0]))
      rollback[i][0]->adjust_mass(rollback[i][1][0]);
    if(objectp(rollback[i][0]))
      rollback[i][0]->adjust_fill(rollback[i][1][1]);
  }
}

/**
 * Moves this item to a new container or environment.
 *
 * Handles mass tracking, capacity limits, and related events.
 * Sends GMCP updates when appropriate.
 *
 * @param {STD_CONTAINER|string} dest - Destination object or filename
 * @returns {int} Move result code (MOVE_OK or an error code)
 */
int move(mixed dest) {
  int result;
  /** @type {STD_CONTAINER} */
  object prev = environment();
  int use_mass = mudConfig("USE_MASS");
  int prev_ignore_capacity = prev ? call_if(prev, "ignore_capacity") : 0;
  int dest_ignore_capacity = call_if(dest, "ignore_capacity") || 0;
  int prev_ignore_mass = prev ? call_if(prev, "ignore_mass") : 0;
  int dest_ignore_mass = call_if(dest, "ignore_mass") || 0;
  int mass = query_mass();
  string e;

  mixed *rollback = ({});

  result = allow_move(dest);

  if(result)
    return result;

  if(!objectp(dest)) {
    if(stringp(dest)) {
      e = catch(dest = load_object(dest));

      if(stringp(e) || !objectp(dest))
        return MOVE_NO_DEST;
    }
    else
      return MOVE_NO_DEST;
  }

  if(prev && prev == dest)
    return MOVE_ALREADY_THERE;

  // 0 = prev, 1 = dest
  rollback = allocate(2);

  if(use_mass) {
    // 0 = object, 1 = ({ mass, fill})
    rollback[0] = allocate(2);

    // Update the mass and fill of the previous environment.
    rollback[0][0] = prev;
    rollback[0][1] = allocate(2, 0);
    if(prev) {
      if(!prev_ignore_mass) {
        if(prev->adjust_mass(-mass))
          rollback[0][1][0] = mass;
        else
          return MOVE_TOO_HEAVY;
      }

      if(!prev_ignore_capacity) {
        if(prev->adjust_fill(-mass))
          rollback[0][1][1] = mass;
        else {
          roll_back(rollback);
          return MOVE_TOO_HEAVY;
        }
      }
    }

    // Update the mass and fill of the new environment. Rolling back if we fail,
    // both the new and the previous environment.
    rollback[1] = allocate(2);
    rollback[1][0] = dest;
    rollback[1][1] = allocate(2, 0);
    if(!dest_ignore_mass) {
      if(dest->adjust_mass(mass))
        rollback[1][1][0] = -mass;
      else {
        roll_back(rollback);
        return MOVE_TOO_HEAVY;
      }
    }

    if(!dest_ignore_capacity) {
      if(dest->adjust_fill(mass))
        rollback[1][1][1] = -mass;
      else {
        roll_back(rollback);
        return MOVE_TOO_HEAVY;
      }
    }
  }

  flush_messages();

  // Ok, we can move now.
  move_object(dest);

  // Notifications for everybody!
  event(this_object(), "moved", prev);

  if(prev && this_object())
    event(prev, "base_released", environment());

  if(this_object())
    event(environment(), "base_received", prev);

  if(userp())
    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_ITEMS_LIST, GMCP_LIST_ROOM);

  if(this_object())
    return MOVE_OK;
  else {
    roll_back(rollback);

    return MOVE_DESTRUCTED;
  }
}
