---
name: channel-system
description: Understand and extend the channel communication system for Oxidus. Covers the channel daemon (CHAN_D), module architecture, channel registration, tuning, message flow, permissions, history, GMCP integration, the channel command, and how to create new channel modules.
---

# Channel System Skill

You are helping create or modify the channel communication system for Oxidus. Channels are persistent chat systems organized into "modules" (networks) that provide topic-specific discussion areas. Follow the `lpc-coding-style` skill for all formatting.

## Architecture Overview

```
Player types channel name as verb (e.g., "gossip hello")
       |
       v
command_hook() in std/object/command.c
       |
       v
CHAN_D->chat(verb, user, arg)          <- adm/daemons/channel.c
       |
       +- validates channel exists, module loaded
       +- checks user is in listener list
       +- checks module->is_allowed()
       |
       v
module->rec_msg(channel, user, msg)    <- module formats & routes
       |
       v
CHAN_D->rec_msg(channel, user, msg)    <- broadcasts to all listeners
       |
       +- tell(listener, msg)          <- text to each listener
       +- GMCP_D->send_gmcp(listener, GMCP_PKG_COMM_CHANNEL_TEXT, payload)
```

## Core Files

| File | Macro | Purpose |
|---|---|---|
| `adm/daemons/channel.c` | `CHAN_D` | Central channel daemon — registration, tuning, routing, broadcasting |
| `adm/daemons/modules/channel/channel.c` | `D_MOD_CHANNEL` | Base inheritable for channel modules |
| `cmds/std/channel.c` | `CMD_CHANNEL` | Player `channel` command (list, tune, show, tuned) |
| `std/object/command.c` | — | Command hook that routes channel verbs via `CHAN_D->chat()` |
| `std/living/player.c` | — | Auto-tunes channels on `enter_world()` |
| `include/daemons.h` | — | Defines `CHAN_D` |
| `include/modules_daemon.h` | — | Defines `D_MOD_CHANNEL` |
| `include/gmcp_defines.h` | — | Defines `GMCP_PKG_COMM_CHANNEL_TEXT` |
| `include/signal.h` | — | Defines `SIG_CHANNEL_MESSAGE` |

## Channel Daemon (CHAN_D)

`adm/daemons/channel.c` is the central coordinator. It maintains two internal mappings:

```lpc
private nosave mapping channels;  // channel_name -> ([ "module", "real_name", "listeners" ])
private nosave mapping modules;   // module_name -> file_path
```

### Key Functions

| Function | Purpose |
|---|---|
| `register_module(name, path)` | Register a network module. Returns 1 on success, -1 if name taken by different path |
| `register_channel(module_name, channel_name)` | Register a channel under a module. If name conflicts with another module, prefixes with first 4 chars of module name |
| `unregister_module(module_name)` | Remove module and all its channels. Only callable by the module itself |
| `remove_channel(channel_name)` | Remove a single channel |
| `tune(channel, user, flag)` | Tune user in (flag=1) or out (flag=0). Checks `module->is_allowed()` first |
| `chat(channel, user, msg)` | Entry point from command hook. Validates and routes to `module->rec_msg()` |
| `rec_msg(channel, user, msg)` | Broadcast formatted message to all listeners via `tell()` and GMCP |
| `valid_channel(name)` | Returns 1 if channel exists |
| `valid_module(name)` | Returns 1 if module exists |
| `get_channels(module_name)` | Get channel names for a module (use `"all"` for all modules) |
| `get_modules()` | Get all registered module names |
| `get_tuned(channel)` | Get listener names for a channel (filters out non-interactive) |
| `grapevine_chat(payload)` | Handle incoming Grapevine messages, routes to `module->rec_grapevine_msg()` |

### Message Broadcasting (rec_msg)

When `CHAN_D->rec_msg()` is called, it:
1. Validates channel and module exist
2. Builds a GMCP payload: `([ "channel": channel, "talker": user, "text": msg ])`
3. For each listener, finds the living object via `find_living()`
4. Sends text via `tell(ob, msg)` and GMCP via `GMCP_D->send_gmcp(ob, GMCP_PKG_COMM_CHANNEL_TEXT, payload)`
5. Removes dead references from the listener list

### Chat Flow (chat)

When `CHAN_D->chat()` is called from the command hook:
1. Checks channel exists in the channels mapping
2. Validates the module is still loaded
3. Checks user is in the listener list (must be tuned in)
4. Checks `module->is_allowed(real_name, user)`
5. If no message provided, sets `notify_fail("Syntax: <channel> <msg>\n")`
6. Calls `module->rec_msg(real_name, user, msg)` — the module is responsible for formatting and calling `CHAN_D->rec_msg()`

## Base Module Inheritable (D_MOD_CHANNEL)

`adm/daemons/modules/channel/channel.c` provides base functionality for channel modules.

