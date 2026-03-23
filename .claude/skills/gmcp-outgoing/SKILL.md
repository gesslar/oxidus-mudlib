---
name: gmcp-outgoing
description: Understand and extend the outgoing GMCP system for Threshold RPG. Covers the GMCP daemon, send_gmcp routing, all outgoing package modules (Char, Core, Comm, Client, External, Room, Beip), data formats, calling patterns from game code, and how to add new outgoing packages.
---

# GMCP Outgoing Skill

This skill covers the **outgoing** side of GMCP — sending data FROM the MUD TO clients. For receiving GMCP messages from clients, see the **gmcp-incoming** skill.

## Architecture Overview

```
Game code (damage, healing, buffs, chat, etc.)
       │
       ▼
GMCP_D->send_gmcp(user, package, payload)    ← adm/daemons/gmcp.c
       │
       ├─ validates user, checks GMCP enabled, checks client support
       ├─ routes to module: adm/daemons/modules/gmcp/<Package>.c
       │
       ▼
Module gathers data, calls user->gmcp_message(package, data)
       │                                         ← std/modules/gmcp.c (fulcrum)
       ├─ diffs against cache
       ├─ JSON-encodes payload
       └─ sends via telopt to client
```

## Core Files

| File | Purpose |
|---|---|
| `adm/daemons/gmcp.c` | GMCP daemon (`GMCP_D`). Routes `send_gmcp()` calls to package modules |
| `adm/daemons/modules/gmcp/Char.c` | Character data: vitals, status, stats, buffs, afflictions, progress |
| `adm/daemons/modules/gmcp/Core.c` | Protocol: Ping, Goodbye |
| `adm/daemons/modules/gmcp/Comm.c` | Communication: channel text with color parsing |
| `adm/daemons/modules/gmcp/Client.c` | Client features: GUI install, media playback |
| `adm/daemons/modules/gmcp/External.c` | External services: Discord integration |
| `adm/daemons/modules/gmcp/Room.c` | Room info (dev mode only) |
| `adm/daemons/modules/gmcp/Beip.c` | Beip client: custom vitals/stats formatting |
| `include/gmcp_defs.h` | All `GMCP_PKG_*`, `GMCP_KEY_*`, `GMCP_VAL_*` defines |

## The send_gmcp() Function

`GMCP_D->send_gmcp(object user, string package, mixed payload)` is the primary way game code triggers outgoing GMCP.

**Guards (in order):**
1. User must be a valid object
2. User must be interactive
3. User must not be a ghost
4. Package must be a valid string
5. GMCP must not be disabled (`no_gmcp` property)
6. Client info must be cached
7. Beip packages only sent to Beip clients

**Routing:**
- Parses package name into components (e.g., `"Char.Vitals"` -> package `"Char"`, subpackage `"Vitals"`)
- Loads module file: `adm/daemons/modules/gmcp/<Package>.c`
- Calls the subpackage function with appropriate arguments:
  - With command and payload: `Module->Subpackage(command, user, payload)`
  - With payload only: `Module->Subpackage(user, payload)`
  - Without payload: `Module->Subpackage(user)`
- All calls wrapped in `catch()` for safety

**Performance monitoring:** If `gmcp_debug` temp property is set on the user, logs execution time and eval cost to the `"debug"` channel.

## Two Invocation Methods

### Method 1: GMCP_D->send_gmcp() (Most Common)

Used from game code for immediate, event-driven sends:

```lpc
// No payload — module gathers its own data
GMCP_D->send_gmcp(user, GMCP_PKG_CHAR_VITALS);
GMCP_D->send_gmcp(user, GMCP_PKG_CHAR_STATUS);

// With payload
GMCP_D->send_gmcp(user, GMCP_PKG_CHAR_BUFFS_ADD, buff_data);
GMCP_D->send_gmcp(user, GMCP_PKG_COMM_CHANNEL_TEXT, message_data);
```

### Method 2: user->gmcp_message() (From Modules)

Used by the daemon modules themselves after gathering data. This is what hits the diff cache:

```lpc
// Standard send (diffs against cache)
user->gmcp_message(GMCP_PKG_CHAR_VITALS, info);

// Force full send (bypass cache)
user->gmcp_message(GMCP_PKG_CHAR_STATUS, info, 1);  // refresh=1
```

**Package remapping** happens at this layer:
- `Beip.Vitals` -> `Beip.Stats`
- `Beip.StopAttack` -> `Beip.Stats`

## Outgoing Packages

### Char.Vitals

**Module:** `Char.c::Vitals(object user)`

Sends current and max HP/SP/EP plus a formatted string.

```
{
  "hp": "150", "maxhp": "200",
  "sp": "80",  "maxsp": "100",
  "ep": "90",  "maxep": "120",
  "string": "HP: 150/200 SP: 80/100 EP: 90/120"
}
```

**Triggered by:** damage received, healing applied, SP/EP changes in combat. Sent immediately on change, not waiting for heartbeat.

### Char.Status

**Module:** `Char.c::Status(object user)`

