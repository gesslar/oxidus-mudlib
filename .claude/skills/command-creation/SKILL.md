---
name: command-creation
description: Create and modify commands for Oxidus. Covers file naming, inheritance hierarchy (STD_CMD, STD_ACT, STD_ABILITY, STD_SPELL), entry points, return value handling, help text, verb rules, and command directory structure.
---

# Command Creation Skill

You are helping create or modify commands for Oxidus. Commands are LPC files that players invoke by typing a verb. Follow the `lpc-coding-style` skill for all formatting.

## How Commands Work

1. Player types a verb (e.g., `eat cheese`)
2. `command_hook()` in `std/object/command.c` processes the input
3. First checks `evaluate_command()` on inventory, environment, and room contents (for `add_command()`-based commands)
4. Then checks soul emotes and channels
5. Then searches the player's PATH directories for `<verb>.c`
6. The command object is loaded and `main()` (or `process_verb_rules()` for verb commands) is called
7. The return value is evaluated by `evaluate_result()`

## File Naming and Location

- Commands are named `<verb>.c` (lowercase, no underscore prefix)
- Path directories are configured in `/adm/etc/` files

| Directory | Purpose | Config File |
|---|---|---|
| `cmds/std/` | Standard player commands | `adm/etc/standard_paths` |
| `cmds/wiz/` | Wizard/developer commands | `adm/etc/wizard_paths` |
| `cmds/adm/` | Admin commands | `adm/etc/wizard_paths` |
| `cmds/file/` | File-related commands | `adm/etc/wizard_paths` |
| `cmds/object/` | Object manipulation commands | `adm/etc/wizard_paths` |
| `cmds/ability/` | Ability-based commands | |
| `cmds/spell/` | Spell commands | |
| `cmds/ghost/` | Ghost/unregistered user commands | `adm/etc/ghost_paths` |

## Inheritance Hierarchy

```
STD_OBJECT
  └── STD_CMD (std/cmd/cmd.c) — base command class
        └── STD_ACT (std/cmd/act.c) — action commands, includes M_CHECKS
              └── STD_ABILITY (std/cmd/ability.c) — abilities with cost/cooldown
                    └── STD_SPELL (std/cmd/spell.c) — spell commands
```

Also: `STD_REPORTER` (`std/cmd/reporter.c`) — for bug/idea/typo reports, inherits `STD_CMD`.

### When to use which

| Inherit | When |
|---|---|
| `STD_CMD` | General commands, info display, wizard tools — no action checks needed |
| `STD_ACT` | Player actions (drop, eat, close) — includes `M_CHECKS` for validation |
| `STD_ABILITY` | Abilities with HP/SP/MP costs, cooldowns, and condition checks |
| `STD_SPELL` | Spells (extends ability with spell-specific features) |

## Entry Point

The standard entry point is `main()`:

```lpc
mixed main(object tp, string arg) {
    // tp = the user/caller
    // arg = everything after the verb (null if nothing)
    // return mixed — see Return Values below
}
```

For ability commands, override `use()` instead — `STD_ABILITY` handles `main()` itself, running `condition_check()` before calling `use()`:

```lpc
inherit STD_ABILITY;

mixed use(object tp, string arg) {
    // Conditions already checked by the time this runs
    apply_cost(tp, arg);
    // ... ability logic ...
    return 1;
}
```

## Return Value Handling

Return values are processed by `evaluate_result()`:

| Return Value | Effect |
|---|---|
| `1` | Success, stop processing |
| `0` | Failure, continue searching other handlers |
| `string` (non-empty) | Display to player (newline appended if missing), treat as success |
| `string` (empty `""`) | Treat as failure |
| `string *` (non-empty array) | Display via pager, treat as success |
| `string *` (empty array) | Treat as failure |

```lpc
// Return a string for simple feedback (auto-displayed, auto-newlined)
return "You can't do that.";

// Return 1 after manually handling output
tell(tp, "You swing the sword.\n");
return 1;

// Return 0 to indicate failure (other handlers may try)
return 0;
```

## Help Text

Two approaches:

### Via setup() variables

```lpc
void setup() {
    usage_text = "eat <item>\n";
    help_text = "Eat an item you are holding.\n\nSee also: drink\n";
}
```

### Via query_help()

```lpc
string query_help(object tp) {
    return
"SYNTAX: force <living> to <cmd>\n\n"
"Force a living object to execute a command.";
}
```

## Messaging Functions

| Function | Purpose |
|---|---|
| `tell(ob, msg)` | Send message to an object |
| `tell_me(msg)` | Send message to self (the command caller) |
| `_info(tp, fmt, ...)` | Info-level formatted message |
| `_error(fmt, ...)` | Error-level formatted message |
| `_ok(tp, fmt, ...)` | Success-level formatted message |

### Action Messages

```lpc
// Message to player and room with verb conjugation
tp->my_action("$N $vswing the sword.");
tp->simple_action("$N $vdrop $p $o.", ob);
tp->targetted_action("$N $vattack $t.", victim);
```

