/**
 * @file /std/room/room.c
 * @description A generic room object that can be inherited by any room.
 *
 * @created 2024-08-11 - Gesslar
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * 2024-08-11 - Gesslar - Created
 * 2025-03-16 - GitHub Copilot - Added documentation
 */

#include <room.h>

inherit STD_OBJECT;
inherit STD_CONTAINER;

inherit __DIR__ "exits";
inherit __DIR__ "items";
inherit __DIR__ "light";
inherit __DIR__ "terrain";
inherit __DIR__ "zone";
inherit __DIR__ "door";

private nosave int *_size = ({1, 1, 1});

/**
 * Sets up the room with default values.
 *
 * Configures the container to ignore capacity and mass limits
 * and adds door resetting to the reset handler.
 *
 * @param {mixed} args - Variable arguments passed to setup
 */
void mudlib_setup(mixed args...) {
  set_ignore_capacity(1);
  set_ignore_mass(1);
  add_reset((: reset_doors :));
}

private nosave string room_type = "room";
private nosave string room_subtype = "";
private nosave string room_icon = "";
private nosave string room_environment = null;
private nosave int room_colour = null;
private nosave mapping custom_gmcp = ([ ]);
private nosave int no_gmcp_room_info = 0;

/**
 * Generates room information for GMCP protocol.
 *
 * Creates a data structure containing all relevant room information
 * including exits, doors, environment, coordinates, and custom data.
 *
 * @returns {mapping} Mapping with room information
 */
mapping gmcp_room_info() {
  mapping info = ([ ]);
  string *exit_dirs;
  string *door_dirs;
  mapping exits;
  mapping doors;
  mapping result;
  mapping gmcp_info = ([]);
  // string *suppress;

  if(no_gmcp_room_info)
    return info;

  exit_dirs = query_exit_ids();
  door_dirs = query_door_ids();

  exits = query_exits();
  // suppress = query_exit_suppress() || ({});
  // exits = filter(exits, (: member_array($1, $(suppress)) == -1 :));
  exits = allocate_mapping(exit_dirs, (: query_exit :));
  exits = map(exits, (: base_name($2) :));
  exits = map(exits, (: hash("md4", $2) :));

  doors = query_doors();
  doors = map(doors, function(string dir, class Door door) {
    return ([
      "hash" : hash("md4", query_exit(dir)),
      "name" : door.name,
      "status" : query_door_status(dir, 1),
      "type" : door.type,
      "short" : door.short,
      "long" : door.long,
    ]);
  });

  result = ([
    "area"       : query_zone_name(),
    "hash"       : hash("md4", base_name()),
    "name"       : no_ansi(query_short()),
    "exits"      : exits,
    "doors"      : doors,
    "environment": query_room_environment() || query_terrain(),
    "coords"     : COORD_D->get_coordinates(base_name()),
    "size"       : _size,
    "type"       : room_type,
    "subtype"    : room_subtype,
    "icon"       : room_icon,
  ]);

  if(room_colour)
    result["color"] = COLOUR_D->colourToRgb(room_colour);

  gmcp_info = query_custom_gmcp();
  if(sizeof(gmcp_info))
    result["custom"] = gmcp_info;

  return result;
}

/**
 * Sets the room's environment.
 *
 * Environment affects gameplay mechanics like weather effects.
 *
 * @param {string} environment - The environment type
 * @returns {void}
 */
void set_room_environment(string environment) {
  room_environment = environment;
}

/**
 * Returns the room's environment setting.
 *
 * @returns {string} The room environment
 */
string query_room_environment() { return room_environment; }

/**
 * Sets the room's color for map display.
 *
 * @param {int} colour - The color value
 * @returns {void}
 */
void set_room_colour(int colour) {
  room_colour = colour;
}

/**
 * Returns the room's color setting.
 *
 * @returns {int} The room color value
 */
int query_room_colour() { return room_colour; }

/**
 * Sets the room's size in three dimensions.
 *
 * Size affects movement costs and other spatial calculations.
 *
 * @param {int*} size - Array of 3 integers representing width, length, height
 */
