---
name: npc-creation
description: Create and modify NPCs and monsters for Oxidus. Covers code-based NPCs (STD_NPC), data-driven monsters (virtual_setup, LPML), set_level, race modules, body parts, heartbeat optimization, combat memory, loot/coin tables, utility-AI decisions, and the virtual compile flow.
---

# NPC Creation Skill

You are helping create and modify NPCs/monsters for Oxidus. Follow the `lpc-coding-style` skill for all LPC formatting.

## Architecture Overview

```
STD_NPC (std/living/npc.lpc)               — NPC base: heartbeat, set_level, death detect
  └── STD_BODY                            — inherits skills, combat, vitals, wealth, etc.

STD_MONSTER (std/mobs/monster.lpc)          — data-driven virtual_setup for LPML mobs
  └── STD_NPC

race.lpc (std/living/race.lpc)               — race module loader
race/race.lpc (std/modules/race/race.lpc)   — body parts, equipment slots, regen rates
race/human.lpc, race/ghost.lpc, etc.         — specific race implementations

decision.lpc (std/living/decision.lpc)       — utility-AI for NPC behaviour
combat_memory.lpc (mob module)             — remember and attack on sight

EXT_LOOT (std/ext/loot.lpc)                — loot/coin table definitions
LOOT_D (adm/daemons/loot.lpc)              — loot drop resolution on death
```

### Inheritance Chain

```
STD_MONSTER (std/mobs/monster.lpc)
  └── STD_NPC (std/living/npc.lpc)
        └── STD_BODY (std/living/body.lpc)
              ├── STD_CONTAINER, STD_ITEM
              ├── advancement, attributes, boon, combat, damage
              ├── equipment, module, race, skills, vitals, wealth
              └── EXT_ACTION, EXT_LOG
```

## Creating a Code-Based NPC

```lpc
inherit STD_NPC;

void setup() {
    set_name("guard");
    set_short("a town guard");
    set_long("A sturdy town guard in chainmail.");
    set_id(({"guard", "town guard"}));
    set_gender("male");
    set_race("human");
    set_level(5.0);                          // BEFORE custom skills
    set_damage(8.0);
    set_weapon_name("sword");
    set_weapon_type("slashing");
    add_loot("/obj/loot/sword.loot", 25.0);
    add_coin("copper", 10, 100.0);
    add_coin("silver", 2, 50.0);
}
```

## Creating a Data-Driven NPC (LPML)

Create a data file at `/d/mobs/town_guard.lpml`:

```lpml
{
    type: "monster",
    name: "guard",
    short: "a town guard",
    long: "A sturdy town guard in chainmail.",
    id: ["guard", "town guard"],
    gender: "male",
    race: "human",
    level: [4, 7],
    damage: 8.0,
    weapon name: "sword",
    weapon type: "slashing",
    loot: [
        ["/obj/loot/sword.loot", 25.0],
    ],
    coins: {
        copper: [10, 100.0],
        silver: [2, 50.0],
    },
}
```

Then reference it as **`mob/town_guard`** — no leading slash, no extension:

```lpc
new("mob/town_guard");
```

Paths resolve from the mudlib root, so that is `/mob/town_guard`, and `mob`
being the leading directory is what routes it to the monster compiler. Only the
basename is then used, to find `/d/mobs/<basename>.lpml`. There is no `/mob/`
directory on disk — the path is a routing instruction, not a location. This is
the form every area in the lib uses.

A `.mob` extension reaches the same compiler by the extension route, which is
how to get there from a path that does not begin with `mob/`.

**What does not work is a path with `mob/` in the middle of it** — 
`/d/forest/mob/crimson_fox` routes on its *first* component (`d`), lands in the
forest's virtual server, and returns 0.

### Virtual Compile Flow

```
Request: mob/wild_boar  (or anything.mob)
  → VIRTUAL_D routes on the leading directory, or on a .mob extension
  → mob.lpc virtual module reads /d/mobs/wild_boar.lpml
  → lpml_decode() → mapping
  → new("/std/mobs/<type>.lpc", data)
  → virtual_setup(data) called on new object
```

