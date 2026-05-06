/**
 * @file /adm/daemons/crawler.c
 * This daemon will crawl the game and setup coordinates in the
 * coordinates daemon.
 *
 * @created 2024-08-21 - Gesslar
 * @last_modified 2024-08-21 - Gesslar
 *
 * @history
 * 2024-08-21 - Gesslar - Created
 */

#include <classes.h>

inherit STD_DAEMON;

inherit CLASS_ROOMINFO;

void crawl_next_room();
void process_room(object room, object tp);
object stash_objects(string room_file, object tp);
int *update_coordinates(int *coords, string exit, int *current_size, int *next_size);
string log_file = "/log/crawl.log";

private nosave float crawl_speed = 0.001;
private mapping done = ([]);
private mapping todo = ([]);
private int x, y, z;

private nosave string crawl_start_room;

void setup() {
  crawl_start_room = "/d/village/square";
  slot(SIG_SYS_BOOT, "crawl");
}

void crawl(mixed arg...) {
  object room;
  object tp = this_player();

  rm(log_file);
  room = stash_objects(crawl_start_room, tp);

  if(!room)
    return;

  call_out_walltime("crawl_next_room", crawl_speed, tp, arg);

  todo[file_name(room)] = new(class RoomInfo,
    short: room->query_short(),
    todo: room->query_exit_ids(),
    done: ({}),
    coords: ({0, 0, 0}),
    size: ({1, 1, 1})
  );
}

void crawl_next_room(object tp, mixed arg...) {
  string file;
  object room;
  object v = load_object(ROOM_VOID);

  if(!sizeof(todo)) {
    if(tp)
      tell(tp, sprintf("Crawling complete. Total rooms discovered: %d\n", sizeof(done)));

    COORD_D->set_coordinate_data(done);
    done = ([ ]);
    todo = ([ ]);
    emit(SIG_SYS_CRAWL_COMPLETE);
    return;
  }

  file = keys(todo)[0];
  room = stash_objects(file, tp);

  if(!room) {
    map_delete(todo, file);
    call_out_walltime("crawl_next_room", crawl_speed, tp, arg);
    return;
  }

  process_room(room, tp);
}

void process_room(object room, object tp) {
  string file = file_name(room);
  class RoomInfo room_data = todo[file];
  string exit;
  string dest;

  while(sizeof(room_data.todo)) {
    exit = room_data.todo[0];
    dest = room->query_exit(exit);

    if(dest && !done[dest] && !todo[dest]) {
      object next_room;
      string e;

      e = catch( next_room = stash_objects(dest, tp) );
      if(e) {
        write_file(log_file, sprintf("Failed to load %s => %s via %s\n", file, dest, exit));
        continue;
      }

      if(next_room) {
        int *next_size;
        int *new_coords;

        next_size = next_room->query_room_size() || ({1, 1, 1});
        new_coords = update_coordinates(room_data.coords, exit, room_data.size, next_size);
        todo[dest] = new(class RoomInfo,
          short: next_room->query_short(),
          todo: next_room->query_exit_ids(),
          done: ({}),
          coords: new_coords,
          size: next_size
        );
      }
    }

    room_data.todo = room_data.todo[1..];
    room_data.done += ({ dest });
  }

  done[file] = room_data;
  map_delete(todo, file);
  call_out_walltime("crawl_next_room", crawl_speed, tp);
}

int *update_coordinates(int *coords, string exit, int *current_size, int *next_size) {
  float *new_coords = ({ to_float(coords[0]), to_float(coords[1]), to_float(coords[2]) });

  switch(exit) {
    case "north": new_coords[1] += (current_size[1] + next_size[1]) / 2.0 ; break;
    case "south": new_coords[1] -= (current_size[1] + next_size[1]) / 2.0 ; break;
    case "east":  new_coords[0] += (current_size[0] + next_size[0]) / 2.0 ; break;
    case "west":  new_coords[0] -= (current_size[0] + next_size[0]) / 2.0 ; break;
    case "up":    new_coords[2] += (current_size[2] + next_size[2]) / 2.0 ; break;
    case "down":  new_coords[2] -= (current_size[2] + next_size[2]) / 2.0 ; break;
    case "northeast":
      new_coords[0] += (current_size[0] + next_size[0]) / 2.0;
      new_coords[1] += (current_size[1] + next_size[1]) / 2.0;
      break;
    case "northwest":
      new_coords[0] -= (current_size[0] + next_size[0]) / 2.0;
      new_coords[1] += (current_size[1] + next_size[1]) / 2.0;
      break;
    case "southeast":
      new_coords[0] += (current_size[0] + next_size[0]) / 2.0;
      new_coords[1] -= (current_size[1] + next_size[1]) / 2.0;
      break;
    case "southwest":
      new_coords[0] -= (current_size[0] + next_size[0]) / 2.0;
      new_coords[1] -= (current_size[1] + next_size[1]) / 2.0;
      break;
  }
  return ({ to_int(round(new_coords[0])), to_int(round(new_coords[1])), to_int(round(new_coords[2])) });
}

object stash_objects(string room_file, object tp) {
  object room;
  object v = load_object(ROOM_VOID);

  if(room = find_object(room_file)) {
    object *all = all_inventory(room);

    if(sizeof(all))
      all->move(v);

    room->remove();
    room = load_object(room_file);
    all->move(room);
  } else {
    catch(room = load_object(room_file));
  }

  return room;
}
