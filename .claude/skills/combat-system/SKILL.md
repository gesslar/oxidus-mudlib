---
name: combat-system
description: Understand and work with the combat system in Oxidus. Covers the attack loop, hit-chance formula, damage pipeline, threat tracking, defence/AC, procs, vitals/regen, death sequence, XP/advancement, NPC combat behaviour, and combat memory.
---

# Combat System Skill

You are helping work with the Oxidus combat system. Follow the `lpc-coding-style` skill for all LPC formatting.

## Architecture Overview

```
body.lpc
  └── combat.lpc          — attack loop, threat, hit chance, AC, defence
        └── damage.lpc     — deliver/receive damage formulas

vitals.lpc                 — HP/SP/MP, regen, condition strings
advancement.lpc            — per-living XP/level state
npc.lpc                    — NPC heartbeat, death detection, set_level override
proc.lpc (module)          — weapon proc system
combat_memory.lpc (module) — NPC "remember and attack on sight"

advance.lpc (daemon)       — TNL formula, kill_xp, earn_xp, advance
death.lpc (daemon)         — signal listener for SIG_PLAYER_DIED/REVIVED
```

### Inheritance Chain

```
body.lpc → combat.lpc → damage.lpc
npc.lpc  → body.lpc (STD_BODY)
```

## Combat Round Loop

`combat_round()` is rescheduled by `next_round()` via `call_out_walltime` at
`__attack_speed` seconds. There is no random jitter on the interval.

```
combat_round()
  ├── is_dead() / hp <= 0        — bail, stopping combat on the hp case
  ├── clean_up_enemies()         — remove dead/gone enemies
  ├── in_combat()                — no enemies left: clear the handle and stop
  ├── highest_threat() → victim  — pick target with most accumulated threat
  ├── valid_enemy(victim)        — must be in same room and alive
  ├── display_health_bar()       — when mp >= 0.0
  ├── swing()                    — execute attack(s)
  ├── GMCP Char.Status update    — current_enemy, current_enemy_health, current_enemies
  └── next_round()               — only if no round is already scheduled
```

### Multi-Strike Logic (`swing`)

The extra swing is **dual wield only** — it is rolled from the off-hand set, so a
body with nothing in its off hand never rolls at all. A two-handed weapon
occupies every slot it needs with the same object, and is excluded by object as
well as by slot, so it does not count as its own off hand.

```lpc
float multi_chance = 15.0 + floor(query_skill_level("combat.melee") / 5.0);

if(random_float(100.0) < multi_chance)
    next_multi = 1;   // next recursive call draws from the off-hand weapons
```

When `next_multi` is set, `element_of()` picks from the distinct off-hand
weapons. The function recurses with the count decremented, then incremented back
for the off-hand swing, so the extra swing is additive.

## Hit Chance Formula — `can_strike()`

```lpc
float chance = mud_config("COMBAT.DEFAULT_HIT_CHANCE");

if(enemy->query_mp() <= 0.0)
    chance += 50.0;    // enemies out of movement points are far easier to hit

chance = chance
    + (lvl - vlvl)                               // effective levels
    + skill                                      // query_skill_level(skill_name)
    - (ac * 2.0)                                 // armour class doubled
    - enemy->query_skill_level(defence_skill);   // dodge or evade

// Squash the raw number before rolling against it.
chance = 50.0 + dim_sigmoid(chance - 50.0, 45.0, 0.03);

result = random_float(100.0);
enemy->use_skill(defence_skill);   // defender trains defence on every attempt
return result < chance;
```

**The sigmoid is load-bearing and easy to miss.** The linear sum is passed
through `dim_sigmoid()` centred on 50, so the result is squeezed into roughly
5–95 and never reaches certainty in either direction. Stacked bonuses have
sharply diminishing returns, a hopeless attacker still connects occasionally,
and no amount of AC makes a defender untouchable. Any reasoning about hit rates
that works from the linear sum alone will be wrong.

**Skill routing by weapon argument:**

| `weapon` | Attacker Skill | Defender AC | Defender Skill |
|--------|---------------|-------------|----------------|
| Object, or null (unarmed / NPC defaults) | `query_weapon_info()["skill"]` | `query_ac()` | `"combat.defence.dodge"` |
| String starting `"arcane."` | the string itself | `query_spell_ac()` | `"combat.defence.evade"` |
| Any other string | the string itself | `query_spell_ac()` | `"combat.defence.dodge"` |

