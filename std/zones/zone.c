// /std/area/zones/zone.c
// Rooms belong to zones! The point of a zone daemon is to keep track of all
// the rooms in a zone at a perspective.
//
// Created:     2024/02/04: Gesslar
// Last Change: 2024/02/04: Gesslar
//
// 2024/02/04: Gesslar - Created

inherit STD_DAEMON;

private nosave object *rooms;
private nosave string zone_name;

nomask void mudlib_setup() {
  rooms = ({});

  zone_name = query_file_name(this_object());
  zone_name = replace_string(zone_name, "_", " ");
  zone_name = cap_significant_words(zone_name, 1);
}

void add_room(object room) {
  if(!of(room, rooms)) {
    rooms += ({ room });
  }
}

void remove_room(object room) {
  if(of(member_array(room, rooms))) {
    rooms -= ({ room });
  }
}

object *query_rooms() {
  rooms -= ({ 0 });

  return rooms;
}

int query_num_rooms() {
  return sizeof(query_rooms());
}

string query_zone_name() {
  return zone_name;
}

int request_clean_up() {
  rooms -= ({ 0 });

  if(sizeof(rooms) == 0)
    return 1;

  return 0;
}

int is_zone() {
  return 1;
}
