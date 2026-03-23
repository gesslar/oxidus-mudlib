---
name: skills-and-advancement
description: Understand and work with the skill tree and advancement systems in Oxidus. Covers the nested skill tree, dot-path addressing, use-based improvement, query_skill vs query_skill_level, boon integration, XP, TNL formula, leveling, attributes, and how skills interact with combat and NPCs.
---

# Skills and Advancement Skill

You are helping work with the Oxidus skill and advancement systems. Follow the `lpc-coding-style` skill for all LPC formatting.

## Architecture Overview

```
skills.c (std/living/skills.c)           — nested skill tree, use-based improvement
advancement.c (std/living/advancement.c) — per-living XP/level state
advance.c (adm/daemons/advance.c)       — TNL formula, killXp, earnXp
attributes.c (std/living/attributes.c)   — STR/DEX/CON/INT/WIS/CHA
boon.c (std/living/boon.c)              — buff/debuff modifiers on skills and vitals
```

All of these are inherited by `STD_BODY` and apply to both players and NPCs.

## Skill System — `std/living/skills.c`

### Storage Structure

Skills are a **nested tree**, not a flat mapping:

```lpc
skills = ([
    "combat": ([
        "level": 3.47,
        "subskills": ([
            "melee": ([
                "level": 2.15,
                "subskills": ([
                    "slashing": ([ "level": 4.82, "subskills": ([]) ]),
                    "piercing": ([ "level": 1.03, "subskills": ([]) ]),
                    "bludgeoning": ([ "level": 2.60, "subskills": ([]) ]),
                    "unarmed": ([ "level": 1.55, "subskills": ([]) ]),
                ]),
            ]),
            "defense": ([
                "level": 1.90,
                "subskills": ([
                    "dodge": ([ "level": 3.21, "subskills": ([]) ]),
                    "parry": ([ "level": 1.10, "subskills": ([]) ]),
                ]),
            ]),
        ]),
    ]),
])
```

Dot notation addresses nodes: `"combat.melee.slashing"` navigates the tree.

The **integer part** of the level is the effective skill level. The **fractional part** is progress toward the next level (0-99%).

### Default Skill Tree (from config)

```
combat
  defense: dodge, parry
  melee: attack, bludgeoning, piercing, slashing, unarmed
social: barter, charm, intimidate, persuade
general: appraise, hide, jump, listen, search, spot, swim
```

Full dot-path examples: `"combat.melee.slashing"`, `"combat.defense.dodge"`, `"social.barter"`, `"general.swim"`.

### Key Functions

| Function | Signature | Description |
|---|---|---|
| `add_skill` | `int (string skill, float level)` | Creates skill at dot-path. Intermediates created at level 1.0. Returns 1 on success |
| `remove_skill` | `int (string skill)` | Removes leaf node |
| `query_skill` | `float (string skill)` | **NPC shortcut: returns `queryLevel() * 3.0`**. For players: returns raw float level |
| `query_skill_level` | `float (string skill, int raw)` | Returns `floor(level)`. Unless `raw=1`, adds `query_effective_boon("skill", skill)`. **No NPC shortcut** |
| `set_skill_level` | `int (string skill, float level)` | Sets exact float level |
| `query_skills` | `mapping ()` | Returns copy of entire tree |
| `set_skills` | `void (mapping s)` | Replaces entire tree |
| `use_skill` | `int (string skill)` | 20% chance to call `improve_skill()`. Auto-creates skill if missing |
| `improve_skill` | `float (string skill, float progress)` | See improvement algorithm below |
| `query_skill_progress` | `int (string skill)` | Fractional part as 0-99 integer |
| `assure_skill` | `int (string skill)` | Creates at level 1.0 if missing, notifies player |
| `wipe_skills` | `void ()` | Resets to empty mapping |
| `initialize_missing_skills` | `void (mapping, string)` | Creates all skills from config tree |
| `adjust_skills_by_npc_level` | `int (float level)` | NPC-only: resets all skills to `random_float(0.01)` |

### Use-Based Improvement

Players improve skills transparently by using them — no skill points or manual allocation.

```lpc
int use_skill(string skill) {
    if(query_skill(skill)) {
        if(random_float(100.0) < 20.0)     // 20% chance per use
            improve_skill(skill);
    } else {
        assure_skill(skill);                // auto-create at level 1.0
    }
}
```

`use_skill()` is called throughout the codebase:
- Combat: attacker trains weapon skill after each swing, defender trains defense skill on every hit attempt.
- Any system can call `use_skill("general.swim")` etc. to trigger organic improvement.

