---
name: gmcp-incoming
description: Understand and extend the incoming GMCP system for Threshold RPG. Covers message reception, normalization, parsing, submodule routing, supported packages from clients, caching, and how to add new incoming handlers.
---

# GMCP Incoming Skill

This skill covers the **incoming** side of GMCP (Generic MUD Communication Protocol) — messages received FROM clients. For sending GMCP data TO clients, see the **gmcp-outgoing** skill.

## Architecture Overview

```
Client sends GMCP telopt
       │
       ▼
std/modules/gmcp.c          ← driver calls gmcp(string message) on user object
       │
       ├─ normalize_input()  ← capitalizes package names
       ├─ GMCP_D->gmcp_to_mapping()  ← parses into structured mapping
       │
       ▼
invoke_submodule()           ← routes to handler file
       │
       ├─ std/modules/gmcp/Core.c      ← Core.Hello, Core.Supports, Core.Ping
       ├─ std/modules/gmcp/Char.c      ← Char.Buffs.List, Char.Stats.List, etc.
       └─ std/modules/gmcp/External.c  ← External.Discord.Hello, etc.
```

## Core Files

| File | Purpose |
|---|---|
| `std/modules/gmcp.c` | Main GMCP module loaded on user objects. Entry point for all incoming messages |
| `std/modules/gmcp/Core.c` | Handles Core protocol (Hello, Supports, Ping) |
| `std/modules/gmcp/Char.c` | Handles character data requests (Buffs, Debuffs, Afflictions, Stats, Progress) |
| `std/modules/gmcp/External.c` | Handles external service messages (Discord) |
| `include/gmcp_defs.h` | All GMCP package name, key, and value defines |
| `adm/daemons/gmcp.c` | GMCP daemon — message parsing, sending, and routing |

## Message Flow

### 1. Reception

The FluffOS driver calls `protected void gmcp(string message)` on the user object when a GMCP telopt arrives. This function lives in `std/modules/gmcp.c`, which is loaded as a module on user objects via `M_GMCP`.

### 2. Normalization

`normalize_input(string input)` capitalizes package name elements:
- `"core.hello"` becomes `"Core.Hello"`
- `"char.buffs.list"` becomes `"Char.Buffs.List"`

The payload portion (after the first space) is left untouched.

### 3. Parsing

`GMCP_D->gmcp_to_mapping(message)` parses the message into a structured mapping:

```lpc
// Input: "Char.Buffs.List"
// Output:
([
  "package"    : "Char",
  "subpackage" : "Buffs",
  "command"    : "List",
  "payload"    : 0,  // or decoded JSON if present
])
```

If a payload is present (space-separated after the package name), it is JSON-decoded.

### 4. Submodule Routing

`invoke_submodule(package, message, command, data)` loads the handler file and calls the appropriate function:

- File path: `std/modules/gmcp/<Package>.c` (e.g., `std/modules/gmcp/Core.c`)
- Function called depends on the subpackage/command structure
- Wrapped in `catch()` with logging to `"gmcp"` log file on error
- Uses `seteuid()` protection for security

## Supported Incoming Packages

### Core Protocol

**Handler:** `std/modules/gmcp/Core.c`

| Package | Function Called | Data Expected | Action |
|---|---|---|---|
| `Core.Hello` | `Hello(mapping info)` | `([ "client": "Name", "version": "1.0" ])` | Stores client identification. Only accepts from CONNECTION objects |
| `Core.Supports.Set` | `Supports("Set", string *info)` | `({ "Char 1", "Room 1" })` | Replaces client's supported package list |
| `Core.Supports.Add` | `Supports("Add", string *info)` | `({ "Beip 1" })` | Adds to supported packages (deduplicates) |
| `Core.Supports.Remove` | `Supports("Remove", string *info)` | `({ "Room 1" })` | Removes from supported packages |
| `Core.Ping` | `Ping(int info)` | Integer timestamp | Echoes back via `GMCP_D->send_gmcp()` |

### Character Data Requests

**Handler:** `std/modules/gmcp/Char.c`