Comprehensive character state. Sent every heartbeat (~2 seconds).

```
{
  "name", "fullname", "age", "session_login",
  "race", "gender", "heritage", "hlevel",
  "level", "glevel", "xp", "expertise", "tnl",
  "guild", "tummy", "hb", "capacity", "maxcapacity",
  "lodge", "lodge_level", "bank",
  "dead", "morality", "harmonic", "primary_axis",
  "invisible", "breath", "heal_tick",
  "inactive", "inactive_type",
  "foe_name", "foe_health", "foe_health_string", "foe_max_health", "foe_foe_name"
}
```

Special handling: when crafting, shows crafting target instead of foe. Breath is converted from float to 0-100 percentage. Foe health shows condition string (e.g., "bleeding profusely", "critically wounded").

### Char.StatusVars

**Module:** `Char.c::StatusVars(object user)`

Mapping of status key names to human-readable labels. Sent once on login so the client knows what each Status field means.

### Char.Stats

**Module:** `Char.c::Stats("List", object user)`

All character attributes with both current and permanent values:

```
{
  "strength": 18, "perm_strength": 16,
  "intelligence": 14, "perm_intelligence": 14,
  "wisdom": 12, "perm_wisdom": 12,
  "dexterity": 15, "perm_dexterity": 14,
  "constitution": 16, "perm_constitution": 15,
  "charisma": 10, "perm_charisma": 10,
  "luck": 8, "perm_luck": 8,
  "height": 72, "perm_height": 72,
  "weight": 180, "perm_weight": 180
}
```

### Char.Buffs / Char.Debuffs

**Module:** `Char.c::Buffs()` / `Char.c::Debuffs()`

Three sub-operations each:

| Package | Data |
|---|---|
| `Char.Buffs.Add` | `([ "name", "kind", "category", "what", "expires" ])` |
| `Char.Buffs.Remove` | buff_id string |
| `Char.Buffs.List` | Full mapping of all active buffs |

List gathers from: temporary buffs, special category buffs (kingdom/clan/god/admin/event), and buff objects. Each entry includes name, kind (`"temp"` or `"object"`), category, what, and expiration timestamp.

**Triggered by:** `std/living/buffs.c` whenever buffs are added/removed.

### Char.Afflictions

**Module:** `Char.c::Afflictions()`

| Package | Data |
|---|---|
| `Char.Afflictions.Add` | `({ type, expiration_time })` |
| `Char.Afflictions.Remove` | affliction type string |
| `Char.Afflictions.List` | Mapping of active stuns + handicaps (blind, mute, etc.) |

Add/Remove also triggers a Status update (afflictions affect status display).

**Triggered by:** `std/living/handicap.c` when handicaps are applied/removed.

### Char.Shielding

**Module:** `Char.c::Shielding(object user)`

Shield health as percentage of max. Returns `"-1"` if no shield active.

**Triggered by:** `std/shield.c` when shields are created/damaged/destroyed.

### Char.Feedback

**Module:** `Char.c::Feedback(object user, string *payload)`

Array of feedback message strings sent to client.

### Char.Progress

**Module:** `Char.c::Progress("List", object user)`

Style/rank progression data:

```
{
  "combat_style": {
    "Style Name": { "value": "75.5", "rank": "Expert" }
  }
}
```

For warriors: combat styles. For tradesmen: crafting styles. Rank names map from level 0-15+: Loser, Initiate, Amateur, Novice, Devotee, Veteran, Expert, Adept, Specialist, Ace, Master, Champion, Warlord, Grand Master, Grand Champion, Supreme Warlord.

### Core.Ping

**Module:** `Core.c::Ping(object user, mixed info)`

Echoes back the client's ping data for RTT measurement.

### Core.Goodbye

**Module:** `Core.c::Goodbye(object user)`

Sends `"Goodbye, adventurer."` on disconnection.

### Comm.Channel.Text

**Module:** `Comm.c::Channel("Text", object who, mixed data)`

Chat/communication messages with color code parsing:

```
{
  "channel": "say",
  "talker": "PlayerName",
  "text": "Hello everyone!"
}
```

Color codes (`%^RED%^` and `` `<color>` `` formats) are parsed into terminal-appropriate sequences based on the player's terminal type preference.

**Triggered by:** Channel daemon (`adm/daemons/channels.c`), say/tell/whisper commands, guild chat, clan chat, etc.

### Client.GUI

**Module:** `Client.c::GUI(command, object user, mapping data)`

- `"Install"`: Sends UI package URL and version for client auto-install
- `"Map"`: Sends map URL (`"https://mud.gesslar.dev/map/threshold.json"`)

Always sent regardless of client support declaration.

### Client.Media

**Module:** `Client.c::Media(command, object user, mapping data)`

- `"Play"`: Trigger media playback on client
- `"Load"`: Preload media without playing
- `"Stop"`: Stop playback

### External.Discord.Info

**Module:** `External.c::Discord("Info", object who)`

Discord integration data. Always sent regardless of support:

```
{
  "inviteurl": "https://discord.gg/0rDwGcIiYEueqon1",
  "applicationid": "857013899773345802"
}
```