Anything else returns `0`. The arcane test is `strsrch(weapon, "arcane.") == 0`
— a **prefix** match on the skill path, not a substring search.

## Damage Formula — `calculate_damage()`

Lives in `std/living/damage.lpc`, not in `combat.lpc`. `strike_enemy()` calls it,
and so does the NPC proc path in `std/living/proc.lpc`.

```lpc
public varargs float calculate_damage(object enemy, mixed weapon_or_skill) {
  // weapon_or_skill: an object, or a skill dot-path string, or nothing
  string skill_name = stringp(weapon_or_skill)
                      ? weapon_or_skill
                      : weapon_info["skill"];

  float skill      = query_skill_level(skill_name);
  float base       = mud_config("COMBAT.BASE_DAMAGE");
  float subtracted = percent_of(mud_config("COMBAT.DAMAGE_VARIANCE"), base);

  base -= subtracted;
  base += random_float(subtracted);        // base ± the variance band

  if(enemy->query_mp() < 0.0)
    base += 4.0;                           // bonus vs a spent defender

  // "arcane." prefix routes to evade, everything else to dodge
  return base
       + weapon_info["base"]                              // the weapon's own damage
       + (skill + query_skill_level("combat") / 2.0)      // specific + half general
       - enemy->query_skill_level(defence_skill);
}
```

**There are no level terms and no max-HP term.** Damage does not scale off the
defender's health pool and the level gap does not enter here — the level
adjustment happens victim-side in `receive_damage()`. Any reasoning that assumes
"a hit takes about 5% of the target" is out of date.

`strike_enemy()` then, in order:

1. `use_skill(skill_name)` — the attacker trains the weapon skill.
2. `if(dam < 0.0) dam = 1.0;` — a **negative** result becomes 1. This is not a
   floor at 1: a result of exactly `0.0` is left alone and lands as a zero-damage hit.
3. Messaging via `MESS_D->get_message("combat", wtype, to_int(ceil(dam)))` and `ACTION_D->action(...)`, tagged `MSG_COMBAT_HIT`.
4. `deliver_mundane_damage(enemy, dam, wtype)` — dispatches to the enemy.
5. `adjust_mp(-random_float(5.0))` — each swing costs 0-5 movement points.
6. `add_threat(enemy, dam)` **and** `add_seen_threat(enemy, dam)`.
7. Weapon proc, via `call_if(weapon, "can_proc")` then `call_if(weapon, "proc", ...)`.
8. NPC proc, via `can_proc()` on the NPC itself then `proc_npc(tag)`.

`valid_enemy(enemy)` is re-tested between the later steps, so a target that dies
mid-sequence stops the rest of it.

## Damage Reception — `receive_damage()`

```lpc
def = query_defence_amount(type);         // type-specific armour value
red = percent_of(def, damage);            // def% of damage is reduced

mod = mud_config("COMBAT.DAMAGE_LEVEL_MODIFIER");
level_difference = attacker_level - defender_level;
mod = mod * level_difference;
red -= percent_of(mod, damage);           // level gap reduces/increases armour effectiveness

damage -= red;
if(damage < 0.0) damage = 0.0;

// Shield effects absorb last, after armour and the level adjustment.
float *reduced = module("shielding", "mitigate_damage", damage, type);
if(!nullp(reduced))
    damage -= max(sum(reduced), 0.0);
```

Then applies `adjust_hp(-damage)`, sets `last_damaged_by`, and sets `killed_by`
if that took the target to zero. Returns the damage actually dealt.

Two early exits: a null attacker, and a target already at or below 0 HP, both
return `0.0` — so damage never lands twice on something already dead.

**Level modifier effect:** Higher-level attackers reduce the defender's armour effectiveness. Lower-level attackers increase it.

## Threat System

Threat values start at `1.0` on `start_attack()` and grow by damage dealt.

| Function | Description |
|---|---|
| `add_threat(ob, float)` | Increments `__current_enemies[ob]` |
| `add_seen_threat(ob, float)` | Increments `__seen_enemies[ob]` (persists across rounds) |
| `highest_threat()` | Returns enemy with highest threat — combat target |
| `lowest_threat()` | Returns enemy with lowest threat |

`__current_enemies` is the active combat mapping. `__seen_enemies` tracks all-time threat. Both start an enemy at `1.0` in `start_attack()`.

## Defence / AC System — `adjust_protection()`

Called whenever equipment changes. Rebuilds `__defence` and `__ac` from all equipped items:

```lpc
mapping adjust_protection() {
    // iterates query_equipped() values
    // sums ob->query_defence() mappings into __defence per damage type
    // sums ob->query_ac() into _ac
}
```

| Function | Description |
|---|---|
| `query_ac()` | Returns aggregated `_ac` float |
| `query_defence_amount(string type)` | Returns `__defence[type]` for a specific damage type |
| `set_defence(mapping)` | Directly sets `__defence` |
| `add_defence(string type, float amount)` | Adds a single type entry |

## Starting and Stopping Combat

### `start_attack(object victim)`

1. Adds victim to `__current_enemies` with threat `1.0`. Returns `0` early if already engaged.
2. Adds to `__seen_enemies` if not already there.
3. For NPCs (`npcp()`): `module("combat_memory", "add_to_memory", victim)`.
4. Schedules `combat_round` via `next_round()`, unless a round is already booked.
5. Calls `victim->start_attack(this_object())` — mutual engagement.

### `stop_all_attacks()`

Cancels combat call out, clears `_current_enemies`, sends GMCP clear.

### `prevent_combat(object victim)`

Returns error string if:
- `victim->query_peaceful()` is set
- `victim->query_no_combat()` is set
- `environment()->query_no_combat()` is set

Returns `1` if combat is allowed.

## NPC Weapon Properties

Used when the NPC has no wielded weapon object:

| Function | Default | Description |
|---|---|---|
| `set_damage(float)` | `0.0` | Base damage. If `<= 0`, uses `random_float(query_level() * 2.0)` |
| `set_weapon_name(string)` | `"fist"` | Display name for combat messages |
| `set_weapon_type(string)` | `"bludgeoning"` | Damage type string |

`query_weapon_info(weapon)` transparently handles both real weapon objects and NPC defaults, returning `([ "name", "type", "skill", "base" ])`.

## Proc System — `std/ext/proc.lpc`

Weapons inherit this module to add special effects on hit. NPCs carry procs
directly through `std/living/proc.lpc`, dispatched by `proc_npc()` — see the
`npc-creation` skill.

### Proc Data Structure

```lpc
__procs[name] = ([
    "function" : string | function,   // required
    "cooldown" : int,                  // seconds, defaults from __proc_cooldown
    "weight"   : int,                  // selection weight, defaults from __proc_weight
])
```

There is **no per-proc chance key**. A bare string or functional passed to
`add_proc()` is wrapped into this shape, and the cooldown and weight defaults
are filled in then.

### Key Functions

| Function | Description |
|---|---|
| `add_proc(string name, mixed proc)` | Register a proc (mapping, string, or function) |
| `set_procs(mixed *procs)` | Batch add: `({ ({ name, proc }), ... })` |
| `set_proc_chance(float)` | Global proc chance, 0.0-100.0 (default 15.0) |
| `can_proc()` | Called by `strike_enemy`. Rolls the global chance first, then filters by cooldown and selects via `element_of_weighted`. Returns proc name or false |
| `proc(string name, mixed args...)` | Executes the proc function with `(attacker, victim)` args. Records cooldown |

### Integration

```lpc
// In strike_enemy():
if(weapon && weapon->is_weapon())
    if(stringp(proc = call_if(weapon, "can_proc")))
        call_if(weapon, "proc", proc, this_object(), enemy);
```

`can_proc()` returns false immediately when `__proc_chance <= 0.0` or when
`random_float(100.0)` exceeds it, so the global chance gates every roll. A proc
whose `cooldown` is `0` is **never** eligible — the cooldown filter only
considers entries with `cooldown > 0`.

## Vitals — `std/living/vitals.lpc`

### Variables

All `private nomask float`, defaults `100.0`:

| Variable | Description |
|---|---|
| `hp` / `max_hp` | Hit points |
| `sp` / `max_sp` | Skill points — what spells and abilities cost |
| `mp` / `max_mp` | Movement points — walking and every combat swing spend them |
| `dead` | `int`, death flag |

### Query Functions

`query_max_hp()`, `query_max_sp()`, `query_max_mp()` all add boon modifiers unless called with `raw = 1`:

```lpc
return max_hp + query_effective_boon("vital", "max_hp");
```

`hp_ratio()`, `sp_ratio()`, `mp_ratio()` return percentage floats.

### Regen — `heal_tick()`