### Properties

```lpc
protected nosave string module_name;    // Auto-set to query_file_name(this_object())
protected nosave string *channel_names; // Set by subclass in setup() — channels this module provides
private mapping history;                // Per-channel message history
```

### Lifecycle

1. `mudlib_setup()` — Sets persistent, no-clean, registers signal slot for `SIG_CHANNEL_MESSAGE`
2. `post_setup_1()` — Calls `CHAN_D->register_module()` and `CHAN_D->register_channel()` for each channel in `channel_names`, initializes history arrays

### Key Functions

| Function | Purpose |
|---|---|
| `add_history(channel, message)` | Append message to per-channel history |
| `last_messages(channel, num_lines)` | Return last N messages (default 10, clamped 10-100) |
| `incomingTransmission(chan, usr, msg)` | Signal handler for `SIG_CHANNEL_MESSAGE`. Parses `/command` or `:command` prefixes, dispatches to `commandXxx()` methods, or falls through to `rec_msg()` |
| `commandLast(who, chan, arg)` | Built-in `/last` command handler — returns recent history |
| `is_allowed(channel, usr, flag)` | Permission check — must be overridden by subclass |

### Command Dispatch in incomingTransmission

Messages starting with `/` or `:` are treated as module commands:
- Extracts command name, looks for `command<Capitalized>()` method on the module
- If found, calls it with `(chan, usr, arg)`
- If not found, falls through to regular message handling
- Regular messages get recorded in history and forwarded to `rec_msg()`

## Installed Modules

Modules are listed in `adm/daemons/modules/channel/installed_modules` (one path per line, `.c` extension optional). Loaded sequentially by `CHAN_D->setup()`.

### Local Module (`local.c`)

Inherits `D_MOD_CHANNEL`. Currently in a transitional state — `rec_msg()` has a command dispatch prefix but most of the message formatting/routing code is commented out. Does not set `channel_names` explicitly.

**Permissions (`is_allowed`):**
- `"admin"` channel requires `adminp(usr)`
- `"wiz"` and `"dev"` channels require `devp(usr)`
- All other channels are open to everyone

### Herald/Announce Module (`herald.c`)

Inherits `D_MOD_CHANNEL` (via `__DIR__ "channel"`). Provides the `"announce"` channel.

```lpc
channel_names = ({ module_name });  // "announce" (derived from file name)
```

**Signal listeners:**
- `SIG_USER_LOGIN` -> `heraldArrival()` — broadcasts login announcement
- `SIG_USER_LOGOUT` -> `heraldDeparture()` — broadcasts logout announcement
- `SIG_SYS_CRAWL_COMPLETE` -> `announce_crawl_complete()` — broadcasts crawl completion

**Permissions:** Only admins can send manual messages. Everyone receives announcements.

**Message format:** `[Announce] System: PlayerName has logged into MudName.`

### Error Module (`error.c`)

Inherits `STD_DAEMON` directly (not `D_MOD_CHANNEL`). Provides the `"error"` channel for system error logging.

**Registration:** Does its own `CHAN_D->register_module()` and `register_channel()` in `setup()`.

**Permissions:** Requires `adminp(usr)` to send messages, `devp(usr)` or master privs to tune in.

### Grapevine Module (`grapevine.c`)

Inherits `STD_DAEMON` directly. Provides intermud chat channels via the Grapevine WebSocket protocol.

**Channel list:** From `mudConfig("GRAPEVINE")` — combines `channels` and `local_only` arrays from config.

**Registration:** Does its own `CHAN_D->register_module()` and `register_channel()` in `post_setup_1()`.

**Features:**
- Persistent history (saved via `saveData()`)
- Emote support: messages starting with `:` are formatted as emotes
- `/last`, `/last N`, `/all` commands for history viewing
- Messages on non-local channels are forwarded to `GRAPEVINE_D->grapevine_broadcast_message()`
- Incoming Grapevine messages handled via `rec_grapevine_msg()` — formats as `user@game`

**Permissions:** Open to everyone (`is_allowed` always returns 1).

## The Channel Command (CMD_CHANNEL)

`cmds/std/channel.c` provides the player-facing `channel` command.

### Subcommands

| Syntax | Action |
|---|---|
| `channel` (no args) | Show channels you're tuned into |
| `channel list` | List all available channels grouped by module |
| `channel show <name>` | Show who is tuned into a channel |
| `channel tune in <channel\|all>` | Tune into a channel or all channels |
| `channel tune out <channel\|all>` | Tune out of a channel or all channels |
| `channel tuned [player]` | Show what channels a player is tuned into (wizards can check others) |

### The tune() Function

`CMD_CHANNEL->tune(channel, name, in_out, silent)` is also called programmatically:
- From `player.c::enter_world()` for auto-tuning on login
- `channel` can be `"all"` to tune all available channels
- `silent` flag suppresses output messages
- Returns 1 on success, 0 on failure

