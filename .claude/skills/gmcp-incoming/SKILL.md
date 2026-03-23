---
name: gmcp-incoming
description: Understand and extend the incoming GMCP system for Oxidus. Covers message reception, parsing via ClassGMCP, submodule routing, supported packages from clients, cooldown limiting, and how to add new incoming handlers.
---

# GMCP Incoming Skill

This skill covers the **incoming** side of GMCP (Generic MUD Communication Protocol) — messages received FROM clients. For sending GMCP data TO clients, see the **gmcp-outgoing** skill.

## Architecture Overview

```
Client sends GMCP telopt
       │
       ▼
std/modules/gmcp.c::gmcp()     ← driver calls this on the user/login object
       │
       ├─ GMCP_D->convert_message()  ← parses into ClassGMCP
       │
       ▼
call_other() to handler         ← routes to handler file by package name
       │
       ├─ std/modules/gmcp/Core.c      ← Core.Hello, Core.Supports.*, Core.Ping
       ├─ std/modules/gmcp/Char.c      ← Char.Login.Credentials, Char.Items.*
       └─ std/modules/gmcp/External.c  ← External.Discord.*
```

## Core Files

| File | Purpose |
|---|---|
| `std/modules/gmcp.c` | Main GMCP module (`M_GMCP`), inherited by player and login objects. Entry point for all incoming messages, also provides `do_gmcp()` for sending |
| `std/modules/gmcp/gmcp_module.c` | Base class for handler modules. Provides cooldown system. Inherits `STD_DAEMON` |
| `std/modules/gmcp/Core.c` | Handles Core protocol (Hello, Supports, Ping) |
| `std/modules/gmcp/Char.c` | Handles Char.Login.Credentials and Char.Items requests |
| `std/modules/gmcp/External.c` | Handles External.Discord messages |
| `std/classes/gmcp.c` | Defines `ClassGMCP` structure |
| `include/gmcp_defines.h` | All GMCP package name, key, and value defines |
| `adm/daemons/gmcp.c` | GMCP daemon — message parsing and outgoing routing |

## Message Flow

### 1. Reception

The FluffOS driver calls `void gmcp(string message)` on the user/login object when a GMCP telopt arrives. This function lives in `std/modules/gmcp.c` (`M_GMCP`), inherited by both `std/living/player.c` and `adm/obj/login.c`.

The module does **not** check if the player has GMCP enabled at this stage — this ensures `Core.Hello` and `Core.Supports` are still processed during login.

### 2. Parsing

`GMCP_D->convert_message(message)` parses the message into a `ClassGMCP` object:

```lpc
class ClassGMCP {
    string name;       // Full message (e.g., "Core.Supports.Set")
    string package;    // First component (e.g., "Core")
    string module;     // Second component (e.g., "Supports")
    string submodule;  // Third component if present (e.g., "Set")
    mixed payload;     // JSON-decoded data, or null
}
```

### 3. Routing

The handler file is loaded from `std/modules/gmcp/<Package>.c` based on the package name. Then:

- **No submodule:** `call_other(ob, module, payload)`
- **With submodule:** `call_other(ob, module, submodule, payload)`

Examples:
- `Core.Hello {"client":"Mudlet"}` → `Core.c::Hello(payload)`
- `Core.Supports.Set [...]` → `Core.c::Supports("Set", payload)`
- `Char.Login.Credentials {...}` → `Char.c::Login("Credentials", payload)`
- `Char.Items.Inv` → `Char.c::Items("Inv", null)`

If the handler file doesn't exist, the error is logged to `system/gmcp`.

## Handler Modules

All handler modules inherit `gmcp_module.c` (which inherits `STD_DAEMON`):

```lpc
inherit __DIR__ "gmcp_module";
```

This provides the cooldown system (see below).

## Supported Incoming Packages

### Core Protocol (`std/modules/gmcp/Core.c`)

| Message | Function | Payload | Action |
|---|---|---|---|
| `Core.Hello` | `Hello(mapping)` | `([ "client": "Name", "version": "1.0" ])` | Stores client info. Only accepts from `LOGIN_OB` |
| `Core.Supports.Set` | `Supports("Set", string*)` | `({ "Char 1", "Room 1" })` | Replaces supported packages list |
| `Core.Supports.Add` | `Supports("Add", mixed)` | `({ "Beip 1" })` or `"Beip 1"` | Adds to supported packages |
| `Core.Supports.Remove` | `Supports("Remove", mixed)` | `({ "Room 1" })` or `"Room 1"` | Removes from supported packages |
| `Core.Ping` | `Ping(int)` | Integer timestamp | Echoes back via `GMCP_D->send_gmcp()`. Cooldown: 60 seconds |

### Character Package (`std/modules/gmcp/Char.c`)

