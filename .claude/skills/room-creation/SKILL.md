---
name: room-creation
description: Create rooms for Threshold RPG using the modern function-based API. Covers the setup chain, exits, descriptions, items, senses, territory, terrain, outside/indoor, monsters, and inventory.
---

# Room Creation Skill

You are creating rooms for Threshold RPG. Rooms use a modern function-based API — prefer calling functions (`set_long()`, `set_exit()`, `set_items()`, `add_sense()`) over raw `set()` property calls wherever functions exist.

## Inheritance Pattern

Rooms use a layered inheritance chain for area organization:

```
STD_ROOM                          ← Base room class
  └─ d/{domain}/{zone}/{zone}.c   ← Area base (sets territory)
       └─ d/{domain}/{zone}/sub.c ← Sub-area (sets sub-territory, terrain)
            └─ d/{domain}/{zone}/room.c  ← Individual room (exits, descriptions)
```

Each layer uses a different setup function to avoid overriding parent setup.

## Setup Chain

The room class calls these functions in order during initialization:

```c
mudlib_setup()   → Internal library setup (don't override)
base_setup()     → Rare — foundational defaults
area_setup()     → Area-wide: territory, terrain, outside
sub_setup()      → Sub-zone overrides
pre_setup()      → Before main setup
setup()          → Main room setup: exits, descriptions, items, senses
post_setup()     → After everything else
```

**Convention:**
- Area base inherits use `area_setup()` for territory/terrain/outside
- Sub-zone inherits use `sub_setup()` for overrides
- Individual rooms use `setup()` for exits, descriptions, items, senses

## Modern API Reference

### Descriptions

```c
// Short description (one-line, shown in room lists and movement)
set_short("Winding Path");
// Supports color: set_short("`<adb>`Gallery of Flora`<res>`");

// Long description (auto-formatted: trimmed, indented, wrapped)
set_long("This wide outdoor space is bordered by tall hedges. "
"The ground is covered in soft green grass.");
// Pass multi-paragraph as separate \n-delimited strings
// The function handles wrapping and indentation automatically
```

### Items (Lookable Things in the Room)

```c
// Set all items at once (replaces existing)
set_items(([
    "plants" : "A wide variety of plants and flowers grow here.",
    "flowers" : ">plants",       // Forward — "look flowers" shows plants desc
    "pond" : "A large circular pond filled with water lilies.",
    "grass" : "Soft, green grass covers the ground.",
    "ground" : ">grass",
]));

// Add individual items
add_item("statue", "A weathered stone statue of a forgotten king.");
add_item(({"tree", "trees", "oak"}), "Tall oaks line the path.");
// Array form: first element gets description, rest forward to it
```

### Senses

```c
// Room-level senses (what the room smells/sounds like)
add_sense("smell", "The air is filled with the sweet scent of flowers.");
add_sense("sound", "A gentle breeze rustles through the leaves.");

// Item-level senses (smelling/touching specific things)
add_sense("smell", "flowers", "The flowers smell sweet and fragrant.");
add_sense("touch", "grass", "The grass is soft and cool to the touch.");
add_sense("sound", "pond", "You hear the gentle lapping of water.");

// Valid senses: "smell", "sound", "touch", "taste"

// Array form (aliases forward to first):
add_sense("smell", ({"flowers", "plants"}), "Sweet fragrance.");
```

### Exits

```c
// Set all exits at once
set("exits", ([
    "north" : __DIR__ "path2",
    "south" : __DIR__ "path1",
    "east"  : "/d/sable/sable/rooms/market",
]));

// Individual exits
set_exit("north", __DIR__ "room2");
set_exit("west", "/d/sable/sable/rooms/gate");

// Use __DIR__ for relative paths within the same directory
```

### Territory

```c
// Set in area_setup() — shared by all rooms in the area
set_territory("Village of Loxos");
```

Territory names are used by the task system, AGEC discovery, and player location display.

### Terrain

```c
// Sets terrain type AND automatically applies movement cost
set_terrain("forest");    // Also calls set_movement_cost() from DICT_TERRAIN

// Override movement cost manually if needed
set_movement_cost(5);     // All directions
set_movement_cost("north", 10);  // Specific direction
```

Common terrain types: `"forest"`, `"swamp"`, `"sand"`, `"savannah"`, `"road"`, `"mountain"`, `"water"`

### Indoor/Outdoor

```c
// Outdoor room — enables weather, dynamic lighting, and ambient light
set_outside("sable");         // Zone name for weather system
set_outside("thrace", 1);     // Second arg = 1 disables weather messages

// Indoor rooms don't call set_outside() — they're indoor by default
// Set light manually for indoor rooms:
set("light", 1);
```

### Resource Zone

```c
// Enable resource gathering (ore, herb, lumber, skin spawns)
set_resource_zone("sable_everwoods");
```

### Monsters and NPCs

```c
// Spawn on reset — use event_reset() callback
void event_reset(object prev) {
    if(present("spawn")) return;  // Don't double-spawn

    if(random(100) > 25) return;  // 25% spawn chance

    object *mons = ({});

    switch(random(100)) {
        case 0..39:
            mons += ({ add_monster(__DIR__ "mon/villager") });
            break;
        case 40..59:
            mons += ({ add_monster("/d/thrace/banyee/mon/gull") });
            break;
        case 60..99:
            mons += ({ add_monster(__DIR__ "mon/guard") });
            break;
    }

    // Tag spawned monsters for cleanup detection
    mons->add("id", ({ "spawn" }));
}
```

