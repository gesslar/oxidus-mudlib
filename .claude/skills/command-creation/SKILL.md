---
name: command-creation
description: Create and modify standard commands for Threshold RPG. Covers file naming, inheritance, entry points (main/cmd_verb/perform), return value handling, check functions, delayed actions, help text, and command directory structure.
---

# Command Creation Skill

You are helping create or modify commands for Threshold RPG. Commands are LPC files that players invoke by typing a verb. Follow the `lpc-coding-style` skill for all formatting.

## How Commands Work

1. Player types a verb (e.g. `attack goblin`)
2. `user.c::cmd_hook()` searches for a matching command file via `CMD_D->find_cmd()`
3. CMD_D looks through the player's PATH directories for `_<verb>.c`
4. The command object is loaded and one of its entry point functions is called
5. The return value is evaluated by `evaluate_result()` to determine success/failure

## File Naming and Location

- Commands are named `_<verb>.c` (underscore prefix, lowercase)
- Standard player commands go in `cmds/std/`
- Other directories: `cmds/wiz/` (wizard), `cmds/adm/` (admin), `cmds/xtra/`, `cmds/stea/`, `cmds/ghost/`, `cmds/clan/`, `cmds/religion/`, `cmds/race/`, `cmds/mount/`, `cmds/spells/`, and others

Path defines in `include/config.h`:

```lpc
#define USER_CMDS "/cmds/std"
#define WIZ_CMDS  "/cmds/wiz"
#define ADM_CMDS  "/cmds/adm"
```

## Inheritance

All commands inherit `STD_CMD` (defined as `std/cmd` in `include/mudlib.h`):

```lpc
inherit STD_CMD;
```

`STD_CMD` provides:
- **M_CHECKS** - validation check functions (see below)
- **M_CASTING** - delayed action system (`delay_cast()`, `set_cooldown()`)
- **M_SUBSTITUTION** - text substitution helpers
- **M_COLOR** - color utilities
- **STD_PROPERTIES** - property get/set system
- Setup chain: `mudlib_setup()` -> `base_setup()` -> `pre_setup()` -> `setup()` -> `post_setup()`
- `query_help(object tp)` stub for help text

Some commands inherit additional modules as needed:

```lpc
inherit STD_CMD;
inherit M_WIDGETS;  // for UI widgets
```

## Entry Point Functions

`cmd_hook()` tries these entry points in order on the command object:

1. `perform(object caller, string arg)` - dispatcher-style commands with sub-commands
2. `execute(object caller, object env, string arg)` - includes environment
3. `main(object tp, string str)` - modern standard entry point
4. `cmd_<verb>(string arg)` - legacy style, uses `this_player()` internally

### Preferred: `main()` (modern style)

```lpc
inherit STD_CMD;

mixed main(object tp, string str) {
  if(!str)
    return notify_fail("Do what?\n");

  // ... command logic ...

  return 1;
}
```

### Alternative: `cmd_<verb>()` (legacy style)

```lpc
inherit STD_CMD;

int cmd_drop(string str) {
  object tp = this_player();

  if(!str)
    return notify_fail("Drop what?\n");

  // ... command logic ...

  return 1;
}
```

### Alternative: `perform()` (dispatcher style)

Best for commands with multiple sub-commands (e.g. `settings`, `account`):

```lpc
inherit STD_CMD;

int perform(object caller, string arg) {
  string type, args;

  if(!arg) {
    show_usage(caller);
    return 1;
  }

  if(sscanf(arg, "%s %s", type, args) != 2)
    type = arg;

  switch(type) {
    case "option1": return handle_option1(caller, args);
    case "option2": return handle_option2(caller, args);
    default: show_usage(caller); return 1;
  }
}
```

## Return Value Handling

The return value from a command entry point is processed by `evaluate_result()`:

| Return Value | Effect |
|---|---|
| `1` | Command succeeded, stop processing |
| `0` | Command failed, continue searching other handlers |
| `string` (non-empty) | Display message to player (newline appended if missing), treat as success |
| `string` (empty `""`) | Treat as failure |
| `string *` (non-empty array) | Display via `more()` pager, treat as success |
| `string *` (empty array) | Treat as failure |

### Using return values effectively

```lpc
// Return a string for simple feedback (auto-displayed, auto-newlined)
return "You can't do that while sleeping.\n";

// Return 1 after manually handling output
tp->tell("You swing the sword.\n");
return 1;

// Use notify_fail for "soft" failures that let other handlers try
return notify_fail("There is nothing to swing here.\n");
```

