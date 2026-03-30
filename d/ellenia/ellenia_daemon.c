/**
 * @file /d/ellenia/ellenia_daemon.c
 * @description Virtual map daemon for the world!
 *
 * @created 2024-09-25 - Gesslar
 * @last_modified 2024-09-25 - Gesslar
 *
 * @history
 * 2024-09-25 - Gesslar - Created
 */

inherit STD_VIRTUAL_MAP;

inherit EXT_NOISE;

protected void generate_map();
private varargs string determine_room_type(int z, int y, int x);
private void setup_wastes_shorts();
private void setup_wastes_longs();

private nosave mapping short_descriptions;
private nosave mapping long_descriptions;

// The seed is used to ensure that every time the daemon runs, it uses the same
// seed, resulting in the same map.

// Uncomment this to use the boot number as the seed.
// private nosave mixed seed = BOOT_D->query_boot_number();

// Testing seed!
private nosave mixed seed = 42;

void setup() {
  set_no_clean(1);
  init_noise(MAX_INT, MAX_INT, seed);

  // Apply the map generator. This will call the generate_map function and
  // and alert the inherited virtual_map that we are using an indeterminate map.
  apply_map_generator((:generate_map:), true);

  // Setup the short and long descriptions.
  setup_wastes_shorts();
  setup_wastes_longs();
}

// Generate and return the map to the parent virtual_map.
protected void generate_map() {
  // we are not actually doing anything here.
  // for an indeterminate map, we don't maintain map dimensions or
  // anything as it is generated on demand.
}

private void setup_wastes_shorts() {
  short_descriptions = ([
    "wastes" : ({
      "Barren Expanse",
      "Scorched Earth",
      "Desolate Stretch",
      "Rugged Terrain",
    }),
    "shallow water" : ({
      "Shallow Water",
    }),
    "water" : ({
      "River",
      "Water",
    }),
    "deep water" : ({
      "Deep Water",
      "Deep River",
    }),
  ]);
}

private void setup_wastes_longs() {
  long_descriptions = ([
    "wastes" : ({
      "The wasteland stretches before you, a desolate expanse of cracked "
      "earth and scattered debris. The air is dry and carries the faint scent "
      "of decay, with the sun beating down mercilessly.",

      "This area of the wasteland is littered with remnants of a forgotten "
      "past, rusted metal and broken glass glinting in the harsh light. The "
      "ground is uneven, with deep fissures and patches of thorny vegetation.",

      "A chilling wind sweeps across the wasteland, stirring up dust and "
      "debris. The landscape is dotted with skeletal remains of long-dead "
      "trees, their twisted branches reaching out like bony fingers.",

      "Silence reigns in this part of the wasteland, broken only by the "
      "occasional whistle of the wind. The ground is barren, with only the "
      "hardiest of plants struggling to survive in the harsh conditions."
    }),
    "shallow water" : ({
      "The shallow water glimmers under the sun, revealing a sandy bottom "
      "where small fish dart about.",

      "Gentle ripples disturb the surface of the shallow water, creating "
      "patterns that dance in the light.",

      "The water is cool and clear, inviting weary travelers to take a moment "
      "to refresh themselves.",

      "Surrounded by lush greenery, the shallow water reflects the vibrant "
      "colors of the flora around it."
    }),
    "water" : ({
      "The river flows steadily, its surface reflecting the sky above, "
      "while the sound of rushing water fills the air.",

      "This body of water is deep and wide, with currents that can be both "
      "calm and treacherous.",

      "The water is a rich blue, teeming with life beneath its surface, "
      "where fish swim and plants sway.",

      "Along the banks, the ground is soft and muddy, a testament to the "
      "power of the flowing water."
    }),
    "deep water" : ({
      "The deep water is dark and mysterious, hiding secrets beneath its "
      "surface that few have dared to uncover.",

      "A sense of foreboding surrounds the deep water, where shadows loom "
      "and the depths seem to stretch endlessly.",

      "The water here is cold and still, a stark contrast to the lively "
      "shallows nearby.",

      "Occasionally, the surface breaks as a large creature stirs below, "
      "reminding all of the dangers that lurk in the depths."
    }),
  ]);
}