## Finding Objects

```lpc
// Find single object by name in a location
object ob = find_target(tp, arg, tp);         // in player inventory
object ob = find_target(tp, arg, environment(tp));  // in the room

// Find multiple objects
object *obs = find_targets(tp, arg, tp);

// Find player by name
object ob = find_player(name);

// Lower-level
object ob = present(name, location);
```

## Verb Rules

Commands can define verb grammar rules for structured argument parsing:

```lpc
void setup() {
    add_verb_rule("");        // No arguments
    add_verb_rule("WRD");    // Single word argument
}
```

When `is_verb()` returns true (i.e., verb rules are defined), `command_hook()` calls `process_verb_rules()` instead of `main()`.

## STD_ABILITY Features

Abilities provide built-in cost and cooldown management:

### Costs

Set cost variables in `setup()`:

```lpc
void setup() {
    hp_cost = 10.0;
    sp_cost = 5.0;
    mp_cost = 0.0;
}
```

`condition_check()` verifies the player can afford the cost. Call `apply_cost()` to deduct after success.

### Cooldowns

```lpc
void setup() {
    cooldowns = ([
        "ability_name" : ({ "", 30 }),  // 30-second cooldown
    ]);
}
```

`condition_check()` checks cooldowns automatically. Call `apply_cooldown()` after success.

### Condition Check Return Values

For `STD_ABILITY`, `condition_check()` returns:
- `0` — failure, return 0 to caller
- `1` — conditions met, proceed to `use()`
- `2` — failure, but message already sent (return 1 to stop processing)

### Targeting

```lpc
void setup() {
    aggressive = 1;     // Marks as aggressive (checks prevent_combat)
    target_current = 1; // Allow targeting current combat target if no arg given
}

mixed use(object tp, string arg) {
    object target = local_target(tp, arg);
    if(!target) return 1;
    // ...
}
```

## add_command() — Object-Based Commands

Objects (rooms, items, NPCs) can register commands that only work when the object is present:

```lpc
void setup() {
    add_command("push", "do_push");
    add_command(({"pull", "yank"}), "do_pull");
}

mixed do_push(object tp, string arg) {
    // ...
    return 1;
}
```

These are checked before PATH-based commands.

## Complete Example: Simple Action Command

```lpc
// /cmds/std/eat.c

inherit STD_ACT;

mixed main(object tp, string str) {
    object ob;
    int uses;

    if(!ob = find_target(tp, str, tp))
        return "You don't have that.";

    if(!ob->is_edible())
        return "You can't eat that.";

    uses = ob->query_uses();
    if(uses < 1)
        return "There is nothing left to eat.";

    if(!ob->consume(tp))
        return "You couldn't eat that.";

    return 1;
}
```

## Complete Example: Multi-Option Command with Help

```lpc
// /cmds/std/drop.c

inherit STD_ACT;

void setup() {
    usage_text =
"drop <object>\n"
"drop all\n"
"drop all <object>\n";
    help_text =
"This command will allow you to drop an object you are currently holding onto "
"the ground.\n\nSee also: get, put\n";
}

mixed main(object tp, string arg) {
    object room = environment(tp);

    if(!arg)
        return "Drop what?";

    if(arg == "all") {
        object *inv = find_targets(tp, 0, tp);

        if(!sizeof(inv))
            return "You don't have anything in your inventory.\n";

        foreach(object item in inv) {
            if(item->prevent_drop())
                tp->my_action("$N $vcannot drop $p $o.", item);
            else if(item->move(room))
                tp->my_action("$N could not drop $p $o.", item);
            else
                tp->my_action("$N $vdrop $p $o.", item);
        }
    } else {
        object ob = find_target(tp, arg, tp);

        if(!ob)
            return "You don't have a '" + arg + "' in your inventory.\n";

        if(ob->prevent_drop())
            return "You cannot drop " + ob->query_real_name() + ".\n";

        if(ob->move(room))
            return "You could not drop " + ob->query_real_name() + ".\n";

        tp->simple_action("$N $vdrop $p $o.", ob);
    }

    return 1;
}
```

## Complete Example: Wizard Command with _error/_ok

```lpc
// /cmds/wiz/force.c

inherit STD_CMD;

mixed main(object tp, string args) {
    string target, cmd;
    object ob;

    if(!stringp(args) || sscanf(args, "%s to %s", target, cmd) != 2)
        return _error("Syntax: force <living> to <cmd>");

    ob = present(lower_case(target), environment(tp));
    if(!ob)
        return _error("%s not found.", target);

    if(!living(ob))
        return _error("%s is not a living object.", target);

    int result = ob->force_me(cmd);
    if(result == false)
        _info(tp, "Unable to force %s to %s", ob->query_name(), cmd);
    else
        _ok(tp, "Forced %s to %s", ob->query_name(), cmd);

    return 1;
}
```
