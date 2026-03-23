---
name: buff-system
description: Understand and work with the buff/debuff system in Threshold RPG. Covers all 12 buff types, gear buffs, temporary buffs, special categories, room buffs, gem buffs, buff objects, stacking, expiration, and downstream effects on stats, combat, and regen.
---

# Buff System Skill

The buff/debuff system is a core cross-cutting mechanic in Threshold RPG. Buffs modify player stats, skills, resistances, regeneration, and more. They originate from many sources — gear, spells, potions, rooms, gems, guild abilities, and administrative systems.

**Core files:**
- `std/living/buffs.c` — player buff system
- `std/living/buffsnpc.c` — simplified NPC version

## Buff Types

There are exactly **12 valid buff types** (defined in `std/living/buffs.c`):

| Type | What It Affects | Update Trigger |
|---|---|---|
| `stat` | Core character stats | `update_stats()` + conditionally `update_life_data()` |
| `skill` | Skill values | None (lazy, queried on demand) |
| `spell` | Spell power values | None (lazy, queried on demand) |
| `resist` | Damage resistances | `update_resists()` |
| `max` | Maximum HP/SP/EP | `update_life_data()` |
| `regen` | Out-of-combat regeneration | `update_regen()` |
| `combatregen` | In-combat regeneration | `update_regen()` |
| `armor` | Armor class bonuses | `update_misc()` |
| `bonus` | Miscellaneous bonuses (hit, crit, gather, etc.) | `update_misc()` |
| `pen` | Penetration (armor bypass) | `update_misc()` |
| `resource` | Resource-related bonuses | `update_misc()` |
| `status` | Status effects | `update_misc()` |

Any buff label is formatted as `"type_name"` — the type and name joined by underscore. Spaces in names become underscores.

## Valid Names by Type

### stat

`strength`, `dexterity`, `constitution`, `intelligence`, `wisdom`, `charisma`, `luck`, `max_capacity`

Constitution, intelligence, and wisdom trigger additional `update_life_data()` because they affect max HP/SP/EP.

### skill

Any skill name: `attack`, `defense`, `blunt_weapons`, `cutting_weapons`, `thrusting_weapons`, `flexible_weapons`, `missile_weapons`, `offhand_weapons`, `adventuring`, etc.

Queried lazily via `query_bonus_total("skill_"+name)` + `query_bonus_total("skill_all")`.

### spell

Any spell school: `combat`, `divination`, `elemental`, `enchantment`, `healing`, `nature`, `voice`, `instruments`, `agility`, etc.

Also supports `bonus_<element>_spell` and `bonus_all_spell` for spell damage bonuses.

### resist

**Physical:** `blunt`, `cutting`, `missile`, `thrusting`, `flexible`

**Magical:** `air`, `earth`, `fire`, `water`, `void`, `magic`, `psionic`

**Generic:** `melee`, `spell`, `weapon`, `all`, `healing`, `armor`

**Conversions applied automatically:** acid/cold/poison -> water, disease -> earth, electricity/gas -> air, vampiric -> void.

Resists are capped at 100% and have diminishing returns applied via `diminish()`.

### max

`hp`, `sp`, `ep`, `tummy`, `capacity`

### regen

`hp`, `sp`, `ep`, `tummy`, `heal_amount` (healing pool), `heal_bank`

### combatregen

`hp`, `sp`, `ep`

### bonus

| Name | Effect |
|---|---|
| `hit` | Added to hit chance in combat |
| `defense` | Defense skill bonus |
| `melee_damage` | Direct damage increase |
| `natural_armor_class` | Added to AC calculation |
| `critical_strike` | Crit chance percentage |
| `style_chance` | Parry/defensive crit chance |
| `dodge_amount` | Reduces attacker's hit chance |
| `thorns` | Reciprocal damage when hit in melee |
| `gather_speed` | Gathering speed |
| `gather_chance` | Gathering success rate |
| `gather_amount` | Resources per gather |
| `experience` | XP multiplier |
| `world_drop` | Drop rate bonus |
| `hate_percent` | Threat generation percentage |
| `hate_value` | Threat generation flat value |
| `fishing` | Fishing bonus |
| `comfort` | Comfort level |

### armor

`armor_class`, `natural_armor_class`

### pen

Penetration reduces target's effective armor class. Uses `resist_armor` internally — `ac -= percent_of(pen, ac)`.

