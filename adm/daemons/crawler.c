/**
 * @file /adm/daemons/crawler.c
 *
 * Crawls the game world room by room, deriving a coordinate grid for
 * every reachable room and handing the result to COORD_D. The crawl
 * is kicked off automatically at boot and walks each room's exits,
 * placing neighbours so that adjacent rooms abut without overlapping.
 *
 * @created 2024-08-21 - Gesslar
 * @last_modified 2026-06-02 - Gesslar
 *
 * @history
 * 2024-08-21 - Gesslar - Created
 * 2026-06-02 - Gesslar - Documented
 */

#include <classes.h>

inherit STD_DAEMON;

inherit CLASS_ROOMINFO;

void crawl_next_room(object tp, mixed arg...);
void process_room(object room, object tp);
object stash_objects(string room_file, object tp);
int *update_coordinates(int *coords, string exit, int *current_size, int *next_size);
string log_file = "/log/crawl.log";

private nosave float crawl_speed = 0.001;

/**
 * Rooms that have been fully crawled, keyed by room file path.
 *
 * @type {([ string: class RoomInfo ])}
 */
private nosave mapping done = ([]);

/**
 * Rooms discovered but not yet crawled, keyed by room file path.
 *
 * @type {([ string: class RoomInfo ])}
 */
private nosave mapping todo = ([]);
private nosave int x, y, z;

private nosave string crawl_start_room;

void setup() {
  crawl_start_room = "/d/village/square";
  slot(SIG_SYS_BOOT, "crawl");
}

/**
 * Begins a fresh crawl of the game world, starting from
 * crawl_start_room and mapping every reachable room into a coordinate
 * grid.
 *
 * Slotted on SIG_SYS_BOOT, so the crawl runs automatically at boot.
 * The stale crawl log is removed, the starting room is stashed and
 * seeded into the todo queue at the origin ({0, 0, 0}), and
 * crawl_next_room is scheduled to process the queue one room at a time
 * on a walltime call_out.
 *
 * @param {mixed...} arg - Optional arguments forwarded through to
 *                         crawl_next_room.
 */
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

/**
 * Processes the next room in the todo queue, or finalises the crawl
 * when the queue is empty.
 *
 * When the queue empties, the accumulated room data is handed to
 * COORD_D, the todo and done state is cleared, and
 * SIG_SYS_CRAWL_COMPLETE is emitted. Otherwise the next queued room is
 * stashed and passed to process_room; rooms that fail to load are
 * dropped and the next step is scheduled.
 *
 * @param {STD_BODY} tp - The crawling player to notify on completion,
 *                        if any.
 * @param {mixed...} arg - Optional arguments carried over from crawl.
 */
void crawl_next_room(object tp, mixed arg...) {
  string file;
  object room;

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

/**
 * Walks every exit of a room, discovering and queuing each unvisited
 * destination with its computed grid coordinates.
 *
 * For each exit, the destination is loaded (stashing any inventory),
 * its grid coordinates are derived from this room's coordinates and
 * the two rooms' sizes, and it is added to the todo queue. Rooms that
 * fail to load are logged and skipped. Once all exits are walked, the
 * room is moved from todo to done and the next crawl step is
 * scheduled.
 *
 * @param {STD_ROOM} room - The room whose exits are being processed.
 * @param {STD_BODY} tp - The crawling player to notify on completion,
 *                        if any.
 */
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

/**
 * Computes the grid coordinates of an adjacent room based on the
 * direction of travel and the sizes of both rooms.
 *
 * The offset along each axis is half the sum of the two rooms' sizes
 * on that axis, so the rooms abut without overlapping. Diagonal exits
 * shift along two axes at once. The result is rounded to the nearest
 * integer grid coordinate.
 *
 * @param {int*} coords - The {x, y, z} coordinates of the current
 *                        room.
 * @param {string} exit - The direction of travel (e.g. "north",
 *                        "southeast", "up").
 * @param {int*} current_size - The {width, height, depth} of the
 *                              current room.
 * @param {int*} next_size - The {width, height, depth} of the
 *                           destination room.
 * @returns {int*} The {x, y, z} grid coordinates of the destination
 *                 room.
 */
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

/**
 * Loads a room fresh while preserving any objects already inside it.
 *
 * If the room is already loaded, its entire inventory is moved into
 * the void, the room is removed and reloaded, and the inventory is
 * moved back. If the room is not loaded, it is simply loaded. This
 * lets the crawler query a clean copy of the room without destroying
 * its live contents.
 *
 * @param {string} room_file - The path of the room to (re)load.
 * @param {STD_BODY} _tp - The crawling player; carried through the
 *                        crawl pipeline but unused here.
 * @returns {STD_ROOM} The freshly loaded room, or 0 if it failed to
 *                      load.
 */
object stash_objects(string room_file, object _tp) {
  /** @type {STD_ROOM} */ object room;
  object v = load_object(ROOM_VOID);

  if(room = find_object(room_file)) {
    /** @type {STD_ITEM | STD_BODY} */ object *all = all_inventory(room);

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