**Key distinction**: returning a string = success (message shown, processing stops). Using `notify_fail()` + returning 0 = failure (message shown only if no other handler succeeds).

## Check Functions (M_CHECKS)

All check functions return `TRUE` on pass, `FALSE` on fail (with automatic error message to player). Use them as guards at the top of your command:

```lpc
if(!ep_check(tp, 10)) return 1;
if(!casting_check(tp)) return 1;
if(!attacking_check(tp)) return 1;
```

### Available checks

| Function | Purpose |
|---|---|
| `player_check(tp)` | Is `tp` a real user (not NPC)? |
| `level_check(tp, level)` | Player level >= `level`? |
| `glevel_check(tp, glevel)` | Guild level >= `glevel`? |
| `hp_check(tp, amount, args)` | Has enough hit points? |
| `sp_check(tp, amount, args)` | Has enough spell points? |
| `ep_check(tp, amount, args)` | Has enough endurance points? |
| `cooldown_check(tp, label, args)` | Cooldown expired? Label auto-prefixed with `cooldown/` |
| `casting_check(tp, args)` | Not currently casting/busy? |
| `attacking_check(tp, args)` | Not currently in combat? |
| `attacking_required_check(tp, args)` | Must be in combat? |
| `bank_balance_check(tp, amount, args)` | Enough coins in bank? |
| `services_check(tp, args)` | Room allows services? |
| `no_mounted_check(tp, args)` | Not mounted? |
| `handicap_check(tp, handicaps)` | Not affected by named handicap(s)? |
| `guild_check(tp, guilds)` | Member of specified guild(s)? Accepts string, array, or mapping (with glevel) |
| `race_check(tp, races)` | Is specified race? Accepts string, array, or mapping (with glevel) |
| `lodge_check(tp, lodges)` | Member of specified lodge? |
| `religion_check(tp, religions)` | Follows specified religion? |
| `kingdom_check(tp, kingdoms)` | Member of specified kingdom? |
| `same_room_check(one, two)` | Two objects in same room? |
| `no_teleport_check(tp)` | Room allows teleportation? |
| `absolutely_no_teleport_check(tp)` | Room allows any teleportation? |
| `register_check(tp, number)` | Player citizenship/registration level? |

The `args` parameter on many checks controls the failure message:
- Omitted or positive int: show default message
- String: show custom message
- `0`: suppress message

## Delayed Actions (M_CASTING)

For commands that take time (digging, crafting, attack stop):

```lpc
// Start a delayed action
// delay_cast(label, callback_func, delay_seconds, args...)
delay_cast("digging", "finish_dig", 15, tp, room, tool);
return 1;

// The callback receives the args plus a serial ID
void finish_dig(object tp, object room, object tool, int serial) {
  // Perform the delayed action
  tp->tell("You finish digging.\n");
}
```

Set cooldowns:

```lpc
// set_cooldown(tp, label, duration_seconds)
set_cooldown(tp, "dig", 30);
```

## Help Text

Implement `query_help()` to provide in-game help when a player types `help <command>`:

```lpc
string query_help(object tp) {
  return @text
USAGE: commandname <argument>

Description of what the command does and how to use it.
text
;
}
```

For dynamic help text based on player state:

```lpc
string query_help(object tp) {
  string mess = @text
USAGE: dig <location> with <tool>
       dig here with hands

Dig in the ground to find buried items.
text
;

  if(tp->query("race") == "canis") {
    mess += @text

As a canis, you dig faster than other races.
text
;
  }

  return mess;
}
```

You can also forward-declare `query_help` and call it within the command for invalid input:

```lpc
string query_help(object tp);

mixed main(object tp, string str) {
  if(!str) {
    tp->tell(query_help(tp));
    return 1;
  }
  // ...
}
```

Or redirect to the help system:

```lpc
if(!str) {
  tp->force_me("help attack");
  return 1;
}
```

## Common Patterns

### Pet prevention

```lpc
if(tp->prevent_pet())
  return 1;
```

### Resource cost with check

```lpc
if(!ep_check(tp, 10))
  return 1;
// ... later, on success:
tp->delta_ep(-10);
```

### Finding objects in inventory or room

```lpc
// In player's inventory
object ob = present(str, tp);
if(!ob)
  return "You don't have that.\n";

// In the room
object ob = present(str, environment(tp));
if(!ob)
  return "You don't see that here.\n";
```

### Parsing "X with Y" or "X from Y" syntax