The `type` field maps to `/std/mobs/<type>.lpc` (spaces become underscores). If that file does not exist the compile returns 0 and the monster silently does not appear.

## NPC Base — `std/living/npc.lpc`

### Setup

`init_living()` runs for every NPC. The rest is guarded by `clonep()`, so a
blueprint gets none of it:

1. `init_living()` — initializes attributes, vitals, boon, wealth.
2. `rehash_capacity()`.
3. `add_init("start_heart_beat")` — starts ticking when a player enters the room.
4. `add_hb("stop_heart_beat")` — checked each heartbeat to stop when the room is empty.
5. `add_module("std/modules/mob/combat_memory")` — loads the combat memory module.

### `set_level(float level)`

**Overrides** the body's `set_level`. After setting the level, calls `adjust_skills_by_npc_level(level)`, which **seeds every stored skill to a multiple of the NPC's level** so combat math reads honest values. The multiplier lives in `adjust_skill_levels()` in `/std/living/skills.lpc`.

**Order matters:** If you add custom skills and then call `set_level()`, `adjust_skills_by_npc_level()` overwrites them all. Set level first, then add any custom skills if needed (though for NPCs every skill already resolves to the level-scaled value, so custom values rarely matter).

### NPC Weapon Properties

Used when the NPC has no wielded weapon object:

| Function | Default | Description |
|---|---|---|
| `set_damage(float)` | `0.0` | **A ceiling, not a fixed value.** `query_damage()` returns `random_float(__damage)`. If `__damage <= 0`, the roll scales with level instead: `query_level() + random_float(query_level())` |
| `set_weapon_name(string)` | `"fist"` | Display name for combat messages |
| `set_weapon_type(string)` | `"bludgeoning"` | Damage type string |

### Heartbeat Optimization

NPCs only tick when players are present:

```lpc
void start_heart_beat() {
    if(player_check())
        set_heart_beat(mud_config("DEFAULT_HEART_RATE"));  // 10
}

void stop_heart_beat() {
    if(!player_check() && query_hp() >= 100.0)
        set_heart_beat(0);
}
```

**Note the second condition.** `stop_heart_beat()` only stops an NPC that is *also* at full health, so a wounded NPC left alone keeps ticking and regenerating until it is whole, and only then goes quiet. An NPC at 1 HP in an empty room heals back up; it does not sit there.

**Consequences once it has stopped:** no regen, no boon expiration processing, no AI decisions, no death detection.

The threshold is the literal `100.0`, not `query_max_hp()`. Every living starts at `max_hp` 100.0, so the two normally coincide — but an NPC given a smaller maximum can never reach 100 and so never stops ticking.

### Death Detection

```lpc
// In heart_beat():
if(!is_dead() && query_hp() <= 0.0) {
    set_dead(1);
    die();
    return;
}
```

Death is detected in the heartbeat, not in `receive_damage()`. There is a brief window between HP hitting zero and `die()` firing.

### Heartbeat Loop

Full sequence each tick: `clean_up_enemies()` → `cooldown()` → net-dead check (for possessed NPCs) → death check → `heal_tick()` → `evaluate_heart_beat()` → `process_boon()`.

### Other NPC Functions

| Function | Description |
|---|---|
| `set_name(string)` | Also calls `set_living_name(lower_case(name))` |
| `force_me(string cmd)` | Executes `command(cmd)` as this NPC |
| `restore_body() / save_body()` | Both no-ops — NPCs don't persist |
| `is_npc()` | Returns 1 |
| `player_check()` | Returns 1 if players are in environment |

## Data-Driven Monsters — `std/mobs/monster.lpc`

### `virtual_setup(mixed args...)`

Called when cloning from LPML data. `args[0]` must be a mapping.