void set_room_size(int *size) {
  _size = size;
}

/**
 * Returns the room's size.
 *
 * @returns {int*} Array of 3 integers representing width, length, height
 */
int *query_room_size() { return _size; }

private nosave float _base_move_cost = 2.0;

/**
 * Calculates the movement cost for a given direction.
 *
 * Movement costs vary based on room size and direction of travel.
 * Diagonal movement combines costs of the relevant dimensions.
 *
 * @param {string} dir - The direction of movement
 * @returns {float} The calculated movement cost
 */
float move_cost(string dir) {
  int *size = query_room_size();
  float cost;

  switch(dir) {
    case "north":
      cost = to_float(size[0]);
      break;
    case "south":
      cost = to_float(size[0]);
      break;
    case "east":
      cost = to_float(size[1]);
      break;
    case "west":
      cost = to_float(size[1]);
      break;
    case "northeast":
      cost = (to_float(size[0]) + to_float(size[1])) / 2.0;
      break;
    case "northwest":
      cost = (to_float(size[0]) + to_float(size[1])) / 2.0;
      break;
    case "southeast":
      cost = (to_float(size[0]) + to_float(size[1])) / 2.0;
      break;
    case "southwest":
      cost = (to_float(size[0]) + to_float(size[1])) / 2.0;
      break;
    case "up":
      cost = to_float(size[2]);
      break;
    case "down":
      cost = to_float(size[2]);
      break;
    default:
      cost = 1.0;
      break;
  }

  return cost * _base_move_cost;
}

/**
 * Extracts coordinates from the filename if it's formatted as x,y,z.
 *
 * Used for virtual/procedurally generated rooms.
 *
 * @returns {int*} Array of coordinates [z, y, x] or null if not applicable
 */
int *get_virtual_coordinates() {
  string file = query_file_name();
  int x, y, z;

  if(sscanf(file, "%d,%d,%d", x, y, z) != 3)
    return null;

  return ({z, y, x});
}

/**
 * Enables or disables GMCP room information broadcasting.
 *
 * @param {int} no_gmcp - 1 to disable GMCP room info, 0 to enable
 */
void set_no_gmcp_room_info(int no_gmcp) {
  no_gmcp_room_info = no_gmcp;
}

/**
 * Adds custom data to the GMCP room information.
 *
 * @param {string} key - The key for the custom data
 * @param {mixed} value - The value to store
 */
void add_custom_gmcp(string key, mixed value) {
  custom_gmcp[key] = value;
}

/**
 * Removes custom data from the GMCP room information.
 *
 * @param {string} key - The key to remove
 */
void remove_custom_gmcp(string key) {
  map_delete(custom_gmcp, key);
}

/**
 * Returns all custom GMCP data.
 *
 * @returns {mapping} Mapping of custom GMCP data
 */
mapping query_custom_gmcp() { return custom_gmcp; }

/**
 * Sets the room's primary type.
 *
 * @param {string} type - The room type
 * @returns {string} The new room type
 */
string set_room_type(string type) { return room_type = type; }

/**
 * Sets the room's subtype.
 *
 * @param {string} subtype - The room subtype
 * @returns {string} The new room subtype
 */
string set_room_subtype(string subtype) { return room_subtype = subtype; }

/**
 * Sets the room's icon for map displays.
 *
 * @param {string} icon - The icon identifier
 * @returns {string} The new room icon
 */
string set_room_icon(string icon) { return room_icon = icon; }

/**
 * Returns the room's type.
 *
 * @returns {string} The room type
 */
string query_room_type() { return room_type; }

/**
 * Returns the room's subtype.
 *
 * @returns {string} The room subtype
 */
string query_room_subtype() { return room_subtype; }

/**
 * Returns the room's icon.
 *
 * @returns {string} The room icon
 */
string query_room_icon() { return room_icon; }

/**
 * Indicates if the room shouldn't be renewed.
 *
 * @returns {int} Always returns 1
 */
int no_renew() { return 1; }

/**
 * Identifies this object as a room.
 *
 * Used for type checking.
 *
 * @returns {int} Always returns 1
 */
int is_room() { return 1; }