### External.Discord.Status

**Module:** `External.c::Discord("Status", object who)`

Discord Rich Presence data:

```
{
  "game": "<mud_name>",
  "starttime": <login_timestamp>,
  "details": "Cross the Threshold"
}
```

### Room.Info

**Module:** `Room.c::Info(object who)`

Room data. **Only active in `__DEV_MODE__`**. Excludes houses, player rooms, and void rooms. Calls `room->gmcp_room_info(who)` for custom data.

### Beip Packages

**Module:** `Beip.c` — specialized formatting for the Beip client with visual bar displays.

**Beip.Stats** — Character info with progress bars, XP progress, lodge, KA. Uses Ansi256 color codes and hex RGB for bar rendering.

**Beip.Vitals** — HP/SP/EP/Capacity/Tummy/Heal Bank/Shield as bar data with enabled/disabled color states:
- Enabled (combat active): bright colors (green HP, blue SP, orange EP, red foe)
- Disabled (resting): muted/darker variants

**Beip.StopAttack** — Toggles vitals colors between enabled/disabled states when combat starts/stops.

Beip packages are **only sent to clients that identify as `"Beip"`** in their `Core.Hello`.

## When GMCP Gets Sent

### On Login (`std/connection.c`)

Full initialization burst:
```lpc
GMCP_D->send_gmcp(body, GMCP_PKG_CHAR_STATUS);
GMCP_D->send_gmcp(body, GMCP_PKG_CHAR_STATUSVARS);
GMCP_D->send_gmcp(body, GMCP_PKG_CHAR_BUFFS_LIST);
GMCP_D->send_gmcp(body, GMCP_PKG_CHAR_DEBUFFS_LIST);
GMCP_D->send_gmcp(body, GMCP_PKG_CHAR_AFFLICTIONS_LIST);
GMCP_D->send_gmcp(body, GMCP_PKG_CHAR_PROGRESS_LIST);
```

### On Heartbeat (~2 seconds, `std/user.c`)

```lpc
// Beip clients:
GMCP_D->send_gmcp(this_object(), GMCP_PKG_BEIP_VITALS);   // only if not inactive/busy
GMCP_D->send_gmcp(this_object(), GMCP_PKG_BEIP_STATS);

// Other clients:
GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS);   // only if not inactive/busy
GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS);
```

### On Events (Immediate)

| Event | Package | Source File |
|---|---|---|
| Damage received | `Char.Vitals` + `Beip.Vitals` | `std/body/damage.c` |
| Healing applied | `Char.Vitals` + `Beip.Vitals` | `std/body/healing.c` |
| SP/EP change | `Char.Vitals` + `Beip.Vitals` | `std/body/attack.c` |
| Buff added/removed | `Char.Buffs.Add/Remove` | `std/living/buffs.c` |
| Debuff added/removed | `Char.Debuffs.Add/Remove` | `std/living/buffs.c` |
| Handicap applied/removed | `Char.Afflictions.Add/Remove` | `std/living/handicap.c` |
| Shield change | `Char.Shielding` + `Beip.Shielding` | `std/shield.c` |
| Chat message | `Comm.Channel.Text` | `adm/daemons/channels.c` |
| Combat start/stop | `Beip.StopAttack` | Various combat files |

## Adding a New Outgoing Package

### 1. Add defines in `include/gmcp_defs.h`

```lpc
#define GMCP_PKG_CHAR_INVENTORY       "Char.Inventory"
#define GMCP_PKG_CHAR_INVENTORY_LIST  "Char.Inventory.List"
#define GMCP_KEY_CHAR_INVENTORY_ITEMS "items"
```

### 2. Add the handler in the module

In `adm/daemons/modules/gmcp/Char.c` (or a new module file):

```lpc
void Inventory(string command, object user) {
  if(command == "List") {
    mapping info = ([]);
    // ... gather inventory data ...
    user->gmcp_message(GMCP_PKG_CHAR_INVENTORY_LIST, info);
  }
}
```

### 3. Trigger from game code

```lpc
GMCP_D->send_gmcp(user, GMCP_PKG_CHAR_INVENTORY_LIST);
```

### 4. For a new top-level package

Create a new module file at `adm/daemons/modules/gmcp/<Package>.c`. The daemon's routing automatically finds it by file name.

## Calling Pattern Summary

**From game code** — use `GMCP_D->send_gmcp()`:
```lpc
GMCP_D->send_gmcp(victim, GMCP_PKG_CHAR_VITALS);
```

**From daemon modules** — use `user->gmcp_message()`:
```lpc
user->gmcp_message(GMCP_PKG_CHAR_VITALS, data_mapping);
user->gmcp_message(GMCP_PKG_CHAR_VITALS, data_mapping, 1);  // force refresh
```

**From channel/comm code** — payload passed through:
```lpc
GMCP_D->send_gmcp(who, GMCP_PKG_COMM_CHANNEL_TEXT, ([
  "channel" : "say",
  "talker"  : player_name,
  "text"    : message,
]));
```