| Key | Type | Behaviour |
|---|---|---|
| `"name"` | string | `set_name()` |
| `"short"` | string | `set_short()` |
| `"long"` | string | `set_long()` |
| `"id"` | string or array | `set_id()` |
| `"level"` | int, or `[min, max]` array | Int: exact. Array: `random(max-min) + min`. Null: 1 |
| `"gender"` | string, array, or mapping | String: exact. Array: `element_of()`. Mapping: `element_of_weighted()` |
| `"damage"` | float | `set_damage()` |
| `"weapon name"` | string | `set_weapon_name()` (e.g., "tusks") |
| `"weapon type"` | string | `set_weapon_type()` (e.g., "piercing") |
| `"race"` | string | `set_race()` |
| `"attack speed"` | float | `set_attack_speed()` — seconds between rounds |
| `"plus attack speed"` | float | `add_attack_speed()` — **subtracts** from the interval, so positive is faster |
| `"proc chance"` | float | `set_proc_chance()` — how often any proc fires |
| `"simple procs"` | array of mappings | One `add_simple_proc()` per entry — see below |
| `"loot"` | mapping or array | See loot section below |
| `"loot chance"` | float | Default % for array-style loot. Default 50.0 |
| `"coins"` | mapping | `{ type: [num, chance] }` |

After all data is applied, calls `call_if(this_object(), "monster_setup", data)` — subclasses can define `monster_setup(mapping data)` for additional custom setup.

### Level Range Interpretation

The `[min, max]` array rolls `random_clamp(min, max)`, which is **inclusive of both ends**. `[1, 3]` gives 1, 2 or 3; `[3, 5]` gives 3, 4 or 5.

This is the same helper the area spawn paths use — the `spawn.lpml` pools and thornwick's `area_spawn()` — so a range means the same thing wherever it is written.

### Gender Formats

- `"male"` — exact
- `["male", "female"]` — random pick via `element_of()`
- `([ "male": 70, "female": 30 ])` — weighted random via `element_of_weighted()`

### Custom Monster Types

Subclass `STD_MONSTER` for specialized mob types:

```lpc
// std/mobs/undead.lpc
inherit STD_MONSTER;

void monster_setup(mapping data) {
    // Called after virtual_setup processes all standard keys
    set_race("skeleton");
    // Apply undead-specific behaviour
}
```

## LPML Monster Data Format

All mob data files live in `/d/mobs/*.lpml`.

### Loot Format

**Mapping format:** `{ "/obj/loot/item.loot": 75.0 }` — path to chance percentage.

**Array format:** Each entry is either a string path (uses `loot_chance` default) or `[path, chance]` pair:

```lpml
loot: [
    "/obj/loot/tusk.loot",                    // uses loot_chance default
    ["/obj/loot/rare_hide.loot", 10.0],       // explicit 10% chance
],
loot chance: 75.0,    // default for plain-path entries
```

### Coin Format

```lpml
coins: {
    copper: [3, 100.0],    // 3 copper, 100% drop chance
    silver: [1, 50.0],     // 1 silver, 50% drop chance
}
```

### Simple Procs

A proc is an alternate attack an NPC sometimes uses instead of a normal swing.
`virtual_setup()` feeds each entry of `"simple procs"` to `add_simple_proc()`:

```lpml
simple procs: [
    {
        tag: "gore",
        messages: [ "$N $vgore $t with $p short tusks." ],
        damage_type: "piercing",
        attack_type: "melee",   // defaults to "melee"
        cooldown: 8,
        weight: 100,
        severity: "normal",
    },
],
```

`tag`, `messages` and `damage_type` are required — `add_simple_proc()` asserts
on them. `messages` must hold **one or two** strings: one goes to the target,
and a second, if given, to everyone else. `weight` is clamped 0-100; `severity`
defaults to `"normal"` and scales the damage.

Dispatch runs through `proc_npc()` in `std/living/proc.lpc`, which builds the
skill name as `combat.<attack_type>.<damage_type>`, rolls `can_strike()` against
the current highest-threat enemy, and delivers mundane damage on a hit.