public void setup_short(object room, string file) {
  int *coords = room->get_virtual_coordinates();
  string room_type = determine_room_type(coords...);

  switch(room_type) {
    case "deep water": case "water": case "shallow water":
      room->set_short(element_of(short_descriptions[room_type]));
      break;
    default:
      room->set_short(element_of(short_descriptions["wastes"]));
      break;
  }
}

public void setup_long(object room, string file) {
  int *coords = room->get_virtual_coordinates();
  string room_type = determine_room_type(coords...);

  switch(room_type) {
    case "deep water": case "water": case "shallow water":
      room->set_long(element_of(long_descriptions[room_type]));
      break;
    default:
      room->set_long(element_of(long_descriptions["wastes"]));
      break;
  }
}

// Setup the exits for the room. This is used to connect the rooms together.
// This is really not that complicated.
public void setup_exits(object room) {
  int *coords = room->get_virtual_coordinates();
  int x, y, z;
  string current_room_type;
  string room_type;
  mapping exits;
  string maze_entrance_file, maze_exit_file;
  int *maze_entrance, *maze_exit;
  int *cavern_entrance, *cavern_exit;

  x = coords[2];
  y = coords[1];
  z = coords[0];

  current_room_type = determine_room_type(z, y, x);
  debug("Current room type: %s", current_room_type);
  if(current_room_type == "impassable")
    return;

  // West
  if((room_type = determine_room_type(z, y, x-1)) != "impassable")
    room->add_exit("west", sprintf("%d,%d,%d", x-1, y, z));

  // East
  if((room_type = determine_room_type(z, y, x+1)) != "impassable")
    room->add_exit("east", sprintf("%d,%d,%d", x+1, y, z));

  // South
  if((room_type = determine_room_type(z, y+1, x)) != "impassable")
    room->add_exit("south", sprintf("%d,%d,%d", x, y+1, z));

  // North
  if((room_type = determine_room_type(z, y-1, x)) != "impassable")
    room->add_exit("north", sprintf("%d,%d,%d", x, y-1, z));

  // Northeast
  if((room_type = determine_room_type(z, y-1, x+1)) != "impassable")
    room->add_exit("northeast", sprintf("%d,%d,%d", x+1, y-1, z));

  // Northwest
  if((room_type = determine_room_type(z, y-1, x-1)) != "impassable")
    room->add_exit("northwest", sprintf("%d,%d,%d", x-1, y-1, z));

  // Southeast
  if((room_type = determine_room_type(z, y+1, x+1)) != "impassable")
    room->add_exit("southeast", sprintf("%d,%d,%d", x+1, y+1, z));

  // Southwest
  if((room_type = determine_room_type(z, y+1, x-1)) != "impassable")
    room->add_exit("southwest", sprintf("%d,%d,%d", x-1, y+1, z));
}
private nosave string *terrain_types = ({ "rocky", "sandy", "impassable", });
private nosave mixed *thresholds = ({
  ({ 0.3, "impassable" }),   //  0 -  30%
  ({ 1.2, "sandy" }),        // 30 -  60%
  ({ 1.7, "rocky" }),        // 60 -  85%
  ({ 2.0, "impassable" }),   // 85 - 100%
});

// Determine the room type based on the noise value.
private varargs string determine_room_type(int z, int y, int x) {
  string room_type;
  float value = simplex2(
    to_float(x) / 25.0,
    to_float(y) / 15.0
  ) + 1.0;

  debug("Noise value for x%d,y%d,z%d: %f", x, y, z, value);

  if(nullp(value))
    return null;

  // First, let's check if the room is part of the river and set it up
  // accordingly and then return.
  if(value == -1.0)
    return "deep water";
  else if(value == -0.5)
    return "water";
  else if(value == -0.25)
    return "shallow water";
  else
    // If it's not part of the river, then we can determine the room type
    // based on the noise value.
    foreach(mixed threshold in thresholds) {
      int threshold_value = threshold[0];
      string terrain_type = threshold[1];

      if(value <= threshold_value) {
        room_type = terrain_type;
        break;
      }
    }

  return room_type;
}

// Setup the room type for the room. This is used to set the environment of the
// room.
public void setup_room_type(object room) {
  int *coords = room->get_virtual_coordinates();
  string room_type;
  room_type = determine_room_type(coords...);

  if(nullp(room_type))
    return;

  room->set_room_environment(room_type);
}
