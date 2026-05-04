/**
 * @file /d/village/field_daemon.c
 * @description Daemon for handling the field stuff
 *
 * @created 2024-07-24 - Gesslar
 * @last_modified 2024-07-24 - Gesslar
 *
 * @history
 * 2024-07-24 - Gesslar - Created
 */

inherit STD_DAEMON;

// Functions
private void setup_field_shorts();
private void setup_field_longs();

// Variables
private nosave mapping field_exits = ([]);

void setup() {
  setup_field_shorts();
  setup_field_longs();
}

/**
 *
 * @param {STD_ROOM} room
 */
public void setup_exits(object room) {
  int *coords = room->get_virtual_coordinates();
  mapping exits = ([]);

  if(!coords)
    return;

  // Check and add exits for each direction. room.c returns coordinates as
  // (z, y, x), but we need to convert them to (x, y, z) for the exits.
  // west
  if(coords[2] > 0) exits["west"] = sprintf("%d,%d,%d", coords[2]-1, coords[1], coords[0]);
  // east
  if(coords[2] < 9) exits["east"] = sprintf("%d,%d,%d", coords[2]+1, coords[1], coords[0]);
  // south
  if(coords[1] < 9) exits["south"] = sprintf("%d,%d,%d", coords[2], coords[1]+1, coords[0]);
  // north
  if(coords[1] > 0) exits["north"] = sprintf("%d,%d,%d", coords[2], coords[1]-1, coords[0]);
  // northwest
  if(coords[2] > 0 && coords[1] > 0) exits["northwest"] = sprintf("%d,%d,%d", coords[2]-1, coords[1]-1, coords[0]);
  // northeast
  if(coords[2] < 9 && coords[1] > 0) exits["northeast"] = sprintf("%d,%d,%d", coords[2]+1, coords[1]-1, coords[0]);
  // southwest
  if(coords[2] > 0 && coords[1] < 9) exits["southwest"] = sprintf("%d,%d,%d", coords[2]-1, coords[1]+1, coords[0]);
  // southeast
  if(coords[2] < 9 && coords[1] < 9) exits["southeast"] = sprintf("%d,%d,%d", coords[2]+1, coords[1]+1, coords[0]);

  // Special case for the bottom-left corner (0,9,0)
  if(coords[2] == 0 && coords[1] == 9 && coords[0] == 0)
    exits["south"] = "../village_path1";

  room->set_exits(exits);
}

private nosave string *field_shorts;

void setup_field_shorts() {
  field_shorts = ({
    "Open Field",
    "Grassy Meadow",
    "Quiet Field",
    "Grassy Plains",
  });
}

/**
 *
 * @param {STD_ROOM} room
 * @param {string} _file
 */
public void setup_short(object room, string _file) {
  room->set_short(element_of(field_shorts) + " ("+query_file_name(room)+")");
}

private nosave int rotation = 0;
private nosave string *field_longs;

public void setup_field_longs() {
  field_longs = ({
    "North of the village, an open grassy field spreads out, the tall "
    "blades of grass swaying gently in the breeze. The field is a vibrant "
    "green, with the occasional wildflower peeking through, adding "
    "splashes of colour to the verdant landscape. The air is fresh and "
    "carries the faint scent of earth and grass, creating a serene and "
    "peaceful atmosphere. This expansive field invites travelers to "
    "wander and enjoy the natural beauty that lies just beyond the "
    "village.",

    "Just north of the village, a sunny grassy meadow extends into the "
    "distance, the grass soft and lush underfoot. The meadow is bathed in "
    "warm sunlight, casting a golden hue over the landscape. The gentle "
    "rustling of the grass in the breeze is the only sound that breaks "
    "the tranquil silence. This open space offers a perfect retreat from "
    "the bustle of village life, a place to pause and appreciate the "
    "simple beauty of the natural world.",

    "To the north of the village, a quiet grassy expanse unfolds, its "
    "soft green grass dotted with tiny, delicate flowers. The area is "
    "peaceful, with the gentle hum of insects and the occasional birdsong "
    "filling the air. The grass sways rhythmically with the wind, "
    "creating a soothing and calming effect. This tranquil field serves "
    "as a serene escape, a place to relax and take in the beauty of the "
    "countryside.",

    "Northward from the village, wide grassy plains stretch as far as the "
    "eye can see, the grass tall and undisturbed. The plains are a sea of "
    "green, with the occasional gust of wind sending ripples through the "
    "grass. The open sky above adds to the sense of vastness and freedom, "
    "making it easy to lose oneself in the natural splendor of the "
    "landscape. This open field is an ideal spot for those seeking "
    "solitude and a connection with nature.",
  });
}

/**
 *
 * @param {STD_ROOM} room
 * @param {string} _file
 */
public void setup_long(object room, string _file) {
  room->set_long(field_longs[rotation]);

  if(++rotation == sizeof(field_longs))
    rotation = 0;
}
