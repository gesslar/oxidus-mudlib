---
name: virtual-area-creation
description: Create virtual (procedurally generated) areas for Threshold RPG. Generates zone virtual servers, room templates, map files, map daemons, and description daemons for coordinate-based wilderness areas.
---

# Virtual Area Creation Skill

You are helping create virtual areas for Threshold RPG. Virtual areas are procedurally generated rooms that don't exist as individual files — instead, coordinate-based paths like `/d/domain/zone/5,10` are compiled on-the-fly from a template room and map data.

Virtual areas are used for large open environments: forests, beaches, plains, underwater zones, sewers — anywhere hand-building hundreds of rooms would be impractical.

## How Virtual Areas Work

1. Player moves to `/d/domain/zone/5,10`
2. No file exists — the driver calls `master->compile_object()`
3. The virtual daemon finds `d/domain/zone/virtual.c` (the zone's virtual server)
4. The server's `generate_object()` validates coordinates against a map, then creates a room: `new(template, "5,10")`
5. The room's `init_virtual(5, 10, 0)` is called with parsed coordinates
6. The room sets up exits, descriptions, and monsters based on its coordinates

## Discovery Method

**Always use the new-style discovery.** Place the virtual server at `d/{domain}/{zone}/virtual.c` — directly in the zone directory. The system finds it automatically by parsing the path. This also enables automatic subzone delegation when `d/{domain}/{zone}/{subzone}/virtual.c` exists.

A legacy method exists where the server lives at `d/{domain}/virtual/server.c` at the domain level. Do not use this for new areas.

## Two Exit Calculation Patterns

Both patterns use the same new-style discovery. They differ in how exits between rooms are determined.

### Pattern 1: Direct Map

The room template reads the raw map data and checks adjacent cells for blocked/open directions. All 8 directions are set as exits, then blocked ones get pre-exit functions that prevent movement.

**Best for**: Open areas where most directions are available and only some are blocked (forests, plains).

**Files needed**:
- `d/{domain}/{zone}/virtual.c` — inherits `STD_VIRTUAL_SERVER`
- `d/{domain}/{zone}/{zone}.c` — room template inherits `STD_ROOM`
- `d/{domain}/{zone}/{zone}.map` — ASCII map (`*` = blocked, space = open, `X` = special)

### Pattern 2: STD_VIRT_MAP

A separate map daemon loads the map and calculates exits from connection symbols between room positions. Only explicitly connected directions become exits.

**Best for**: Areas with selective connectivity — not all-8-directions (caves, paths, structured layouts).

**Files needed**:
- `d/{domain}/{zone}/virtual.c` — inherits `STD_VIRTUAL_SERVER`
- `d/{domain}/{zone}/{zone}_inherit.c` — room template inherits `STD_ROOM`
- `d/{domain}/{zone}/{zone}_map.c` — map daemon inherits `STD_VIRT_MAP`
- `d/{domain}/{zone}/{zone}.map` — ASCII map with connection symbols

## Creating a Virtual Area

### Step 1: Draw the map

**Pattern 1 map** — one character per cell:
```
***************
*****     *****
***         ***
**    X      **
***         ***
*****     *****
***************
```
- `*` or `#` = impassable
- space = walkable room
- `X` = special location
- Border is typically added programmatically

**Pattern 2 map** — rooms at even positions, connection symbols at odd:
```
O-O-O-O
|/|   |
O-O O-O
|\| |/|
O-O-O-O
```
- `O` = walkable room
- `X` = special room
- `.` = out of bounds
- `|` = north/south connection
- `-` = east/west connection
- `/` = NE/SW diagonal
- `\` = NW/SE diagonal
- First two lines of file are skipped (header/blank)

### Step 2: Create the virtual server

`d/{domain}/{zone}/virtual.c`:

```c
inherit STD_VIRTUAL_SERVER;

protected object generate_object(string file) {
    string *parts;
    int x, y;

    parts = path_file(file);

    if(sscanf(parts[1], "%d,%d", x, y) != 2)
        return 0;

    // Validate coordinates — choose one:
    // Pattern 1: validate against own map
    if(!valid_coord(parts[1]))
        return 0;

    // Pattern 2: validate against map daemon
    // if(!abs_path("zone_map")->within_bounds(parts[1]))
    //     return 0;

    return new(abs_path("ZONE_TEMPLATE"), parts[1]);
}
```

For Pattern 1, the server also needs to load and expose the map (see Misthaven Forest example in the reference guide).

### Step 3: (Pattern 2 only) Create the map daemon

`d/{domain}/{zone}/{zone}_map.c`:

```c
inherit STD_VIRT_MAP;

void setup() {
    set_exit_mask(abs_path("%d,%d"));
    load_map_file(abs_path("ZONE.map"));
}
```

`set_exit_mask()` defines the sprintf format for generating exit paths. `abs_path("%d,%d")` produces paths relative to this file's directory.

### Step 4: Create the room template

**Pattern 1** — `d/{domain}/{zone}/{zone}.c`:

```c
inherit STD_ROOM;

#define VSERVER __DIR__ "virtual"
#define EMASK   __DIR__ "%d,%d"
#define SPAWN_RATE 25

private nosave int XPos, YPos;

void area_setup() {
    set_territory("TERRITORY_NAME");
    set_outside("OUTSIDE_TYPE");
    set_terrain("TERRAIN_TYPE");

    set("short", "@@query_short");
    set("long", "@@query_long");
}

void init_virtual(int coord_x, int coord_y, int coord_z) {
    if(nullp(XPos) || nullp(YPos)) {
        XPos = coord_x;
        YPos = coord_y;
        setup_exits(coord_x, coord_y, coord_z);
    }
}

void setup_exits(int x, int y, int z) {
    string *exits_map;

    // Set all 8 exits
    set("exits/north",     sprintf(EMASK, x,   y-1));
    set("exits/south",     sprintf(EMASK, x,   y+1));
    set("exits/east",      sprintf(EMASK, x+1, y  ));
    set("exits/west",      sprintf(EMASK, x-1, y  ));
    set("exits/northeast", sprintf(EMASK, x+1, y-1));
    set("exits/southeast", sprintf(EMASK, x+1, y+1));
    set("exits/southwest", sprintf(EMASK, x-1, y+1));
    set("exits/northwest", sprintf(EMASK, x-1, y-1));

    // Check map for blocked directions and add pre-exit blockers
    exits_map = VSERVER->query_map();
    if(sizeof(exits_map)) {
        if(exits_map[y-1][x..x] == "*")
            set("pre_exit_func/north", "@@block_exit");
        // ... check all 8 directions
    }
}

int block_exit(object tp, string dir) {
    tp->tell("You cannot go " + dir + ".\n");
    return 1;
}

int *query_xy() {
    return ({ XPos, YPos });
}

string query_short() {
    string color = element_of(({ "`<z15>`", "`<g10>`" }));
    return color + "ZONE_DISPLAY_NAME" + "`<res>`";
}

string query_long() {
    return abs_path("desc_daemon")->query_long();
}

void event_reset(object prev) {
    load_npc();
}

void load_npc() {
    if(!present("ZONE_mon") && random(100) < SPAWN_RATE) {
        object ob = new(element_of(MOB_FILES));
        ob->add("id", ({ "ZONE_mon" }));
        add_monster(ob);
    }
}
```

**Pattern 2** — `d/{domain}/{zone}/{zone}_inherit.c`:

```c
inherit STD_ROOM;

private nosave string map_daemon = abs_path("ZONE_map");
private nosave int XPos, YPos;

void area_setup() {
    set_territory("TERRITORY_NAME");
    set_outside("OUTSIDE_TYPE");
    set_terrain("TERRAIN_TYPE");

    set("short", "@@query_short");
    set("long", "@@query_long");
}

void init_virtual(int coord_x, int coord_y, int coord_z) {
    if(nullp(XPos) || nullp(YPos)) {
        XPos = coord_x;
        YPos = coord_y;
        set("coords", ({ coord_x, coord_y }));
        set("exits", map_daemon->query_exits_by_coord(coord_x, coord_y));
    }
}

int *query_xy() {
    return ({ XPos, YPos });
}

string query_short() {
    string color = element_of(({ "`<z15>`", "`<g10>`" }));
    return color + "ZONE_DISPLAY_NAME" + "`<res>`";
}

string query_long() {
    return abs_path("desc_daemon")->query_long();
}
```

### Step 5: (Optional) Create a description daemon

For varied procedural descriptions:

```c
inherit STD_DAEMON;

string query_long() {
    return element_of(({
        "DESCRIPTION_VARIANT_1\n",
        "DESCRIPTION_VARIANT_2\n",
        "DESCRIPTION_VARIANT_3\n",
    }));
}
```

### Step 6: (Optional) Add special features

Common additions:
- **Digging**: Set `dig_chance`, implement `finish_digging()` in the room template
- **Resource zones**: `set_resource_zone("zone_name")` for gathering
- **Map display**: Implement `query_area_info()` to show ASCII map centered on player
- **Pre-exit functions**: Add movement costs, daze effects, skill checks
- **Special coordinates**: Check coordinates in `init_virtual()` for unique exits or content
- **Subzones**: Create `d/{domain}/{zone}/{subzone}/virtual.c` for nested areas

## Subzone Delegation

A subzone is a virtual area nested inside another. When the parent server receives a path like `subzone/5,10`, it checks for `subzone/virtual.c` and delegates.

To create a subzone:
1. Create `d/{domain}/{zone}/{subzone}/virtual.c` inheriting `STD_VIRTUAL_SERVER`
2. The parent server's `compile_object_new()` automatically detects and delegates

No registration needed — the base class handles discovery.

## Key Methods

### STD_VIRTUAL_SERVER

| Method | Override? | Description |
|---|---|---|
| `generate_object(string path)` | **Yes** | Validate coords, return `new(template, coords)` |
| `compile_object(string file)` | No | Legacy: parses path, calls `generate_object()` |
| `compile_object_new(string path)` | No | New-style: handles subzone delegation |
| `query_virtual_rooms()` | No | Returns array of loaded virtual rooms |

### STD_VIRT_MAP

| Method | Override? | Description |
|---|---|---|
| `set_exit_mask(string mask)` | Call in `setup()` | Set sprintf format for exit paths |
| `load_map_file(string path)` | Call in `setup()` | Load ASCII map from file |
| `query_exits_by_coord(int x, int y, int z)` | No | Returns `([ "north": path, ... ])` from map symbols |
| `query_map_symbol(int x, int y, int z)` | No | Returns character at coordinate |
| `within_bounds(string coord)` | No | Check if "x,y" is valid |

### Room Template

| Method | Override? | Description |
|---|---|---|
| `init_virtual(int x, int y, int z)` | **Yes** | Store coords, call `setup_exits()` |
| `area_setup()` | **Yes** | Set territory, terrain, descriptions |
| `query_xy()` | **Yes** | Return `({ XPos, YPos })` |

## Common Pitfalls

- **Map border**: Pattern 1 maps need a border of blocking characters. Some implementations add it programmatically, others include it in the file. Be consistent.
- **Coordinate system**: Pattern 2 maps use double-spacing (room at 0,0 is map position 0,0; room at 1,1 is map position 2,2). The map daemon handles this internally.
- **init_virtual guard**: Always check `if(nullp(XPos) || nullp(YPos))` before setting up — `init_virtual` can be called multiple times.
- **Exit mask**: Must use `abs_path("%d,%d")` to produce full paths, not relative ones.
- **Monster IDs**: Give spawned monsters a zone-specific ID (e.g. `"mforest_mon"`) and check `present()` before spawning to prevent stacking.
- **Map file format**: Pattern 2 maps skip the first two lines. Make sure your map file has two header lines (even if blank).

## Validation Checklist

- [ ] `virtual.c` exists at `d/{domain}/{zone}/` and inherits `STD_VIRTUAL_SERVER`
- [ ] `generate_object()` is implemented and validates coordinates
- [ ] Map file exists and is correctly formatted for the chosen pattern
- [ ] Room template implements `init_virtual()` with coordinate guard
- [ ] Room template sets territory, terrain, and outside type
- [ ] Exits are generated correctly (all 8 directions for Pattern 1, map-based for Pattern 2)
- [ ] `query_xy()` returns coordinates for external queries
- [ ] Descriptions are procedural (use `@@query_func` or daemon) to avoid repetition
- [ ] Monster spawning checks `present()` before adding more
- [ ] If Pattern 2: map daemon exists, inherits `STD_VIRT_MAP`, calls `set_exit_mask()` and `load_map_file()`