- Fires every `__regen_interval_pulses` heartbeats, derived at setup from
  `HEART_PULSE` × `HEARTBEATS_TO_REGEN`. A `force` argument ticks it regardless
  without resetting the counter.
- **HP and SP are suppressed during combat. MP is not** — movement points
  regenerate mid-fight, which is deliberate: bottoming out on MP adds a large
  bonus to everyone's chance to hit you (`can_strike()`), and the hole would be
  otherwise inescapable.
- Rates come from the race module: `module("race", "query_regen_rate")`.
- **No race module still regenerates.** The fallback is a flat rate for all
  three pools, and it branches on `pcp()` — player characters get the low rate,
  everything else a much higher one.
- Out of combat, a change in any pool calls `display_health_bar()`. Completion
  messages are gated on the `recovery_messages` preference.

### Condition Strings

`query_condition_string()` returns `({ hp_string, sp_string, mp_string })`:

**HP:** "dead" (0) → "critical" (10) → "severely injured" (30) → "moderately injured" (45) → "injured" (60) → "hurt" (75) → "wounded" (90) → "bruised and nicked" (<100) → "healthy" (100)

**SP:** "brain dead" (5) → "depleted" (15.5) → ... → "fully charged" (100)

**MP:** "exhausted" (5) → "sluggish" (15.5) → ... → "full of stamina" (100)

## Death Sequence — `body.lpc::die()`

Death is detected in the NPC heartbeat:

```lpc
if(!is_dead() && query_hp() <= 0.0) {
    set_dead(1);
    die();
}
```

### `die()` Flow

1. `stop_all_attacks()` — clear combat state
2. SU body ejection (if a developer is possessing this body)
3. `simple_action("$N $vhave perished.")` — room message
4. `save_body()` — persist player state
5. `emit(SIG_PLAYER_DIED, this_object(), killed_by())` — signal
6. Create corpse: `new(LIB_CORPSE)` → `corpse->setup_corpse(self, killer)`
7. Loot drop: `LOOT_D->loot_drop(killed_by(), self)` if `query_loot_table` exists
8. Coin drop: `LOOT_D->coin_drop(killed_by(), self)` if `query_coin_table` exists
9. Move all inventory to corpse
10. Move all wealth to corpse (converted to coin objects)
11. Move corpse to room
12. **Player path:** `BODY_D->create_ghost(privs)`, `exec(ghost, self)`, move ghost to room
13. **NPC path:** `ADVANCE_D->kill_xp(killed_by(), self)` — award XP to killer
14. `remove()` — destroy this object

## XP and Advancement — `adm/daemons/advance.lpc`

### TNL Formula

```lpc
to_next_level(level) = to_int(BASE_TNL * pow(TNL_RATE, level - 1.0))
```

Geometric: each level costs `TNL_RATE` times the one before. Call
`ADVANCE_D->to_next_level()` for a real number rather than working one out from
a table — `BASE_TNL` and `TNL_RATE` are tuning knobs and any table of worked
values goes stale the first time either moves.

### Kill XP Formula

```lpc
xp       = to_next_level(killed_level) / 10;  // 10% of killed NPC's TNL
variance = xp / 10;
xp       = xp - variance + random(variance);  // +/- 10% random

level_diff = killer_level - killed_level;

if(level_diff > 5)      // overlevel
    factor -= 0.05 * (level_diff - 5);    // -5% per level over threshold
else if(level_diff < 0) // underlevel
    factor += 0.05 * (-level_diff);       // +5% per level under

xp = to_int(xp * factor);
```

If `PLAYER_AUTOLEVEL` is true (default), `advance()` is called immediately after XP award.

### Config Constants

| Key | Used In |
|---|---|
| `COMBAT.DEFAULT_HIT_CHANCE` | `can_strike()` |
| `COMBAT.BASE_DAMAGE` | `calculate_damage()` |
| `COMBAT.DAMAGE_VARIANCE` | `calculate_damage()` |
| `COMBAT.DAMAGE_LEVEL_MODIFIER` | `receive_damage()` |
| `COMBAT.NPC_SKILL_MULTIPLIER` | `query_skill()` / `query_skill_level()` on non-PCs, `adjust_skills_by_npc_level()` |
| `BASE_TNL` | `to_next_level()` |
| `TNL_RATE` | `to_next_level()` |
| `OVERLEVEL_THRESHOLD` | `kill_xp()` |
| `OVERLEVEL_XP_PUNISH` | `kill_xp()` |
| `UNDERLEVEL_THRESHOLD` | `kill_xp()` |
| `UNDERLEVEL_XP_BONUS` | `kill_xp()` |
| `PLAYER_AUTOLEVEL` | `earn_xp()` |
| `HEART_PULSE` | regen interval |
| `HEARTBEATS_TO_REGEN` | regen interval |
| `DEFAULT_HEART_RATE` | NPC heartbeat |