### Improvement Algorithm

When `progress` is null (standard path from `use_skill`):

1. Builds the full skill path (e.g., `"combat"`, `"combat.melee"`, `"combat.melee.slashing"`).
2. Creates a weighted mapping: leaf skills get weight `(depth+1)*3`, parents get lower weights.
3. Selects a skill from the path via `element_of_weighted`.
4. Sets `progress = random_float(0.01)` — tiny increment.
5. Adds progress to the selected skill's level.
6. If the integer part increases, notifies the player: `"You have improved your X skill."`

This means using `"combat.melee.slashing"` can also improve `"combat.melee"` or `"combat"` — but with lower probability. Parent skills grow organically as their children are used.

When `progress` is provided (direct call), it's applied to the named skill directly without the weighted path selection.

### `query_skill()` vs `query_skill_level()` — Critical Difference

| | `query_skill()` | `query_skill_level()` |
|---|---|---|
| NPC behavior | Returns `queryLevel() * 3.0` | Reads stored tree value |
| Return value | Raw float (e.g., 3.47) | Floored integer (e.g., 3) |
| Boon modifier | No | Yes (unless `raw=1`) |
| Used for | Multi-strike chance, NPC general checks | Hit/damage formulas, combat math |

**This distinction matters for NPCs.** `query_skill()` gives the level*3 shortcut, but `query_skill_level()` reads the actual (near-zero) stored values. Combat formulas use `query_skill_level()`.

**For players**, both read the real stored values — `query_skill()` returns the raw float, `query_skill_level()` returns the floored integer with boon modifiers applied.

### Boon Integration

`query_skill_level()` (unless `raw=1`) adds the effective boon modifier:

```lpc
return floor(level) + query_effective_boon("skill", skill);
```

Where `query_effective_boon("skill", "combat.melee.slashing")` = sum of boons minus sum of curses for that skill class+type. See the `buff-system` skill for details on applying boons.

### Skills Used by the Combat System

| Skill | Where Used |
|---|---|
| `"combat.melee"` | Multi-strike chance in `swing()` |
| `"combat.melee.<type>"` | Hit chance and damage formulas |
| `"combat.melee.unarmed"` | Unarmed combat fallback |
| `"combat.defense.dodge"` | Melee defense in hit chance |
| `"combat.defense.evade"` | Spell defense in hit chance |
| `"combat.defense"` | Generic defense reduction in damage formula |

## XP and Advancement

### Per-Living State — `std/living/advancement.c`

| Variable | Type | Default | Description |
|---|---|---|---|
| `_level` | `float` | `1.0` | Current level |
| `_level_mod` | `float` | `0.0` | Temporary level modifier (buffs/debuffs) |
| `_xp` | `int` | `0` | Accumulated experience points |

### Functions

| Function | Signature | Description |
|---|---|---|
| `queryXp` | `int ()` | Returns `_xp` |
| `queryLevel` | `float ()` | Returns `_level` |
| `query_effective_level` | `float ()` | Returns `_level + _level_mod`. Used throughout combat math |
| `query_tnl` | `float ()` | Returns `ADVANCE_D->toNextLevel(_level)` |
| `setLevel` | `float (float l)` | Sets `_level`. Sends GMCP `Char.Status` with xp/tnl/level |
| `add_level` | `float (float l)` | Adds to `_level`. Sends GMCP |
| `set_level_mod` | `float (float l)` | Sets temporary modifier |
| `adjust_level_mod` | `float (float l)` | Adjusts modifier by delta |
| `adjustXp` | `int (int amount)` | Adds to `_xp`. Sends GMCP |
| `setXp` | `int (int amount)` | Currently delegates to `adjustXp` |
| `on_advance` | `void (object tp, float l)` | Sends "You have advanced to level N!" message |

### Advancement Daemon — `adm/daemons/advance.c`

Config constants:

| Key | Default | Description |
|---|---|---|
| `BASE_TNL` | `100` | XP for level 1 to 2 |
| `TNL_RATE` | `1.25` | Geometric multiplier per level |
| `PLAYER_AUTOLEVEL` | `true` | Auto-advance on XP gain |
| `OVERLEVEL_THRESHOLD` | `5` | Levels above target before XP penalty |
| `OVERLEVEL_XP_PUNISH` | `0.05` | -5% per level over threshold |
| `UNDERLEVEL_THRESHOLD` | `0` | Levels below target for XP bonus |
| `UNDERLEVEL_XP_BONUS` | `0.05` | +5% per level under threshold |

### TNL Formula