**`"simple"` is currently the only proc type.** `proc_npc()` dispatches on
`proc.type` and handles nothing else. This is a deliberate starting point —
curses, damage over time and area effects are not built.

## Race System — `std/living/race.lpc`

### `set_race(string race)`

1. Checks for race module file at `DIR_STD_MODULES "race/" + race + ".lpc"`.
2. If file exists: loads via `add_module("race/" + race)`. The module's `start_module()` sets up body parts.
3. **If file doesn't exist: silently stores just the string.** No body parts and no equipment slots. No error is raised.

Every animal race used by the mobs in `/d/mobs` (`pig`, `wolf`, `fox`, `insect`,
`rodent`, …) is in this state — name only. Race on a monster is identity and
flavour until somebody writes a module for it. The modules that do exist are the
playable races: human, elf, dwarf, gnome, orc, troll, plus ghost.

### Race Module Base — `std/modules/race/race.lpc`

All race modules inherit this.

#### Default Humanoid Body

| Part | Size | Vitalness |
|---|---|---|
| head | 3 | 3 |
| neck | 2 | 2 |
| torso | 30 | 1 |
| left arm / right arm | 4 | 1 |
| left hand / right hand | 2 | 1 |
| left leg / right leg | 4 | 1 |
| left foot / right foot | 2 | 1 |

**Size** is the weight for `random_body_part()` (hit location in combat). **Vitalness** controls hit severity.

#### Default Equipment Slots

| Slot | Covers |
|---|---|
| head | `{"head"}` |
| neck | `{"neck"}` |
| torso | `{"torso"}` |
| arms | `{"left arm", "right arm"}` |
| hands | `{"left hand", "right hand"}` |
| legs | `{"left leg", "right leg"}` |
| feet | `{"left foot", "right foot"}` |

#### Key Functions

| Function | Description |
|---|---|
| `use_default_body_parts()` | Populates from humanoid defaults |
| `wipe_body_parts()` | Clears all body/equipment mappings |
| `add_body_part(string, int size, int vitalness)` | Add custom part |
| `remove_body_part(string)` | Remove a part |
| `random_body_part()` | Size-weighted random selection |
| `add_equipment_slot(string, string*)` | Add slot covering body parts |
| `remove_equipment_slot(string)` | Remove a slot |
| `query_regen_rate(string type)` | Returns regen mapping or specific value |

### Existing Race Implementations

**Human** (`race/human.lpc`): Calls `use_default_body_parts()`. Regen: `([ "hp":2.0, "sp":2.0, "mp":4.0 ])`.

**Ghost** (`race/ghost.lpc`): Calls `wipe_body_parts()`. No body parts at all. No regen (all 0.0).

### Creating a New Race

```lpc
// std/modules/race/orc.lpc
inherit M_MOBILE "race/race";

protected void set_up_body_parts(object ob, mixed args...) {
    use_default_body_parts();           // start with humanoid template
    set_body_part_size("torso", 35);    // larger torso
    add_body_part("tusks", 1, 2);      // add tusks
    regen_rate = ([ "hp": 3.0, "sp": 1.5, "mp": 3.0 ]);
}
```

## Loot System — `std/ext/loot.lpc`

Mixed into `STD_NPC` via `inherit EXT_LOOT` in `npc.lpc`.

### Loot Table

Each entry: `({ item, chance_float })`. Item can be:
- `string` — file path for `new()`
- `mapping` — weighted random selection via `element_of_weighted()`
- `function` — called with `(killer, source)`, return value used as item
- `array` — random element pick

### Coin Table

Each entry: `({ type_string, num_int, chance_float })`.

### Functions

| Function | Signature | Description |
|---|---|---|
| `add_loot` | `(mixed item, float chance)` | Chance defaults to 100.0, clamped 0-100 |
| `set_loot_table` | `(mixed *table)` | Replace entire table |
| `query_loot_table` | `() : mixed*` | Returns copy |
| `add_coin` | `(string type, int num, float chance)` | Add coin drop entry |
| `set_coin_table` | `(mixed *table)` | Replace entire table |
| `query_coin_table` | `() : mixed*` | Returns copy |