Values live in `adm/etc/default.lpml`. Read them with `mud_config()`; do not
restate them here or in code.

## NPC Combat Behaviour — `std/living/npc.lpc`

### Heartbeat

NPCs only tick when players are present:

```lpc
void start_heart_beat() {
    if(player_check())
        set_heart_beat(mud_config("DEFAULT_HEART_RATE"));
}

void stop_heart_beat() {
    if(!player_check() && query_hp() >= 100.0)
        set_heart_beat(0);
}
```

Heartbeat loop: `clean_up_enemies()` → `cooldown()` → death check → `heal_tick()` → `evaluate_heart_beat()` → `process_boon()`.

### Combat Memory Module — `std/modules/mob/combat_memory.lpc`

Automatically added to all NPCs. Registered as an `add_init()` handler, so it
fires when a body enters the room.

**The store is `private nomask nosave string *` — names, not object
references.** That is the whole design, and it has two consequences worth
holding onto:

- **A grudge outlives the player's body object.** Dying, being revived, and
  logging out and back in all construct a *new* body, but `set_name()` is
  re-applied from the character name every time (`adm/daemons/body.lpc`
  `create_body()` / `create_ghost()`, and `adm/obj/login.lpc`). The name matches,
  so the NPC still knows them.
- **The grudge lasts exactly as long as the NPC object.** A reboot, a `renew`,
  or anything else that replaces that NPC starts it with an empty list. The
  `nosave` on the declaration is belt-and-braces and changes nothing — NPCs
  never call `set_persistent()`, so none of their state was going to be saved
  either way.

Ghosts are skipped outright, so a dead player walks back to their corpse
unmolested.

```lpc
void attack_on_sight(object target) {
    if(target->is_ghost())
        return;

    name = target->query_name();

    if(of(name, combat_memory)) {
        query_owner()->targetted_action(
            "{{FF0033}}Raging, $N $vattack $t with a vengeance!{{res}}\n\n", target);
        query_owner()->start_attack(target);
        query_owner()->strike_enemy(target);   // immediate free strike
        query_owner()->strike_enemy(target);   // second free strike
    }
}
```

Memory is populated from `combat.lpc::start_attack()`:

```lpc
if(npcp(this_object()))
    module("combat_memory", "add_to_memory", victim);
```

## GMCP Events

| Package | When | Fields |
|---|---|---|
| `Char.Status` | Every combat round | `current_enemy`, `current_enemy_health` ("%.2f" ratio), `current_enemies` |
| `Char.Status` | Combat ends | `current_enemy: ""`, `current_enemies: ({})` |
| `Char.Vitals` | Every `adjust_hp/sp/mp` | `hp`, `sp`, `mp` (as "%.2f") |
| `Char.Status` | Level/XP changes | `xp`, `tnl`, `level` |

## Signals

| Signal | When | Payload |
|---|---|---|
| `SIG_PLAYER_DIED` | `die()` after corpse creation | `(self, killed_by)` |
| `SIG_PLAYER_REVIVED` | External revival | `(self)` |
| `SIG_PLAYER_ADVANCED` | `advance()` on level-up | `(tp, new_level)` |

## Combat Skill Names

| Skill | Used In |
|---|---|
| `"combat.melee"` | Multi-strike chance in `swing()` |
| `"combat.melee.<type>"` | Hit/damage formulas (`can_strike`, `strike_enemy`) |
| `"combat.melee.unarmed"` | Unarmed fallback |
| `"combat.defence.dodge"` | Melee defence in `can_strike()` |
| `"combat.defence.evade"` | Spell defence in `can_strike()` |
| `"combat.defence"` | Generic defence reduction in `strike_enemy()` |

## Attack Speed

| Function | Description |
|---|---|
| `set_attack_speed(float)` | Sets the base speed, then clamps to `[0.5, 10.0]` |
| `add_attack_speed(float)` | **Subtracts** the amount, then clamps — a positive argument makes the body *faster* |
| `query_attack_speed()` | Returns `__attack_speed` |