### status

Status effects tracked by the system: `stun`, `hold`, `root`, `mez`, `confuse`, `daze`, `ignite`, `chill`, `ionize`, `stagger`, `bleeding`, `unbalance`, `snare`. These are largely managed by the separate `std/living/status_effects.c` system.

### resource

Valid but appears primarily informational. Gathering mechanics use `bonus_gather_*` types instead.

## Buff Sources

### 1. Gear Buffs (Equipment)

Applied when armor, weapons, clothing, held items, or ornaments are equipped/wielded.

**Equipment side** (e.g., in armor/weapon `setup()`):
```lpc
set("buffs", ([
  "stat_strength"  : 2,
  "resist_fire"    : 3,
  "skill_defense"  : 1,
]));
```

**Lifecycle:**
- On equip: equipment calls `gear_buff(tp)` which parses each `"type_name"` key and calls `tp->gear_buff(type, name, amount)`
- On unequip: equipment calls `gear_unbuff(tp)` which calls `tp->remove_gear_buff(type, name, amount)`

Debuffs work the same way via `set("debuffs", ...)`.

**Equipment types that support gear buffs:** armor (`std/armor.c`), weapons (`std/weapon.c`), clothing (`std/clothing.c`), held items (`std/held.c`), ornaments (`std/ornament.c`).

### 2. Temporary Buffs (Spells, Abilities, Potions)

Applied by guild abilities, potions, food, and other game systems.

```lpc
// Apply a buff: name, type, what, amount, duration_seconds, wear_off_message
tp->buff("Thorns", "bonus", "thorns", 15, 60, "The thorns fade away.\n");

// Apply a debuff:
tp->debuff("Weakness", "stat", "strength", 3, 30, "You feel your strength return.\n");
```

- Duration of `-1` = permanent (never expires)
- Buffs with the same name replace each other (old one purged automatically)
- Each buff gets a unique ID: `buff_<nanosecond_timestamp>`

**Expiration:** `process_expirations()` runs on heartbeat, removes expired buffs and shows wear-off messages.

**Persistence:** Active buffs are saved on logout via `persist_all_buff_debuff()` and restored on login via `reapply_buffs_debuffs()`. Only non-expired buffs are restored.

### 3. Special Category Buffs

Five special categories for large-scale group buffs:

| Category | Stored In | Use |
|---|---|---|
| `kingdom` | `kingdom_buffs` / `kingdom_debuffs` | Kingdom-wide effects |
| `clan` | `clan_buffs` / `clan_debuffs` | Clan-wide effects |
| `god` | `god_buffs` / `god_debuffs` | Divine effects |
| `admin` | `admin_buffs` / `admin_debuffs` | Administrative buffs |
| `event` | `event_buffs` / `event_debuffs` | Event-based effects |

```lpc
tp->special_buff("event", "Harvest Festival", "bonus", "experience", 10, 3600, "The festival blessing fades.\n");
tp->remove_special_buff("event", buff_id);
```

### 4. Room Buffs (Environmental)

Applied by rooms/locations for area-based effects.

```lpc
tp->room_buff("resist_fire", 5);      // +5 fire resist in this room
tp->remove_room_buff("resist_fire");   // removed when leaving
tp->clear_room_buffs();                // remove all room buffs
```

Non-stacking: returns 0 if a room buff with that label already exists.

### 5. Gem Buffs (Belt System)

Processed by `BELT_D` (`adm/daemons/belt.c`). Gems in the player's belt provide passive bonuses.

- Gems stored in `belt/gems` array
- Each gem maps to effects based on quality and type
- **Diminishing returns** applied when multiple gems provide the same bonus
- Final values stored in `gem_buffs` mapping on the player
- Included in `query_bonus_total()` calculations

### 6. Buff Objects (Animated/Persistent)

Objects that move into player inventory and provide ongoing effects.

```lpc
// Apply a buff object
tp->buff_object(buff_ob, "Bone Armor", 120);  // 120 second duration

// Remove
tp->remove_buff_object(buff_ob);  // calls buff_ob->expire_buff(tp)
```

Used by guild abilities like Bone Armor, Swift Wind, Mirror Image, Totem buffs, Juggernaut, etc. The buff object lives in the player's inventory and is destroyed on expiration.

### 7. Permanent Bonuses

Stored in `perm_bonus/<label>` on the player. Always included in `query_bonus_total()`. Not managed through the buff/debuff API — set directly on the player object.