### Loot Daemon Resolution

`LOOT_D->loot_drop(killer, source)`:
1. Iterates `source->query_loot_table()`.
2. For each: rolls `random_float(100.0)`, drops if roll < chance.
3. Processes item through `process_loot_item()` (handles functions, arrays, mappings, strings).
4. Clones result, optionally auto-values via `determine_value_by_level()`.

### Auto-Valuation

If a loot item has `query_loot_property("autovalue") == true`:

```lpc
value = level * COIN_VALUE_PER_LEVEL   // then spread by COIN_VARIANCE
item->set_value(value)
```

The variance is a fraction of the base, applied centred on it, so the result
lands within half that spread either side. Both keys are tuning knobs — read
them with `mud_config()` rather than working an example out here.

## Utility-AI Decision System — `std/living/decision.lpc`

Adapted from the npm `utility-ai` package.

:::danger
**This is not wired in.** `std/living/decision.lpc` is not inherited by
`npc.lpc`, `body.lpc`, or anything else, and `setup_utility_ai()` is never
called. The decisions inside it (`remove_stun`, `cast_fireball`,
`cast_lightning`) are illustrative placeholders, not live behaviour.

Nothing in the lib currently makes an AI decision through this path. Treat the
API below as a design that exists on disk and would need inheriting and calling
before it does anything — not as something an NPC you create will use.
:::

### Classes

```lpc
class Score {
    string description;
    function callback;       // returns int score contribution
}

class Decision {
    string description;
    function condition;      // returns truthy to allow evaluation
    class Score *scores;
}
```

### Functions

| Function | Signature | Description |
|---|---|---|
| `add_decision` | `(string desc, function condition)` | Register a decision with its condition |
| `add_func` | `(string desc, function func)` | Register the action for a decision (matched by description string) |
| `add_score` | `(string decision_desc, string score_desc, function callback)` | Add a scoring function to a decision |
| `modify_condition` | `(string desc, function callback)` | Replace a decision's condition |
| `modify_score` | `(string decision_desc, string score_desc, function new_callback)` | Replace a scoring function |
| `evaluate_decision` | `(class Decision, mapping data)` | Sum all scores if condition passes, else `-MAX_INT` |
| `decide` | `(mapping data)` | Evaluates all decisions, returns `([ "decision", "score", "func" ])` for highest |

### Usage Pattern

This is how it *would* be used once wired in; no NPC does this today.

```lpc
// In setup:
add_decision("cast_fireball", (: num_combatants :));
add_score("cast_fireball", "priority", (: 50 + random(10) :));
add_func("cast_fireball", (: cast_fireball :));

add_decision("flee", (: query_hp_ratio() < 25 :));
add_score("flee", "urgency", (: 100 - query_hp_ratio() :));
add_func("flee", (: attempt_flee :));

// In heartbeat or combat callback:
mapping result = decide(data_mapping);
if(valid_function(result["func"]))
    result["func"]();
```

**Note:** `add_func` is keyed by description string and looked up separately from the Decision — the description must match exactly.

## Combat Memory Module — `std/modules/mob/combat_memory.lpc`

Automatically added to all NPCs in `npc.lpc::mudlib_setup()`.

### Behaviour

- Stores enemy **names** (strings) in `combat_memory` array (nosave).
- Registers `attack_on_sight` as an init hook.
- When a player enters the room: if their name is in memory, the NPC immediately attacks with **two free strikes** before the combat round loop starts.

```lpc
void attack_on_sight(object target) {
    if(target->is_ghost()) return;
    if(of(target->query_name(), combat_memory)) {
        query_owner()->start_attack(target);
        query_owner()->strike_enemy(target);   // free strike 1
        query_owner()->strike_enemy(target);   // free strike 2
    }
}
```