### Objects/Inventory

```c
// Add objects to the room
add_inventory("/obj/general/torch");
add_inventory("/std/coins", "danar", 10);  // With arguments
```

### Extra Descriptions

```c
// Append to long description dynamically
add_extra_long("warning", "A sign warns of danger ahead.");
remove_extra_long("warning");

// Append to short description
add_extra_short("weather", " (raining)");
```

### Pre-Exit Functions

```c
// Gate or modify movement in specific directions
set("pre_exit_func/north", "@@check_north");

int check_north(object tp, string dir) {
    if(!tp->query("has_key")) {
        tp->tell("The gate is locked.\n");
        return 1;  // Block movement
    }
    return 0;  // Allow movement
}
```

### Room Flags

```c
set("no_attack", 1);       // No combat allowed
set("no_teleport", 1);     // Cannot teleport here
set("no_peek", 1);         // Cannot peek into adjacent rooms
set("no_discovery", 1);    // No territory discovery trigger
set("no_mount", 1);        // Cannot ride mounts
```

## Complete Room Example

```c
// /d/akanee/kiyl/loxos/shrine1.c
inherit __DIR__ "shrine";

void setup() {
    set_short("`<adb>`Gallery of Flora`<res>`");
    set_long("This wide, outdoor space, bordered to the east and west by "
    "tall hedges, is filled with a variety of plants and flowers. The "
    "ground is covered in a thick layer of soft, green grass.");

    set_items(([
        "plants"  : "A wide variety of plants and flowers grow here.",
        "flowers" : ">plants",
        "grass"   : "The ground is covered in soft, green grass.",
        "ground"  : ">grass",
        "pond"    : "A large circular pond filled with water lilies.",
    ]));

    add_sense("smell", "The air is filled with the sweet scent of flowers.");
    add_sense("smell", "flowers", "The flowers smell sweet and fragrant.");
    add_sense("sound", "Very little sound here, other than rustling leaves.");
    add_sense("touch", "grass", "The grass is soft and cool to the touch.");

    set("exits", ([
        "north" : __DIR__ "shrine2",
        "south" : __DIR__ "path21",
    ]));
}
```

## Inheritance Templates

### Area Base (shared by all rooms in a zone)

```c
// /d/{domain}/{zone}/{zone}.c
inherit STD_ROOM;

void area_setup() {
    set_territory("TERRITORY_NAME");
}
```

### Sub-Zone Override

```c
// /d/{domain}/{zone}/sub.c
inherit __DIR__ "ZONE";

void sub_setup() {
    set_territory("SUB_TERRITORY");
    set_terrain("TERRAIN_TYPE");
    set_outside("WEATHER_ZONE");
}
```

### Simple Room (Exits Only)

```c
inherit __DIR__ "ZONE_OR_SUB";

void setup() {
    set("exits", ([
        "north" : __DIR__ "room2",
        "south" : __DIR__ "room1",
    ]));
}
```

### Full Room

```c
inherit __DIR__ "ZONE_OR_SUB";

void setup() {
    set_short("ROOM_NAME");
    set_long("ROOM_DESCRIPTION");

    set_items(([
        "KEY" : "DESCRIPTION",
    ]));

    add_sense("smell", "ROOM_SMELL");

    set("exits", ([
        "DIRECTION" : __DIR__ "TARGET",
    ]));
}
```

## Key Files

```
std/room.c                      — Base room class
std/room/*.c                    — Room modules (exits, terrain, territory, etc.)
std/modules/description.c       — set_long, set_items, add_sense, add_item
std/room/exits.c                — set_exit, set_exits, add_exit
std/room/terrain.c              — set_terrain, set_movement_cost
std/room/territory.c            — set_territory
std/room/indoor_outdoor.c       — set_outside
std/room/resource.c             — set_resource_zone
std/room/dynamic_desc.c         — set_dynamic_room_descs
```

## Common Pitfalls

- **Using `set("long", ...)` instead of `set_long()`**: the function auto-formats with indentation and wrapping. Raw `set()` doesn't.
- **Using `set("item_desc", ...)` instead of `set_items()`**: the function auto-formats descriptions. Use `set_items()` or `add_item()`.
- **Forgetting `__DIR__`**: use `__DIR__ "filename"` for relative paths within the same directory. Avoids hardcoding full paths.
- **Overriding `setup()` in an inherit**: use `area_setup()` for area bases, `sub_setup()` for sub-zones. Individual rooms use `setup()`.
- **Double-spawning monsters**: always check `if(present("spawn")) return;` at the top of `event_reset()` and tag spawns with a zone-specific ID.
- **Item forwarding**: use `">key"` to forward one item's description to another (e.g. `"flowers" : ">plants"`). The `>` prefix is the forwarding syntax.
- **Indoor rooms**: don't call `set_outside()`. Set `set("light", 1)` for lit indoor rooms.
