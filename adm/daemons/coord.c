/**
 * @file /adm/daemons/coord.c
 *
 * Coordinate daemon to hold room data
 *
 * @created 2024-08-18 - Gesslar
 * @last_modified 2024-08-18 - Gesslar
 *
 * @history
 * 2024-08-18 - Gesslar - Created
 */

#include <classes.h>

inherit STD_DAEMON;

inherit CLASS_ROOMINFO;

private nomask mapping rooms = ([ ]);
private nomask mixed *rooms_array = ({ });

void setup() {
  set_persistent(1);
}

void clear() {
  rooms = ([]);
  rooms_array = ({});

  save_data();
}

void set_coordinate_data(mapping m) {
  rooms = m;
  rooms_array = map(values(rooms), (: $1.coords :));

  save_data();
}

mapping get_coordinate_data() {
  return copy(rooms);
}

int *get_coordinates(string room) {
  if(classp(rooms[room]))
    return rooms[room].coords;

  return null;
}

int valid_coordinate(int *coords) {
  int sz = sizeof(rooms_array);
  for(int i = 0; i < sz; i++)
    if(rooms_array[i][0] == coords[0] &&
       rooms_array[i][1] == coords[1] &&
       rooms_array[i][2] == coords[2])
      return 1;

  return 0;
}