```lpc
toNextLevel(level) = to_int(baseTnl * pow(tnlRate, level - 1.0))
// Default: to_int(100 * 1.25^(level-1))
```

| Level | TNL | Level | TNL |
|---|---|---|---|
| 1 | 100 | 10 | 931 |
| 2 | 125 | 15 | 2842 |
| 5 | 244 | 20 | ~8674 |

### `advance(object tp)`

1. Checks `canAdvance(xp, level)` — is XP >= TNL?
2. Deducts TNL from XP.
3. Increments level by 1.0.
4. Emits `SIG_PLAYER_ADVANCED` signal with `(tp, new_level)`.
5. Returns 1.

### Kill XP Formula — `killXp(object killer, object killed)`

```lpc
xp       = toNextLevel(killed_level) / 10;    // 10% of killed NPC's TNL
variance = xp / 10;
xp       = xp - variance + random(variance);  // ±10% random

level_diff = killer_level - killed_level;

if(level_diff > 5)      // overlevel
    factor -= 0.05 * (level_diff - 5);    // -5% per level over threshold
else if(level_diff < 0) // underlevel
    factor += 0.05 * (-level_diff);       // +5% per level under

xp = to_int(xp * factor);
```

Called from `body.c::die()` for NPC deaths only. If `PLAYER_AUTOLEVEL` is true, `advance()` is called immediately after XP award.

### `earnXp(object tp, int amount)`

Calls `tp->adjustXp(amount)`. If `PLAYER_AUTOLEVEL`, calls `advance(tp)`. The general-purpose XP award function.

## Attributes — `std/living/attributes.c`

Default attributes (from config): `"strength"`, `"dexterity"`, `"constitution"`, `"intelligence"`, `"wisdom"`, `"charisma"`. All initialized to `5`.

| Function | Signature | Description |
|---|---|---|
| `set_attribute` | `int (string key, int value)` | Set directly. Returns new value or null if invalid key |
| `query_attribute` | `int (string key, int raw)` | Returns `value + query_effective_boon("attribute", key)`. If `raw=1`, raw only |
| `modify_attribute` | `int (string key, int value)` | Adjust by delta |
| `query_attributes` | `mapping ()` | Returns copy of all attributes |
| `init_attributes` | `void ()` | Loads from config, initializes missing to 5 |

Like skills, attributes support boon/curse modifiers via `query_effective_boon("attribute", key)`.

## GMCP Events

| Package | When | Fields |
|---|---|---|
| `Char.Status` | `setLevel`, `add_level` | `level`, `xp`, `tnl` |
| `Char.Status` | `adjustXp` | `xp`, `tnl`, `level` |

## Signals

| Signal | When | Payload |
|---|---|---|
| `SIG_PLAYER_ADVANCED` | `advance()` on level-up | `(tp, new_level)` |

## NPC Skill Behavior

NPCs interact with the skill system differently:

1. **`query_skill()` returns `queryLevel() * 3.0`** for all skills — NPCs never use stored skill values for this function.
2. **`npc.c::setLevel()` calls `adjust_skills_by_npc_level()`** which resets all stored skill levels to near-zero (`random_float(0.01)`). This means `query_skill_level()` returns ~0 for NPCs.
3. **Combat formulas use `query_skill_level()`** — so NPC skill effectiveness in the actual hit/damage math comes from stored values (near-zero), not from the `level * 3` shortcut. This is a design tension.
4. **Use-based improvement still fires** for NPCs (defenders train defense skill on hit attempts), but the values are largely irrelevant given `setLevel()` wipes them.

## Gotchas

1. **Skills are a nested tree, not flat.** Using `"combat.melee.slashing"` requires the full path to exist. `add_skill` creates intermediates automatically.
2. **`query_skill()` vs `query_skill_level()`** — different return types, different NPC behavior, different boon handling. Know which one you need.
3. **`setLevel()` on NPCs wipes stored skills.** Always call `setLevel()` before adding custom skills.
4. **Improvement bubbles up.** Using a leaf skill has a chance to improve parent skills too, via weighted path selection.
5. **Progress is tiny.** Each `use_skill` call adds at most `random_float(0.01)` — roughly 0-1% of a level. Skills improve slowly and organically.
6. **Boons affect `query_skill_level()` but NOT `query_skill()`.** A boon on `"combat.melee.slashing"` changes what `query_skill_level` returns but not `query_skill`.
7. **Attributes are currently independent of skills.** They have their own boon class (`"attribute"`) and don't directly modify skill checks — they're tracked but not yet wired into formulas.
