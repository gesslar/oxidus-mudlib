---
name: skills-and-advancement
description: Understand and work with the skill tree and advancement systems in Oxidus. Covers the nested skill tree, dot-path addressing, use-based improvement and the level-based skill cap, the query_skill / query_raw_skill / query_skill_level / query_raw_skill_level API grid, has_skill existence checks, boon integration, XP, TNL formula, leveling, attributes, and how skills interact with combat and NPCs.
---

# Skills and Advancement Skill

You are helping work with the Oxidus skill and advancement systems. Follow the `lpc-coding-style` skill for all LPC formatting.

## Architecture Overview

```
skills.lpc (std/living/skills.lpc)           — nested skill tree, use-based improvement
advancement.lpc (std/living/advancement.lpc) — per-living XP/level state
advance.lpc (adm/daemons/advance.lpc)        — TNL formula, kill_xp, earn_xp
attributes.lpc (std/living/attributes.lpc)   — STR/DEX/CON/INT/WIS/CHA
boon.lpc (std/living/boon.lpc)               — buff/debuff modifiers on skills and vitals
```

All of these are inherited by `STD_BODY` and apply to both players and NPCs.

## Skill System — `std/living/skills.lpc`

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
            "defence": ([
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

A private `find_skill_node(string skill)` helper walks the dot-path and returns the live node mapping (or 0). Every read/leaf-mutate function delegates to it — `add_skill` and `remove_skill` keep their own walks because they need creation / parent-ref semantics.

### Default Skill Tree (from config)

The tree lives under `SKILLS.learnable` in `adm/etc/default.lpml`:

```
combat
  defence: dodge, parry
  melee: attack, bludgeoning, piercing, slashing, unarmed
social: barter, charm, intimidate, persuade
general: appraise, hide, jump, listen, search, spot, swim
```

Full dot-path examples: `"combat.melee.slashing"`, `"combat.defence.dodge"`, `"social.barter"`, `"general.swim"`.

`"combat.defence.evade"` is used by combat but is not in the learnable tree — it is created on first use by `assure_skill()`.

### Improvement Tuning Knobs (config)

The rest of the `SKILLS` block holds the numbers `use_skill` reads on every call:

| Key | Description |
|---|---|
| `SKILLS.improve_chance.floor` | Percent chance floor for a `use_skill` roll |
| `SKILLS.improve_chance.ceiling` | Hyperbolic scale added to the floor; the chance rises toward `floor + ceiling` as the skill grows |
| `SKILLS.default_gain` | Progress bound used when the caller passes no `improvement` |
| `SKILLS.cap_factor` | Multiplied by the living's level to get the per-node skill cap |

`COMBAT.NPC_SKILL_MULTIPLIER` is the matching knob for NPC skill levels.

**Read the current values from `adm/etc/default.lpml`, and never restate them in code, comments, or a call site.** They are tuning knobs and they move; a literal copied out of that file is wrong the next time it is turned. Everything below describes the shape of the maths, not the numbers going into it.

### Key Functions

| Function | Signature | Description |
|---|---|---|
| `add_skill` | `int (string skill, float level)` | Creates skill at dot-path. Intermediates created at level 1.0. **Does not overwrite existing nodes.** Returns 1 on success |
| `remove_skill` | `int (string skill)` | Removes leaf node |
| `has_skill` | `int (string skill)` | Returns 1 if the node exists, 0 otherwise. Use this for existence checks instead of `nullp(query_raw_skill(...))` |
| `query_raw_skill` | `float (string skill)` | Raw float level — no flooring, no boon |
| `query_skill` | `float (string skill)` | Raw float level + boon modifier |
| `query_raw_skill_level` | `float (string skill)` | `floor(level)` — no boon |
| `query_skill_level` | `float (string skill)` | `floor(level)` + `query_effective_boon("skill", skill)`. The function combat math uses |
| `set_skill_level` | `int (string skill, float level)` | Sets exact float level. Requires intermediates to already exist; will not create them |
| `query_skills` | `mapping ()` | Returns a copy of the entire tree |
| `set_skills` | `void (mapping s)` | Replaces the tree wholesale (no-op if `s` is not a mapping) |
| `use_skill` | `int (string skill, mixed improvement)` | Rolls for improvement, chooses a node, clamps the gain, and applies it. Auto-creates the skill if missing. `improvement` overrides the `SKILLS.default_gain` progress bound. See improvement algorithm below |
| `improve_skill` | `float (string skill_name, mixed potential_progress)` | Applies progress to **one** node — no path walk, no bubble-up, no cap. Defaults to `SKILLS.default_gain`. `use_skill` is the entry point; call this directly only when you deliberately want to bypass the cap and the weighted pick |
| `determine_skill_to_improve` | `private string (string skill_name, float skill_cap)` | Builds the node-and-ancestors candidate list, drops any node at or over `skill_cap`, weighted-draws one survivor. `undefined` if all are capped |
| `clamp_improvement` | `private float (string skill_name, float improvement)` | Trims a proposed gain to the distance remaining to that node's cap; `0.0` if already at or over |
| `query_skill_progress` | `int (string skill)` | Fractional part of the level as a 0-99 integer |
| `modify_skill_level` | `int (string skill, int level)` | Replace level with an int. Like `set_skill_level` but accepts int and doesn't enforce a minimum |
| `assure_skill` | `int (string skill)` | Creates at level 1.0 if missing, tells the player they gained a new skill |
| `wipe_skills` | `void ()` | Resets to empty mapping |
| `initialize_missing_skills` | `void (mapping, string)` | Creates any missing skills from a config-shaped tree |
| `adjust_skills_by_npc_level` | `int (float level)` | NPC-only: seeds every skill in the tree to `level * COMBAT.NPC_SKILL_MULTIPLIER`. Errors if called on a user |

### Use-Based Improvement

Players improve skills transparently by using them — no skill points or manual allocation.

`use_skill()` is the only entry point. An unknown skill is created instead of rolled, so the first use costs a call:

```lpc
varargs int use_skill(string skill, mixed improvement) {
    // unknown skill -> assure_skill() at 1.0, no roll this call
    // known skill   -> roll, then pick a node, clamp, improve
}
```

The chance is not flat. It is `improve_chance.floor + dim_hyperbolic(raw, improve_chance.ceiling)`, where `dim_hyperbolic(v, s) == (s * v) / (s + v)`. That starts at the floor, reaches `floor + ceiling/2` when the raw skill equals the ceiling, and approaches `floor + ceiling` asymptotically — **higher** skill means a more frequent roll, and the cap is what slows advancement down.

`use_skill()` is called throughout the codebase:
- Combat: attacker trains weapon skill after each swing, defender trains defence skill on every hit attempt.
- Any system can call `use_skill("general.swim")` etc. to trigger organic improvement.

**`improvement`** replaces the `SKILLS.default_gain` bound for this call. It is a bound, not an award — `improve_skill` applies `random_float()` of it.

Omit it unless the call site genuinely wants to advance at a different rate from everything else, and if you do pass one, **read `SKILLS.default_gain` before choosing the literal** — the argument is an absolute bound, not a multiplier, so whether a given number speeds a call site up or slows it down depends entirely on where the default currently sits. Existing spell and ability sites pass literals (`victim->use_skill("combat.defence.evade", 0.1);`) that were chosen against an older default.

### Improvement Algorithm

Selection and application are separate functions. `use_skill()` orchestrates:

1. **Roll.** `random_float(100.0) < improve_chance.floor + dim_hyperbolic(raw, improve_chance.ceiling)`. Fail -> return 0.
2. **Select** — `determine_skill_to_improve(skill, query_level() * cap_factor)`:
   - Candidates are the skill itself and every ancestor: `"combat.melee.slashing"` -> `({ "combat", "combat.melee", "combat.melee.slashing" })`.
   - Any candidate whose `query_raw_skill()` is at or over the cap is dropped.
   - Survivors are weighted `(segments + 1) * 3` and drawn with `element_of_weighted()`. For a full 3-segment path: leaf 12, middle 9, root 6 — **44% / 33% / 22%**. As parents cap out, the surviving weights redistribute toward the leaf.
   - All capped -> `undefined`, and `use_skill` returns 0 even though the roll succeeded.
3. **Clamp** — `clamp_improvement(chosen, improvement)` trims the bound to `cap - current`, so a near-cap node gets a proportionally smaller bound.
4. **Apply** — `improve_skill(chosen, clamped)` coerces the bound to a float (float as-is, int promoted, functional evaluated against `this_object()`, omitted -> `SKILLS.default_gain` via `??=`), adds `random_float(bound)` to that node, and notifies the player if the floored level rose.

This means using `"combat.melee.slashing"` can also improve `"combat.melee"` or `"combat"` — but with lower probability. **Parent skills grow organically as their children are used, but more slowly because they are picked less often.**

**The cap is `query_level() * SKILLS.cap_factor`** — base level, not `query_effective_level()`, so a level boon does not raise the ceiling. Progress rolls freely over intervening level boundaries; only the cap halts it, and the only way to lift it is to level.

### Query API Grid

Four query functions form an orthogonal grid over two axes: floored vs raw float, and with-boon vs without-boon.

|         | Raw float                      | Floored (combat math)                |
|---------|--------------------------------|--------------------------------------|
| No boon | `query_raw_skill(s)` → 3.47    | `query_raw_skill_level(s)` → 3.0     |
| + boon  | `query_skill(s)` → 3.47 + boon | `query_skill_level(s)` → 3.0 + boon  |

Pick by what the call site actually wants:

- **Hit/damage formulas** → `query_skill_level()`. Combat math.
- **Proc rolls / scaling on fractional progress** → `query_raw_skill()` (e.g. multi-strike in `swing()`) or `query_skill()` if buffs should help.
- **Existence check** → `has_skill(s)` — returns 1 or 0. Do not use `nullp(query_raw_skill(...))` for this.

The four functions return `null` if the skill is not found — with one exception. **On an NPC, `query_skill()` and `query_skill_level()` do not read storage at all**: they synthesize a node at `query_effective_level() * COMBAT.NPC_SKILL_MULTIPLIER`, add the boon, and return it for any skill name whatsoever. Only `query_raw_skill()` and `query_raw_skill_level()` walk an NPC's actual tree, and only `has_skill()` is a valid existence check on an NPC.

### Boon Integration

The `query_skill` and `query_skill_level` variants add the effective boon modifier:

```lpc
// query_skill_level
return floor(level) + query_effective_boon("skill", skill);
// query_skill
return level + query_effective_boon("skill", skill);
```

Where `query_effective_boon("skill", "combat.melee.slashing")` = sum of boons minus sum of curses for that skill class+type. See the `buff-system` skill for details on applying boons. Use the `query_raw_*` variants when you explicitly want to bypass boons (e.g. introspection commands, raw progression UI).

### Skills Used by the Combat System

| Skill | Where Used |
|---|---|
| `"combat.melee"` | Multi-strike chance in `swing()` (uses `query_skill_level` — floored level + boons) |
| `"combat.melee.<type>"` | Hit chance and damage formulas |
| `"combat.melee.unarmed"` | Unarmed combat fallback |
| `"combat.defence.dodge"` | Melee defence in hit chance |
| `"combat.defence.evade"` | Spell defence in hit chance |
| `"combat.defence"` | Generic defence reduction in damage formula |

## XP and Advancement

### Per-Living State — `std/living/advancement.lpc`

| Variable | Type | Default | Description |
|---|---|---|---|
| `__level` | `float` | `1.0` | Current level |
| `__level_mod` | `float` | `0.0` | Temporary level modifier (boons / curses) |
| `__xp` | `int` | `0` | Accumulated experience points |

### Functions

| Function | Signature | Description |
|---|---|---|
| `query_xp` | `int ()` | Returns `__xp` |
| `query_level` | `float ()` | Returns `__level` |
| `query_effective_level` | `float ()` | Returns `__level + __level_mod`. Used throughout combat math |
| `query_tnl` | `float ()` | Returns `ADVANCE_D->to_next_level(__level)` |
| `set_level` | `float (float l)` | Sets `__level`. Sends GMCP `Char.Status` if user |
| `adjust_level` | `float (float l)` | Adds delta to `__level`. Sends GMCP if user |
| `query_level_mod` | `float ()` | Returns the temporary modifier |
| `set_level_mod` | `float (float l)` | Sets the temporary modifier (routes through `adjust_level_mod`) |
| `adjust_level_mod` | `float (float l)` | Adjusts modifier by delta |
| `adjust_xp` | `int (int amount)` | Adds delta to `__xp`. Sends GMCP if user |
| `set_xp` | `int (int amount)` | Sets `__xp` to `amount` via `adjust_xp(amount - __xp)` |
| `on_advance` | `void (object tp, float l)` | Slot for `SIG_PLAYER_ADVANCED`. Tells the player they have advanced |

### Advancement Daemon — `adm/daemons/advance.lpc`

Config constants:

| Key | Description |
|---|---|
| `BASE_TNL` | XP for level 1 to 2 |
| `TNL_RATE` | Geometric multiplier per level |
| `PLAYER_AUTOLEVEL` | Auto-advance on XP gain |
| `OVERLEVEL_THRESHOLD` | Levels above target before the XP penalty applies |
| `OVERLEVEL_XP_PUNISH` | Fraction of XP lost per level over the threshold |
| `UNDERLEVEL_THRESHOLD` | Levels below target before the XP bonus applies |
| `UNDERLEVEL_XP_BONUS` | Fraction of XP gained per level under the threshold |

Current values live in `adm/etc/default.lpml`.

### TNL Formula

```lpc
to_next_level(level) = to_int(BASE_TNL * pow(TNL_RATE, level - 1.0))
```

Geometric: each level costs `TNL_RATE` times the last. Call `ADVANCE_D->to_next_level()` for a real number rather than working one out from a table.

### `advance(object tp)`

1. Checks `can_advance(xp, level)` — is XP >= TNL?
2. Deducts TNL from XP.
3. Increments level by 1.0.
4. Emits `SIG_PLAYER_ADVANCED` signal with `(tp, new_level)`.
5. Returns 1.

### Kill XP Formula — `kill_xp(object killer, object killed)`

```lpc
xp       = to_next_level(killed_level) / 10;  // 10% of killed NPC's TNL
variance = xp / 10;
xp       = xp - variance + random(variance);  // ±10% random

level_diff = killer_level - killed_level;

if(level_diff > OVERLEVEL_THRESHOLD)
    factor -= OVERLEVEL_XP_PUNISH * (level_diff - OVERLEVEL_THRESHOLD);
else if(level_diff < UNDERLEVEL_THRESHOLD)
    factor += UNDERLEVEL_XP_BONUS * (-level_diff);

xp = to_int(xp * factor);
```

Called from `body.lpc::die()` for NPC deaths only. If `PLAYER_AUTOLEVEL` is true, `advance()` is called immediately after XP award.

### `earn_xp(object tp, int amount)`

Calls `tp->adjust_xp(amount)`. If `PLAYER_AUTOLEVEL`, calls `advance(tp)`. The general-purpose XP award function.

## Attributes — `std/living/attributes.lpc`

Default attributes (from config): `"strength"`, `"dexterity"`, `"constitution"`, `"intelligence"`, `"wisdom"`, `"charisma"`. All initialized to `5`.

| Function | Signature | Description |
|---|---|---|
| `set_attribute` | `int (string key, int value)` | Set directly. Returns new value or null if invalid key |
| `get_attribute` | `int (string key, int raw)` | Returns `value + query_effective_boon("attribute", key)`. If `raw=1`, raw only |
| `modify_attribute` | `int (string key, int value)` | Adjust by delta |
| `get_attributes` | `mapping ()` | Returns copy of all attributes |
| `init_attributes` | `void ()` | Loads from config, initializes missing to 5 |

Like skills, attributes support boon/curse modifiers via `query_effective_boon("attribute", key)`.

## GMCP Events

| Package | When | Fields |
|---|---|---|
| `Char.Status` | `set_level`, `adjust_level` | `level`, `xp`, `tnl` |
| `Char.Status` | `adjust_xp` | `xp`, `tnl`, `level` |

## Signals

| Signal | When | Payload |
|---|---|---|
| `SIG_PLAYER_ADVANCED` | `advance()` on level-up | `(tp, new_level)` |

## NPC Skill Behaviour

NPCs interact with the skill system through the same code paths as players — no shortcuts, no special-case branching in `query_*` functions:

1. **`npc.lpc::set_level()` calls `adjust_skills_by_npc_level()`**, which seeds every skill in the tree to `level * COMBAT.NPC_SKILL_MULTIPLIER`.
2. **Combat formulas use `query_skill_level()`**, which on an NPC bypasses that storage entirely and returns `query_effective_level() * COMBAT.NPC_SKILL_MULTIPLIER + boon` — for *any* skill name, known or not. The seeded storage is what the `_raw_` queries and use-based improvement see.
3. **Use-based improvement still fires** for NPCs (defenders train defence skill on hit attempts), but `set_level()` reseeding overwrites any accumulated progress, so improvements are transient — and since combat reads the synthesized value, they do not affect NPC combat performance either way.

## Gotchas

1. **Skills are a nested tree, not flat.** Using `"combat.melee.slashing"` requires the full path to exist. `add_skill` creates intermediates automatically but does NOT overwrite existing nodes.
2. **Pick the right query.** Four-way grid: `query_raw_skill` / `query_skill` / `query_raw_skill_level` / `query_skill_level`. Combat math uses `query_skill_level()`. For existence checks use `has_skill()` — not `nullp(query_raw_skill(...))`.
3. **`set_level()` on NPCs reseeds stored skills.** Always call `set_level()` before adding custom NPC skills.
4. **Improvement bubbles up.** Using a leaf skill has a chance to improve parent skills too, via the weighted pick in `determine_skill_to_improve()` — the pick is the only bubble-up mechanic. Exactly one node is improved per successful roll.
5. **Skills cap at `query_level() * SKILLS.cap_factor`.** Level up to raise the ceiling; a level *boon* will not, because the cap reads `query_level()`, not `query_effective_level()`. A `use_skill` whose roll succeeds but whose candidates are all capped returns 0 and awards nothing.
6. **`improvement` is an absolute bound, not an award or a multiplier.** `improve_skill` applies `random_float(bound)`, and the bound defaults to `SKILLS.default_gain`. A literal passed at a call site is only faster or slower relative to whatever that config key currently holds — check it before picking one, and prefer omitting the argument.
7. **Boons apply to `query_skill` / `query_skill_level`, not the `_raw_` variants.** A boon on `"combat.melee.slashing"` changes what `query_skill_level` and `query_skill` return but never mutates the stored value.
8. **Attributes are currently independent of skills.** They have their own boon class (`"attribute"`) and don't directly modify skill checks — they're tracked but not yet wired into formulas.
9. **`improve_skill` no longer selects a node.** It applies progress to exactly the dot-path it is handed, with no cap check. Selection and clamping live in `use_skill`, so calling `improve_skill` directly bypasses both. Use `use_skill()` unless that bypass is the point.
10. **Defaults come from config, not from a default-arg functional.** `improve_skill` resolves an omitted bound with `potential_progress ??= mud_config("SKILLS.default_gain")`. The `valid_function()` branch only fires when a caller explicitly passes a functional.
11. **Spelling is `defence` everywhere.** The config tree, the dot-paths (`"combat.defence.dodge"`, `"combat.defence.evade"`), and the armour-side identifiers (`set_defence`, `query_defence_amount`, `__defence`) all use the Canadian spelling. There is no `defense` anywhere in the lib.