```lpc
string target, tool;
if(sscanf(str, "%s with %s", target, tool) != 2)
  return notify_fail("Syntax: use <item> with <tool>\n");
```

### Action messages

```lpc
// Message to the player
tp->tell("You do something.\n");

// Message to the room (excluding the player)
environment(tp)->tell("Someone does something.\n", tp);

// Using action message system with verb conjugation
tp->my_action("$N $vswing the sword.");
tp->targetted_action("$N $vattack $t.", victim);
```

### Text wrapping

```lpc
// iwrap() for intelligent word-wrapping (adds newline)
return iwrap("This is a long message that will be wrapped.");

// no_ansi() to strip color codes for length calculations
return iwrap(no_ansi(ob->query("short")) + " cannot be assembled.");
```

## Complete Example: Simple Command

```lpc
// /cmds/std/_taste.c
// Taste an item in your inventory
//
// Created:     2026/03/22: Gesslar
// Last Change: 2026/03/22: Gesslar
//
// 2026/03/22: Gesslar - Created

inherit STD_CMD;

mixed main(object tp, string str) {
  object ob;

  if(tp->prevent_pet())
    return 1;
  if(!ep_check(tp, 5))
    return 1;
  if(!casting_check(tp))
    return 1;

  if(!str)
    return notify_fail("Taste what?\n");

  ob = present(str, tp);
  if(!ob)
    return "You don't have any " + str + ".\n";

  if(!ob->query("tasteable"))
    return iwrap("You can't taste " + no_ansi(ob->query("short")) + ".");

  tp->delta_ep(-5);
  tp->my_action("$N $vtaste $o.", ob);

  return ob->query("taste_message") ||
    iwrap("You taste " + no_ansi(ob->query("short")) + ". It tastes unremarkable.");
}

string query_help(object tp) {
  return @text
USAGE: taste <item>

Taste an item you are holding to determine its flavour.
text
;
}
```

## Complete Example: Delayed Action Command

```lpc
// /cmds/std/_meditate.c
// Meditate to recover spell points
//
// Created:     2026/03/22: Gesslar
// Last Change: 2026/03/22: Gesslar
//
// 2026/03/22: Gesslar - Created

inherit STD_CMD;

#define DELAY 20
#define SP_GAIN 50
#define EP_COST 15

mixed main(object tp, string str) {
  if(tp->prevent_pet())
    return 1;
  if(!ep_check(tp, EP_COST))
    return 1;
  if(!casting_check(tp))
    return 1;
  if(!attacking_check(tp))
    return 1;
  if(!cooldown_check(tp, "meditate"))
    return 1;

  tp->delta_ep(-EP_COST);
  tp->my_action("$N $vsit down and $vbegin to meditate.");
  delay_cast("meditating", "finish_meditate", DELAY, tp);

  return 1;
}

void finish_meditate(object tp) {
  if(!tp) return;

  tp->delta_sp(SP_GAIN);
  set_cooldown(tp, "meditate", 120);
  tp->my_action("$N $vfinish meditating, looking refreshed.");
}

string query_help(object tp) {
  return @text
USAGE: meditate

Sit and meditate to recover spell points. Requires endurance and
cannot be done while in combat or performing another action.
text
;
}
```

## Complete Example: Dispatcher Command

```lpc
// /cmds/std/_toolkit.c
// Manage your toolkit
//
// Created:     2026/03/22: Gesslar
// Last Change: 2026/03/22: Gesslar
//
// 2026/03/22: Gesslar - Created

inherit STD_CMD;

int perform(object caller, string arg) {
  string sub, args;

  if(!arg) {
    caller->tell(query_help(caller));
    return 1;
  }

  if(sscanf(arg, "%s %s", sub, args) != 2)
    sub = arg;

  switch(sub) {
    case "list": return list_tools(caller);
    case "add": return add_tool(caller, args);
    case "remove": return remove_tool(caller, args);
    default:
      caller->tell(query_help(caller));
      return 1;
  }
}

private int list_tools(object caller) {
  // ... implementation ...
  return 1;
}

private int add_tool(object caller, string args) {
  if(!args)
    return notify_fail("Add which tool?\n");
  // ... implementation ...
  return 1;
}

private int remove_tool(object caller, string args) {
  if(!args)
    return notify_fail("Remove which tool?\n");
  // ... implementation ...
  return 1;
}

string query_help(object tp) {
  return @text
USAGE: toolkit list
       toolkit add <item>
       toolkit remove <item>

Manage the tools in your toolkit.
text
;
}
```