| Message | Function | Payload | Action |
|---|---|---|---|
| `Char.Login.Credentials` | `Login("Credentials", mapping)` | `([ "account": "char@account", "password": "..." ])` | Authenticates user, sends `Char.Login.Result` |
| `Char.Items.Contents` | `Items("Contents", string)` | Container target ID | Sends container contents list |
| `Char.Items.Inv` | `Items("Inv", null)` | None | Sends inventory list |
| `Char.Items.Room` | `Items("Room", null)` | None | Sends room items list |

### External Package (`std/modules/gmcp/External.c`)

| Message | Function | Action |
|---|---|---|
| `External.Discord.Hello` | `Discord("Hello", null)` | Sends Discord info to client |
| `External.Discord.Get` | `Discord("Get", null)` | Sends Discord status to client |

## Client Support Tracking

The module tracks what GMCP packages each client supports in a hierarchical mapping structure:

```lpc
// Check if a package/module/submodule is supported
int supported = prev->query_gmcp_supported("Char.Items");

// Get full supports data
mapping supports = prev->query_gmcp_supports();
```

The supports hierarchy uses nested mappings with `"modules"` and `"submodules"` keys, each with a `"version"` field.

## State Functions on M_GMCP

| Function | Purpose |
|---|---|
| `set_gmcp_client(mapping)` | Store client identification from Core.Hello |
| `query_gmcp_client()` | Retrieve stored client info |
| `set_gmcp_supports(mapping)` | Store client capabilities |
| `query_gmcp_supports()` | Retrieve client capabilities |
| `query_gmcp_supported(string)` | Check if a specific package path is supported |
| `gmcp_enabled()` | Check if GMCP is enabled (has GMCP + pref not "off") |
| `do_gmcp(package, data)` | Send a GMCP message to the client (JSON-encodes data) |
| `clear_gmcp_data()` | Reset all stored GMCP data |

## Cooldown System

Handler modules can rate-limit incoming requests:

```lpc
// In setup():
cooldown_limits = ([
    GMCP_PKG_CORE_PING : 60,  // 60-second cooldown
]);

// In handler function:
void Ping(int time) {
    object prev = previous_object();

    if(!cooldown_check(GMCP_PKG_CORE_PING, prev))
        return;

    apply_cooldown(GMCP_PKG_CORE_PING, prev);

    // ... handle request
}
```

Cooldowns are tracked per-player using `query_privs()` as the key.

## Disabling GMCP

Players can disable GMCP by setting the `"gmcp"` preference to `"off"`. The `gmcp_enabled()` function checks both that the connection has GMCP negotiated (`has_gmcp()`) and the preference is not off. Login objects always report GMCP as enabled.

## Adding a New Incoming Handler

### Adding to an existing package

Add a new function to the handler file. For example, to handle `Char.Skills.List`:

```lpc
// In std/modules/gmcp/Char.c

void Skills(string submodule, mixed data) {
    object prev = previous_object();

    switch(submodule) {
        case "List":
            GMCP_D->send_gmcp(prev, GMCP_PKG_CHAR_SKILLS_LIST);
            break;
    }
}
```

Add the package define in `include/gmcp_defines.h`:

```lpc
#define GMCP_PKG_CHAR_SKILLS_LIST "Char.Skills.List"
```

### Adding a new top-level package

1. Create `std/modules/gmcp/<Package>.c`
2. Inherit the base module: `inherit __DIR__ "gmcp_module";`
3. Add package defines in `include/gmcp_defines.h`
4. The routing in `gmcp()` will automatically find it by file name

```lpc
// std/modules/gmcp/Custom.c
#include <daemons.h>
#include <gmcp_defines.h>

inherit __DIR__ "gmcp_module";

void Status(string submodule, mixed data) {
    object prev = previous_object();

    switch(submodule) {
        case "Request":
            GMCP_D->send_gmcp(prev, GMCP_PKG_CUSTOM_STATUS, status_data);
            break;
    }
}
```

## GMCP Defines Reference

All defines live in `include/gmcp_defines.h`. Naming convention:

| Pattern | Example | Purpose |
|---|---|---|
| `GMCP_PKG_*` | `GMCP_PKG_CORE_PING` | Package name strings |
| `GMCP_KEY_*` | `GMCP_KEY_CHAR_VITALS_HP` | Data key names |
| `GMCP_VAL_*` | `GMCP_VAL_CHAR_STATUS_DEAD` | Predefined values |
| `GMCP_LIST_*` | `GMCP_LIST_INV`, `GMCP_LIST_ROOM` | List target constants |

The `GMCP_D` daemon define is in `include/daemons.h`. The module directory path `DIR_STD_MODULES` is used to locate handler files.