The interval per round is `__attack_speed` seconds, unmodified. The value is a
*gap*, not a rate, which is why `add_attack_speed()` subtracts.

## Gotchas

1. **Death is detected in heartbeat, not in `receive_damage`**. The `dead` flag is set in `npc.lpc::heart_beat()` when `query_hp() <= 0.0`. There can be a brief window between HP hitting zero and `die()` firing.
2. **HP and SP do not regenerate during combat. MP does.** `heal_tick()` gates the HP and SP branches on `in_combat()`; the MP branch is ungated on purpose, because being at 0 MP hands every attacker a large hit-chance bonus.
3. **NPCs stop ticking in empty rooms.** No heartbeat = no regen, no boon processing, no AI. Developers expecting continuous background behaviour need to understand this.
4. **An NPC's skills are derived from its level — that is the design, not a quirk.** `query_skill()` and `query_skill_level()` branch on `pcp()` and compute `query_effective_level() * COMBAT.NPC_SKILL_MULTIPLIER` for *any* skill name, known or not, so a monster is fully equipped by `set_level()` alone and needs no skill authoring. The API consequence: those two never return null on an NPC, so `has_skill()` is the existence check, and `query_raw_skill()` / `query_raw_skill_level()` are what read an NPC's stored tree. Combat math uses `query_skill_level()`, so stored skills do not change how an NPC fights.
5. **`set_level()` on NPCs reseeds skills, so it must come first.** `npc.lpc::set_level()` calls `adjust_skills_by_npc_level()`, which overwrites every node in the tree with `level * COMBAT.NPC_SKILL_MULTIPLIER`. Custom skills added beforehand are lost silently — no error, just a line of code that achieved nothing.
6. **`__proc_chance` gates every proc roll.** `can_proc()` rolls it before it looks at cooldowns, then picks among the off-cooldown procs with `element_of_weighted`. A proc with `cooldown` 0 is never eligible.
7. **Threat is accumulated damage**, not an abstract aggro value. `highest_threat()` targets whoever has dealt the most damage to this living.
8. **Hit chance is not the linear sum.** `can_strike()` squashes it through `dim_sigmoid()` before rolling, so it never reaches 0 or 100 and stacked bonuses fall off sharply.
9. **`add_attack_speed()` subtracts.** The value is the gap between rounds, so a positive argument speeds the body up.
10. **Damage has no level term.** `calculate_damage()` dropped it; the level gap is applied victim-side in `receive_damage()`, where it scales how effective the defender's armour is.

## Timed Abilities: async_act

Abilities with a wind-up use `async_act()` on the body — the async counterpart
of `act()` in `std/living/act.lpc`. It registers an ordinary act, so
`is_acting()` reports it and `cancel_act()` / `cancel_acts()` still interrupt
it; what changes is that the outcome arrives as a promise rather than a
callback:

```lpc
private async void strike(object tp, object victim) {
  mixed err = acatch {
    if(!await tp->async_act("punch", 2.0))
      return;                                   // interrupted

    if(!same_env_check(tp, victim))
      return;

    if(tp->can_strike(victim)) {
      float damage = percent_of(25.0, tp->query_damage());

      tp->targetted_action("$N $vpunch $t!", victim);
      tp->deliver_mundane_damage(victim, damage, "bludgeoning");
      tp->use_skill("combat.melee.unarmed");
    } else {
      tp->targetted_action("$N $vtry to punch $t, but $vmiss.", victim);
      victim->use_skill("combat.defence.dodge");
    }

    victim->start_attack(tp);
  };

  if(err)
    debug_message(`punch: ${err}\n`);
}
```

The promise fulfils with **1** if the act ran to completion and **0** if it was
cancelled — which is what makes walking out of the room abandon the blow, since
`move()` calls `cancel_acts()`. Treat a `0` as "silently abandon", not as a
failure to report.

`cmds/ability/punch.lpc` is the worked example. `use()` must stay synchronous
and call the helper without awaiting — see the `command-creation` skill for why.
The older `delay_act(action, delay, assemble_call_back(...))` form still works
and several spells use it; the difference is that the async form keeps `tp` and
`victim` in lexical scope instead of threading them through the callback array.

Two interruptions exist and they are not interchangeable. `cancel_act()`
interrupts **the act**, fulfilling the promise with `0` so the awaiting body
handles it normally — that is the one the game world uses. `promise_cancel()`
interrupts **the awaiting body**, raising at its next `await`. See the
`async-promises` skill.