## Player Integration

### Auto-Tune on Login

In `std/living/player.c::enter_world()`:

```lpc
ch = explode(query_pref("auto_tune"), " ");
foreach(string channel in ch)
  CMD_CHANNEL->tune(channel, query_privs(this_object()), 1, 1);
```

The `auto_tune` preference defaults to `"all"` (set in `setup_body()`). Players can set it to specific space-separated channel names.

### Sending Messages

Players type the channel name as a verb:
```
gossip Hello everyone!
chat :waves
```

The command hook in `std/object/command.c` (around line 310) catches this:
```lpc
if(CHAN_D->chat(verb, query_privs(), arg))
  return 1;
```

## Creating a New Channel Module

### 1. Create the module file

```lpc
// /adm/daemons/modules/channel/mymodule.c

inherit D_MOD_CHANNEL;

void setup() {
  channel_names = ({ "mychannel", "anotherchannel" });
}

int rec_msg(string chan, string usr, string msg) {
  string real_message;

  // Emote support
  if(msg[0..0] == ":") {
    msg = msg[1..<1];
    real_message = sprintf(" %s", msg);
  } else {
    real_message = sprintf(": %s", msg);
  }

  // Format and broadcast
  CHAN_D->rec_msg(chan, lower_case(usr),
    sprintf("[%s] %s%s\n", capitalize(chan), capitalize(usr), real_message));

  // Record history
  add_history(chan, sprintf("%s %s [%s] %s%s\n",
    ldate(time(), 1), ltime(),
    capitalize(chan), capitalize(usr), real_message));

  return 1;
}

int is_allowed(string channel, string usr, int flag) {
  // Return 1 to allow, 0 to deny
  // Check adminp(), devp(), etc. as needed
  return 1;
}
```

### 2. Register in installed_modules

Add the module path to `adm/daemons/modules/channel/installed_modules`:

```
/adm/daemons/modules/channel/mymodule
```

### 3. Module registration happens automatically

The base `D_MOD_CHANNEL` handles registration in `post_setup_1()`:
- Calls `CHAN_D->register_module(module_name, file_name())` using the file name as module name
- Registers each channel in `channel_names`
- If a channel name conflicts with another module's channel, it gets prefixed (first 4 chars of module name)

### Alternative: Direct STD_DAEMON approach

For modules that need more control (like error.c or grapevine.c), inherit `STD_DAEMON` directly and handle registration manually:

```lpc
inherit STD_DAEMON;

private nosave string module_name = query_file_name(this_object());
private nosave string *channels = ({ "mychannel" });

void setup() {
  set_no_clean(1);
  CHAN_D->register_module(module_name, file_name());
  filter(channels, (: CHAN_D->register_channel($(module_name), $1) :));
}
```

This approach requires you to implement all functionality yourself (history, persistence, etc.).

## Message Format Convention

All modules follow this format for displayed messages:

```
[ChannelName] Username: message text
[ChannelName] Username emote text       (for emotes with : prefix)
```

With history entries prepending timestamp:
```
2024-07-18 14:30:00 [ChannelName] Username: message text
```

## GMCP Integration

Every message broadcast through `CHAN_D->rec_msg()` automatically sends GMCP:

```lpc
// Package: Comm.Channel.Text
// Payload:
([
  "channel" : "gossip",
  "talker"  : "playername",
  "text"    : "[Gossip] Playername: hello!\n",
])
```

## Signal Integration

Channel modules can listen for system signals via `slot()`:

| Signal | Use Case |
|---|---|
| `SIG_CHANNEL_MESSAGE` | Base module routes to `incomingTransmission()` |
| `SIG_USER_LOGIN` | Herald announces arrivals |
| `SIG_USER_LOGOUT` | Herald announces departures |
| `SIG_SYS_CRAWL_COMPLETE` | Herald announces system events |

## Key Design Points

- **Channel names are verbs**: Players type the channel name directly as a command. The command hook tries `CHAN_D->chat()` after soul emotes but before PATH-based command lookup.
- **Modules own formatting**: The daemon broadcasts raw formatted strings. Each module's `rec_msg()` is responsible for formatting messages before calling `CHAN_D->rec_msg()`.
- **Permission is per-module**: `is_allowed(channel, usr, flag)` is called on the module for both tuning and chatting. The `flag` parameter is present for tune operations (1=in, 0=out) but absent for chat checks.
- **Listener cleanup is automatic**: `CHAN_D->rec_msg()` removes listeners whose living objects can't be found.
- **History is per-module**: Each module manages its own history. The base `D_MOD_CHANNEL` provides `add_history()` and `last_messages()`. Modules inheriting `STD_DAEMON` directly must manage their own.