## Bonus Calculation

All buff values funnel through `query_bonus_total(label)`:

```
Total = query_temp("bonus/" + label)    // temporary buff bonuses
      + query("gem_buffs/" + label)     // gem belt bonuses
      + query("perm_bonus/" + label)    // permanent bonuses
```

Gear buffs, temporary buffs, room buffs, and special category buffs all write to `bonus/<label>` in temp storage via `update_bonuses()`.

## Stacking Rules

- **Gear buffs** from multiple items stack additively
- **Temporary buffs** with the same name replace each other (purge-then-apply)
- **Temporary buffs** from different names coexist and stack
- **Gem buffs** use diminishing returns for same-type bonuses
- **Room buffs** do not stack (same label returns 0)
- **Special category buffs** (kingdom, clan, god, admin, event) are tracked separately and stack with all other sources

## Downstream Effects

When `update_bonuses()` applies a buff, it triggers the appropriate update function:

### stat -> update_stats()
Reads `perm_stat` base + `query_bonus_total("stat_"+name)` for each stat. Constitution/intelligence/wisdom also trigger `update_life_data()` since they affect max HP/SP/EP.

### resist -> update_resists()
Combines `perm_resist` + bonus + stat-derived resists (INT adds to fire/air/water/magic, WIS adds to psionic/void, CON adds to earth/water). Diminishing returns applied. Capped at 100%.

### max -> update_life_data()
Adds `query_bonus_total("max_hp/sp/ep")` to base max values. Current vitals are capped to new maximums.

### regen / combatregen -> update_regen()
Adds bonus to base regen rates. Regen applies out of combat; combatregen applies during combat.

### bonus -> update_misc()
Updates derived properties: `hit_bonus`, `defense_bonus`, `natural_armor_class`, `critical_strike`, `gather_speed/chance/amount`, etc.

### skill / spell -> (no trigger)
Queried lazily. When combat or spells call `query_real_skill()` or `query_spell()`, the bonus is included at query time.

## Setting Buffs on Gear

When creating armor, weapons, clothing, or other equippables, use the `"buffs"` and `"debuffs"` mappings:

```lpc
// In setup():
set("buffs", ([
  "stat_strength"     : 2,
  "stat_constitution" : 1,
  "resist_fire"       : 5,
  "skill_attack"      : 2,
  "max_hp"            : 20,
  "regen_hp"          : 1,
  "bonus_critical_strike" : 2,
]));

set("debuffs", ([
  "stat_dexterity" : 1,  // -1 dexterity while worn
]));
```

The equipment's `gear_buff()` / `gear_unbuff()` handles the rest automatically.

## Applying Temporary Buffs in Code

```lpc
// Simple stat buff for 60 seconds
tp->buff("Iron Will", "stat", "constitution", 3, 60, "Your iron will fades.\n");

// Permanent skill buff (duration -1)
tp->buff("Master Training", "skill", "attack", 5, -1);

// Resist buff
tp->buff("Fire Ward", "resist", "fire", 10, 120, "The fire ward dissipates.\n");

// Debuff
tp->debuff("Chill", "stat", "dexterity", 2, 30, "The chill passes.\n");

// Special category buff
tp->special_buff("event", "Double XP Weekend", "bonus", "experience", 100, 7200);

// Remove by name
tp->purge_buff_by_name("Iron Will");

// Find buff IDs by name
string *ids = tp->find_buff_by_name("Fire Ward");
```

## GMCP Integration

Buff changes send GMCP notifications to the client:
- `GMCP_PKG_CHAR_BUFFS_ADD` — buff applied
- `GMCP_PKG_CHAR_BUFFS_REMOVE` — buff removed
- `GMCP_PKG_CHAR_BUFFS_LIST` — full buff list
- `GMCP_PKG_CHAR_DEBUFFS_ADD/REMOVE/LIST` — debuff equivalents

Each message includes: buff_id, name, kind (temp/object), category, what, and expiration time.

## NPC Buffs

NPCs use `std/living/buffsnpc.c` — a simplified version:
- Same `gear_buff()` / `remove_gear_buff()` interface
- Same `VALID_TYPES`
- No gem buffs, no special categories, no GMCP
- No expiration processing (NPCs don't track buff timers)
- `query_bonus_total()` returns only temp bonus (no gem/perm layers)