| Package | Function Called | Action |
|---|---|---|
| `Char.Reset` | `Reset()` | Clears GMCP cache and resends all character data |
| `Char.Buffs.List` | `Buffs("List")` | Sends current buff list to client |
| `Char.Debuffs.List` | `Debuffs("List")` | Sends current debuff list to client |
| `Char.Afflictions.List` | `Afflictions("List")` | Sends current affliction list to client |
| `Char.Stats.List` | `Stats("List")` | Sends character stats to client |
| `Char.Progress.List` | `Progress("List")` | Sends progression info (styles/crafting) to client |

These are all "pull" requests — the client asks for data and the handler responds by calling the appropriate outgoing GMCP function.

### External Services

**Handler:** `std/modules/gmcp/External.c`

| Package | Function Called | Action |
|---|---|---|
| `External.Discord.Hello` | `Discord("Hello")` | Sends Discord info (invite URL, app ID) and current status |
| `External.Discord.Get` | `Discord("Get")` | Sends updated Discord play status |

## Client Support Tracking

The module tracks what GMCP packages each client supports:

```lpc
// Check if client supports a package
int supported = gmcp_supports("Char");     // returns 1 or 0
int supported = gmcp_supports("Beip");     // Beip-specific check
```

**Core packages are always supported** — Core, Core.Ping, Core.Hello, Core.Goodbye never need a support check.

**Some packages always send regardless of support:**
- `Client.GUI`
- `External.Discord.Status`
- `External.Discord.Info`

## Caching and Differential Updates

The incoming module manages a cache (`gmcp_cache`) for outgoing responses to minimize bandwidth:

```lpc
// gmcp_diff() compares cached vs new values
// Only changed fields are sent to the client
mapping diff = gmcp_diff(package, new_data);
```

- `gmcp_cache` stores previously sent values per package
- `gmcp_diff()` returns only the keys that have changed
- A `refresh` flag can force sending all data even if unchanged
- **Grapevine client exception**: always receives full `Char.Vitals` data (no diffing)

To clear the cache and force a full resend, clients send `Char.Reset`.

## Client Identification

After `Core.Hello`, client info is stored and accessible:

- Client name and version are normalized (keys lowercased)
- Stored via `set_gmcp_client(info)` on the connection object
- Used for client-specific behavior (e.g., Beip packages only sent to Beip clients)

## Disabling GMCP

Players can disable GMCP with `set_property("no_gmcp", 1)`. Ghost players also don't receive GMCP.

## Adding a New Incoming Handler

### Adding to an existing package

Add a new function to the appropriate handler file. For example, to handle `Char.Inventory.List`:

```lpc
// In std/modules/gmcp/Char.c

void Inventory(string command) {
  object who = this_object();

  if(command == "List") {
    // Gather inventory data
    // Send response via GMCP_D
    GMCP_D->send_gmcp(who, GMCP_PKG_CHAR_INVENTORY_LIST, inventory_data);
  }
}
```

You'll also need to add the package define in `include/gmcp_defs.h`:

```lpc
#define GMCP_PKG_CHAR_INVENTORY       "Char.Inventory"
#define GMCP_PKG_CHAR_INVENTORY_LIST  "Char.Inventory.List"
```

### Adding a new top-level package

1. Create the handler file at `std/modules/gmcp/<Package>.c`
2. Add package defines in `include/gmcp_defs.h`
3. The routing in `invoke_submodule()` will automatically find it by file name

### Handler function signatures

The submodule router calls functions based on the message structure:

- `Core.Hello` → calls `Hello(payload)` with the parsed data
- `Core.Supports.Set` → calls `Supports("Set", payload)` with subpackage as first arg
- `Char.Buffs.List` → calls `Buffs("List")` with command as arg
- `External.Discord.Hello` → calls `Discord("Hello")` with command as arg

The pattern is: the **subpackage** name becomes the function name. If there's a command, it's passed as the first argument. Payload data follows.

## GMCP Defines Reference

All defines live in `include/gmcp_defs.h`. The naming convention:

| Pattern | Example | Purpose |
|---|---|---|
| `GMCP_PKG_*` | `GMCP_PKG_CHAR_VITALS` | Package name strings |
| `GMCP_KEY_*` | `GMCP_KEY_CHAR_VITALS_HP` | Data key names |
| `GMCP_VAL_*` | `GMCP_VAL_CHAR_STATUS_DEAD` | Predefined values |

The `GMCP_D` daemon define is in `include/daemons.h`. The module directory path `DIR_STD_SUBMODULE_GMCP` is in `include/dirs.h`.