Memory is populated from `combat.lpc::start_attack()` for NPC combatants.

Because the store holds **names rather than object references**, a grudge
survives the player's body being replaced — dying, being revived, and logging
out and back in all build a new body carrying the same name. It lasts exactly as
long as the NPC object, though: a reboot or a `renew` starts it empty. The
`nosave` on the declaration is belt-and-braces and changes nothing, since NPCs
never call `set_persistent()`.

## NPC Skill Behaviour

**An NPC's skills come from its level.** This is a deliberate normalisation, not
a gap: a monster exists to be level-appropriate opposition, so `set_level(3.0)`
is the entire skill configuration of a level 3 monster and nothing further needs
authoring.

1. **`set_level()` seeds the stored tree.** `adjust_skills_by_npc_level()` walks every node and sets it to a multiple of the level — it does not wipe them to zero. The recursive walker is `adjust_skill_levels()` in `/std/living/skills.lpc`; the multiplier is `COMBAT.NPC_SKILL_MULTIPLIER`.
2. **`query_skill()` and `query_skill_level()` do not read that storage on an NPC.** Both branch on `pcp()`, and on a non-PC they compute `query_effective_level() * COMBAT.NPC_SKILL_MULTIPLIER` (plus boon) for *any* skill name, known or not. There very much is NPC special-casing in the query path.
3. **The API consequence:** those two never return null on an NPC, so `has_skill()` is the existence check, and `query_raw_skill()` / `query_raw_skill_level()` are what read the seeded tree. Combat math uses `query_skill_level()`, so the seeded values do not change how an NPC fights.
4. See the `skills-and-advancement` skill for full details on the skill system.

## Signals

| Signal | When | Payload |
|---|---|---|
| `SIG_PLAYER_DIED` | `die()` sequence | `(self, killed_by)` |
| `SIG_PLAYER_ADVANCED` | Level-up via `advance()` | `(tp, new_level)` |

## Config Constants

| Key | Used In |
|---|---|
| `DEFAULT_HEART_RATE` | NPC heartbeat rate |
| `COIN_VALUE_PER_LEVEL` | Loot auto-valuation |
| `COIN_VARIANCE` | Loot value variance |
| `DEFAULT_RACE` | Default race |
| `COMBAT.NPC_SKILL_MULTIPLIER` | Skill level seeded by `set_level()` |

Values live in `adm/etc/default.lpml`. Read them with `mud_config()`; do not restate them here or in code.

## Gotchas

1. **`set_level()` on NPCs overwrites stored skills** with the level-scaled value. Always call `set_level()` before adding custom skills.
2. **`set_race()` silently falls back** if the race module file doesn't exist. The NPC will have no body parts and no equipment slots — but it still regenerates, via the fallback in `heal_tick()`. No error is raised.
3. **NPCs stop ticking in empty rooms — but only once healed.** `stop_heart_beat()` requires no players *and* `query_hp() >= 100.0`, so a wounded NPC keeps ticking and regenerates to full before going quiet. Once stopped: no regen, no boon processing, no AI decisions, no death detection.
4. **`add_func()` matches by exact description string.** If the string doesn't match a registered decision, the function is never called.
5. **Combat memory lasts exactly as long as the NPC object**, and is keyed by name — so it survives the player dying, reviving, or relogging, but not the NPC being reloaded. Ghosts are skipped outright.
6. **Level range `[min, max]` uses `random(max - min) + min`**, so the maximum value is `max - 1`, not `max`.
7. **No race module still regenerates.** `heal_tick()` calls `module("race", "query_regen_rate")` and, when that comes back null, falls back to a flat rate for all three pools chosen by `pcp()` — player characters get the low rate, everything else a considerably higher one. A race-less monster heals fine.
8. **Death is detected in heartbeat, not `receive_damage`.** There's a brief window between HP hitting zero and `die()` firing. If the heartbeat is stopped (empty room), death won't trigger at all.
