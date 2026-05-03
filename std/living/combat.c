/**
 * @file /std/living/combat.c
 *
 * Combat module for living objects. Drives the per-round attack
 * loop, threat tracking for current and recently-seen enemies,
 * hit-chance and damage resolution, defence and AC aggregation
 * from equipment, and the weapon/damage fields NPCs use to fight
 * without wielded equipment.
 *
 * @created 2024-07-24 - Gesslar
 * @last_modified 2024-07-24 - Gesslar
 *
 * @history
 * 2024-07-24 - Gesslar - Created
 */

#include <combat.h>
#include <action.h>
#include <advancement.h>
#include <body.h>
#include <equipment.h>
#include <gmcp_defines.h>
#include <module.h>
#include <skills.h>
#include <vitals.h>

inherit __DIR__ "damage";

// Functions from other objects
string query_name();

// Variables
/**
 * Active threat table — enemies currently engaged with this body,
 * keyed by the enemy object and valued by accumulated threat.
 *
 * @type {([ STD_BODY: float ])}
 */
private nosave mapping _current_enemies = ([]);

/**
 * Recently-seen threat table — enemies this body has fought or
 * observed, retained after disengagement so memory persists
 * between encounters.
 *
 * @type {([ STD_BODY: float ])}
 */
private nosave mapping _seen_enemies = ([]);

private nosave float _attack_speed = 2.0;
private nosave int _next_combat_round = 0;

/**
 * Aggregate defence values by damage type, recomputed from
 * equipped items in adjust_protection().
 *
 * @type {([ string: float ])}
 */
private nosave mapping _defense = ([]);

private nosave float _ac = 0.0;

/**
 * The last body to deliver damage to this one. Used by the
 * damage and death pipeline to attribute kills.
 *
 * @type {STD_BODY}
 */
private nosave object _last_damager;

/**
 * The body credited with killing this one, if killed.
 *
 * @type {STD_BODY}
 */
private nosave object _killed_by_ob;

private nosave string *_combat_memory = ({ });
private nosave int _no_combat = 0;

/**
 * Per-tick combat heartbeat. Stops the loop if dead or
 * unconscious, prunes stale enemies, picks the highest-threat
 * target, executes a swing, broadcasts the GMCP combat status,
 * and schedules the next round if one is not already pending.
 */
void combat_round() {
  /** @type {STD_BODY} */ object victim;

  if(is_dead())
    return;

  if(query_hp() <= 0.0) {
    stop_all_attacks();
    return;
  }

  clean_up_enemies();

  if(!in_combat()) {
    _next_combat_round = 0;
    return;
  }

  victim = highest_threat();

  if(!valid_enemy(victim))
    return;

  swing();

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
    GMCP_LBL_CHAR_STATUS_CURRENT_ENEMY: victim->query_name(),
    GMCP_LBL_CHAR_STATUS_CURRENT_ENEMY_HEALTH: sprintf("%.2f", victim->hp_ratio()),
    GMCP_LBL_CHAR_STATUS_CURRENT_ENEMIES: keys(_current_enemies),
  ]));

  if(find_call_out(_next_combat_round) == -1)
    next_round();
}

/**
 * Begin attacking a victim. Adds it to the current and seen
 * enemy tables, schedules the first combat round, and tells the
 * victim to start attacking back. NPCs also push the victim
 * into combat memory.
 *
 * @param {STD_BODY} victim - The body to engage.
 * @returns {int} 1 if a new engagement was started, 0 if the
 *                victim was missing or already engaged.
 */
int start_attack(object victim) {
  if(!victim)
    return 0;

  if(_current_enemies[victim])
    return 0;

  _current_enemies[victim] = 1.0;

  if(!_seen_enemies[victim])
    _seen_enemies[victim] = 1.0;

  if(!userp())
    module("combat_memory", "add_to_memory", victim);

  _next_combat_round = call_out_walltime("combat_round", _attack_speed);

  victim->start_attack(this_object());

  return 1;
}

/**
 * Recursively performs a sequence of attack swings against the
 * current highest-threat enemy. Picks the main weapon for the
 * first swing and may roll into an off-hand swing for the next
 * iteration based on the combat.melee skill. Stops early if the
 * attacker is exhausted, the enemy is invalid, or count is
 * exhausted.
 *
 * @param {int} [count=1] - Remaining swings to perform.
 * @param {int} [multi] - Non-zero if this iteration should pick
 *                        an off-hand weapon instead of the main.
 */
void swing(int count, int multi) {
  object enemy = highest_threat();
  object weapon;
  string *slots = query_weapon_slots();
  mapping _wielded;

  if(nullp(count))
    count = 1;

  if(count < 1)
    return;

  if(!enemy)
    return;

  if(!valid_enemy(enemy))
    return;

  if(query_mp() <= 0.0) {
    tell(this_object(), "You are too exhausted to attack.\n");
    return;
  }

  _wielded = query_wielded();
  _wielded = filter(_wielded, (: objectp($2) :));
  if(sizeof(_wielded)) {
    string main_slot = slots[0];
    if(multi) {
      object *poss;
      poss = filter(_wielded, (: $1 != $(main_slot) :));
      poss = distinct_array(values(_wielded));
      weapon = element_of(poss);
      multi = 0;
    } else {
      weapon = _wielded[main_slot];
      if(random(100) < 5 + query_skill("combat.melee"))
        multi = 1;
    }
  }

  if(can_strike(enemy, weapon))
    strike_enemy(enemy, weapon);
  else {
    enemy->use_skill("combat.defense.dodge");
    fail_strike(enemy, weapon);
  }

  swing(count - 1, multi);
}

/**
 * Schedules the next combat round. Adds up to 1.5 seconds of
 * jitter to the configured attack speed so attackers do not all
 * fire on the same tick.
 *
 * @returns {int} The call_out handle for the scheduled round.
 */
int next_round() {
  float speed = _attack_speed;

  speed += random_float(1.5);

  _next_combat_round = call_out_walltime("combat_round", speed);

  return _next_combat_round;
}

/**
 * Rolls the hit chance for an attempted strike. The weapon
 * argument selects how the roll is computed:
 *
 *   - object (or null) — physical attack against the enemy's
 *     AC, defended by combat.defense.dodge.
 *   - string — treated as a skill name (e.g. an ability or
 *     spell). Spell paths defend against combat.defense.evade,
 *     other skill paths against combat.defense.dodge, and the
 *     enemy's spell AC is used in place of physical AC.
 *
 * The defending skill is exercised regardless of outcome.
 *
 * @public
 * @param {STD_BODY} enemy - The body being struck.
 * @param {object | string} weapon - Wielded weapon, null for
 *                                   unarmed/default, or a skill
 *                                   name string for spells and
 *                                   abilities.
 * @returns {int} 1 if the strike lands, 0 if it misses or the
 *                weapon argument is of an unsupported type.
 */
public int can_strike(object enemy, mixed weapon) {
  float ac;
  float chance = mudConfig("DEFAULT_HIT_CHANCE");
  float lvl = query_effective_level();
  float vlvl = enemy->query_effective_level();
  float result;
  string skill_name;
  string defense_skill;
  float skill;
  mapping weapon_info;

  if(nullp(weapon) || objectp(weapon)) {
    weapon_info = query_weapon_info(weapon);
    skill_name = weapon_info["skill"];
    ac = enemy->query_ac();
    defense_skill = "combat.defense.dodge";
  } else if(stringp(weapon)) {
    skill_name = weapon;
    ac = enemy->query_spell_ac();
    if(strsrch(weapon, ".spell.") != -1)
      defense_skill = "combat.defense.evade";
    else
      defense_skill = "combat.defense.dodge";
  } else
    return 0;

  skill = query_skill_level(skill_name);

  if(enemy->query_mp() < 0.0)
    chance += 25.0;

  chance = chance
          + (lvl - vlvl)
          + skill
          - (ac * 2.0)
          - enemy->query_skill_level(defense_skill)
;

  result = random_float(100.0);

  enemy->use_skill(defense_skill);

  return result < chance;
}

/**
 * Emits the miss messages for a failed strike to the attacker,
 * the victim, and the surrounding environment.
 *
 * @private
 * @param {STD_BODY} enemy - The body that dodged.
 * @param {object} weapon - The weapon used (may be null for
 *                          unarmed or NPC defaults).
 */
private fail_strike(object enemy, object weapon) {
  string wname, wtype;
  string *messes, mess;
  float skill;
  mapping weapon_info = query_weapon_info(weapon);

  wname = weapon_info["name"];
  wtype = weapon_info["type"];

  mess = MESS_D->get_message("combat", wtype, 0);
  messes = ACTION_D->action(({ this_object(), enemy }), mess, ({wname}));

  tell(this_object(), messes[0], MSG_COMBAT_MISS);
  tell(enemy, messes[1], MSG_COMBAT_MISS);
  tell_down(environment(), messes[2], MSG_COMBAT_MISS, ({ this_object(), enemy }));
}

/**
 * Resolves a successful strike. Computes damage from a base
 * percentage of the enemy's max HP plus level and skill bonuses,
 * minus the enemy's level, type-specific defence, and combat
 * defence skill, with a hidden bonus when the enemy is past
 * exhaustion. Sends the hit messages, exercises the weapon
 * skill, delivers damage, drains the attacker's MP, records
 * threat in both the current and seen tables, and triggers any
 * weapon proc.
 *
 * @param {STD_BODY} enemy - The body being struck.
 * @param {object} weapon - The weapon used, or null for unarmed
 *                          or NPC defaults.
 */
void strike_enemy(object enemy, object weapon) {
  string wname, wtype;
  string *messes, mess;
  float dam;
  string skill_name;
  float skill;
  float base, variance, wbase;
  mapping weapon_info;
  string proc;

  if(!valid_enemy(enemy))
    return;

  if(!current_enemy(enemy))
    return;

  weapon_info = query_weapon_info(weapon);

  skill_name = sprintf(weapon_info["skill"]);
  skill = query_skill_level(skill_name);
  base = percent_of(5.0, enemy->query_max_hp());
  variance = percent_of(25.0, base);
  base -= variance;
  variance = random_float(variance);
  base += variance;

  wbase = weapon_info["base"];
  wname = weapon_info["name"];
  wtype = weapon_info["type"];

  if(enemy->query_mp() < 0.0)
      base += 4.0;

  dam =
      base
    + query_effective_level()
    + skill
    - enemy->query_effective_level()
    - enemy->query_defense_amount(wtype)
    - enemy->query_skill_level("combat.defense")
  ;

  // tell(enemy, sprintf("Base: %f, Skill: %f, Level: %f, Enemy Level: %f, Enemy Defense: %f, Enemy Skill: %f\n",
  //     base, skill, query_effective_level(), enemy->query_effective_level(), enemy->query_defense_amount(wtype), enemy->query_skill_level("combat.defense")));
  // tell(enemy, "Damage: " + dam + "\n");

  use_skill(skill_name);

  if(dam < 0.0)
    dam = 1.0;

  mess = MESS_D->get_message("combat", wtype, to_int(ceil(dam)));
  messes = ACTION_D->action(({ this_object(), enemy }), mess, ({wname}));

  tell(this_object(), messes[0], MSG_COMBAT_HIT);
  tell(enemy, messes[1], MSG_COMBAT_HIT);
  tell_down(environment(), messes[2], MSG_COMBAT_HIT, ({ this_object(), enemy }));

  deliver_damage(enemy, dam, wtype);
  adjust_mp(-random_float(5.0));
  add_threat(enemy, dam);
  add_seen_threat(enemy, dam);

  if(weapon && weapon->is_weapon())
    if(stringp(proc = weapon->can_proc()))
      weapon->proc(proc, this_object(), enemy);
}

/**
 * Resolves the descriptive fields used when striking with a
 * given weapon. For an actual weapon object the name, damage
 * type, derived combat skill, and damage coefficient are read
 * from the weapon. For unarmed players the result is fists with
 * the unarmed combat skill. For NPCs without a weapon the
 * configured weapon name and type are used along with the
 * NPC's base damage.
 *
 * @param {object} weapon - The weapon object, or null for
 *                          unarmed or NPC defaults.
 * @returns {([ string: mixed ])} Mapping with keys "name"
 *          (string), "type" (string), "skill" (string), and
 *          "base" (float).
 */
mapping query_weapon_info(object weapon) {
  string wname, wtype;
  string skill_name;
  float base;

  if(weapon) {
    wname = weapon->query_name();
    wtype = weapon->query_damage_type();
    skill_name = sprintf("combat.melee.%s", wtype);
    base = weapon->query_dc();
  } else {
    if(userp()) {
      wname = "fist";
      wtype = "bludgeoning";
      skill_name = "combat.melee.unarmed";
    } else {
      wname = query_weapon_name();
      wtype = query_weapon_type();
      skill_name = "combat.melee.unarmed";
    }
    base = query_damage();
  }

  return ([
    "name": wname,
    "type": wtype,
    "skill": skill_name,
    "base": base,
  ]);
}

/**
 * Returns whether this body is currently attacking the given
 * victim.
 *
 * @param {STD_BODY} victim - The body to test.
 * @returns {int} 1 if engaged with victim, 0 otherwise.
 */
int attacking(object victim) {
  if(!victim)
    return 0;

  if(_current_enemies[victim])
    return 1;

  return 0;
}

/**
 * Removes the victim from the current enemy table, optionally
 * also from the seen-enemies table.
 *
 * @param {STD_BODY} victim - The body to disengage from.
 * @param {int} [seen] - If non-zero, also drop the victim from
 *                       the seen-enemies table.
 * @returns {int} 1 if a removal occurred, 0 if neither table
 *                contained the victim, -1 if victim was missing
 *                or not currently engaged.
 */
varargs int stop_attack(object victim, int seen) {
  if(!victim)
    return -1;

  if(!_current_enemies[victim])
    return -1;

  if(_current_enemies[victim]) {
    map_delete(_current_enemies, victim);
    return 1;
  }

  if(seen && _seen_enemies[victim]) {
    map_delete(_seen_enemies, victim);
    return 1;
  }

  return 0;
}

/**
 * Disengages from every current enemy. Cancels the pending
 * combat round, clears the current enemy table, broadcasts an
 * empty GMCP combat status, and runs a final cleanup pass.
 */
void stop_all_attacks() {
  if(find_call_out(_next_combat_round) != -1)
    remove_call_out(_next_combat_round);

  _current_enemies = ([]);

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
    GMCP_LBL_CHAR_STATUS_CURRENT_ENEMY: "",
    GMCP_LBL_CHAR_STATUS_CURRENT_ENEMIES: ({}),
  ]));

  clean_up_enemies();

  if(is_dead())
    return;
}

/**
 * Returns whether this body has any current enemies.
 *
 * @returns {int} 1 if engaged with at least one enemy, 0
 *                otherwise.
 */
int in_combat() {
  return sizeof(_current_enemies) > 0;
}

/**
 * Returns whether the victim has been seen recently — i.e. is
 * tracked in the seen-enemies table even if not currently
 * engaged.
 *
 * @param {STD_BODY} victim - The body to test.
 * @returns {int} 1 if present in the seen table, 0 otherwise.
 */
int seen_enemy(object victim) {
  if(!victim)
    return 0;

  if(_seen_enemies[victim])
    return 1;

  return 0;
}

/**
 * Returns whether the victim is in the current enemy table.
 *
 * @param {STD_BODY} victim - The body to test.
 * @returns {int} 1 if currently engaged, 0 otherwise.
 */
int current_enemy(object victim) {
  if(!victim)
    return 0;

  if(_current_enemies[victim])
    return 1;

  return 0;
}

/**
 * Returns a shallow copy of the current enemy threat table.
 *
 * @returns {([ STD_BODY: float ])} Copy of the current enemies
 *          mapping, keyed by enemy and valued by threat.
 */
mapping current_enemies() {
  return copy(_current_enemies);
}

/**
 * Returns the current enemy with the highest accumulated threat.
 *
 * @returns {STD_BODY} The highest-threat enemy, or 0 if no
 *                     current enemies exist.
 */
object highest_threat() {
  object *enemies;
  object highest;
  int highest_threat = 0;

  if(!sizeof(_current_enemies))
    return 0;

  enemies = keys(_current_enemies);
  highest = enemies[0];

  foreach(object enemy in enemies) {
    if(_current_enemies[enemy] > highest_threat) {
      highest = enemy;
      highest_threat = _current_enemies[enemy];
    }
  }

  return highest;
}

/**
 * Returns the current enemy with the lowest accumulated threat.
 *
 * @returns {STD_BODY} The lowest-threat enemy.
 */
object lowest_threat() {
  object *enemies = keys(_current_enemies);
  object lowest = enemies[0];
  int lowest_threat = MAX_INT;

  foreach(object enemy in enemies) {
    if(_current_enemies[enemy] < lowest_threat) {
      lowest = enemy;
      lowest_threat = _current_enemies[enemy];
    }
  }

  return lowest;
}

/**
 * Returns the seen enemy with the highest accumulated threat.
 *
 * @returns {STD_BODY} The highest-threat seen enemy.
 */
object highest_seen_threat() {
  object *enemies = keys(_seen_enemies);
  object highest = enemies[0];
  int highest_threat = 0;

  foreach(object enemy in enemies) {
    if(_seen_enemies[enemy] > highest_threat) {
      highest = enemy;
      highest_threat = _seen_enemies[enemy];
    }
  }

  return highest;
}

/**
 * Returns the seen enemy with the lowest accumulated threat.
 *
 * @returns {STD_BODY} The lowest-threat seen enemy.
 */
object lowest_seen_threat() {
  object *enemies = keys(_seen_enemies);
  object lowest = enemies[0];
  int lowest_threat = MAX_INT;

  foreach(object enemy in enemies) {
    if(_seen_enemies[enemy] < lowest_threat) {
      lowest = enemy;
      lowest_threat = _seen_enemies[enemy];
    }
  }

  return lowest;
}

/**
 * Filters the current enemy table down to valid enemies. If the
 * table empties as a result, clears the next-round handle,
 * tells the body that combat is over (when alive), and broadcasts
 * an empty GMCP combat status.
 */
void clean_up_enemies() {
  if(is_dead())
    return;

  if(!in_combat())
    return;

  _current_enemies = filter(_current_enemies, (: valid_enemy :));

  if(!in_combat()) {
    _next_combat_round = 0;

    if(query_hp() > 0.0)
      tell(this_object(), "You are no longer in combat.\n");

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
      GMCP_LBL_CHAR_STATUS_CURRENT_ENEMY: "",
      GMCP_LBL_CHAR_STATUS_CURRENT_ENEMIES: ({}),
    ]));
  }
}

/**
 * Tests whether an enemy is still a valid combat target — same
 * environment as this body and not already dead.
 *
 * Declared varargs so it can be passed directly as a filter
 * callback over the threat mappings, where the iterator hands
 * in a value alongside the key.
 *
 * @param {STD_BODY} enemy - The candidate enemy.
 * @returns {int} 1 if still a valid target, 0 otherwise.
 */
varargs int valid_enemy(object enemy) {
  if(!same_env_check(this_object(), enemy))
    return 0;

  if(enemy->is_dead())
    return 0;

  return 1;
}

/**
 * Filters the seen-enemies table down to entries that are still
 * valid (objects that are not dead).
 */
void clean_up_seen_enemies() {
  _seen_enemies = filter(_seen_enemies, (: valid_seen_enemy :));
}

/**
 * Tests whether a seen-enemy entry is still worth keeping —
 * the value must be a live object. Unlike valid_enemy, no
 * environment check is performed; seen enemies can be tracked
 * across rooms.
 *
 * @param {STD_BODY} enemy - The candidate seen enemy.
 * @param {int} [threat] - The associated threat value, supplied
 *                         when used as a mapping filter callback.
 * @returns {int} 1 if the entry should be retained, 0 otherwise.
 */
varargs int valid_seen_enemy(object enemy, int threat) {
  if(!objectp(enemy))
    return 0;

  if(enemy->is_dead())
    return 0;

  return 1;
}

/**
 * Adds threat for a current enemy and returns the new
 * accumulated total. No-op if the enemy is no longer valid.
 *
 * @param {STD_BODY} enemy - The enemy whose threat to adjust.
 * @param {float} amount - Threat to add (may be negative).
 * @returns {float} The new threat total, or 0.0 if the enemy
 *                  was rejected.
 */
float add_threat(object enemy, float amount) {
  if(!valid_enemy(enemy))
    return 0.0;

  _current_enemies[enemy] += amount;

  return _current_enemies[enemy];
}

/**
 * Adds threat for a seen enemy and returns the new accumulated
 * total. No-op if the seen entry is no longer valid.
 *
 * @param {STD_BODY} enemy - The enemy whose seen threat to
 *                           adjust.
 * @param {float} amount - Threat to add (may be negative).
 * @returns {float} The new seen-threat total, or 0.0 if the
 *                  entry was rejected.
 */
float add_seen_threat(object enemy, float amount) {
  if(!valid_seen_enemy(enemy))
    return 0.0;

  _seen_enemies[enemy] += amount;

  return _seen_enemies[enemy];
}

/**
 * Adjusts the attack speed by a delta and clamps the result to
 * the legal range of 0.5 to 10.0 seconds per round.
 *
 * @param {float} amount - Delta to apply (may be negative).
 * @returns {float} The new clamped attack speed.
 */
float add_attack_speed(float amount) {
  _attack_speed += amount;

  _attack_speed = clamp(0.5, 10.0, _attack_speed);

  return _attack_speed;
}

/**
 * Sets the raw attack speed in seconds per round. No clamping
 * is applied; callers are responsible for staying in range.
 *
 * @param {float} speed - The new attack speed in seconds.
 */
void set_attack_speed(float speed) {
  _attack_speed = speed;
}

/**
 * Returns the current attack speed in seconds per round.
 *
 * @returns {float} The configured attack speed.
 */
float query_attack_speed() {
  return _attack_speed;
}

/**
 * Replaces the per-type defence mapping wholesale and triggers
 * a recompute from equipment.
 *
 * @param {([ string: float ])} def - New defence mapping keyed
 *                                    by damage type.
 */
void set_defense(mapping def) {
  _defense = def;

  adjust_protection();
}

/**
 * Sets a single damage-type defence value and triggers a
 * recompute from equipment.
 *
 * @param {string} type - The damage type (e.g. "fire", "slashing").
 * @param {float} amount - Defence value for that type.
 */
void add_defense(string type, float amount) {
  if(!_defense)
    _defense = ([ ]);

  _defense[type] = amount;

  adjust_protection();
}

/**
 * Returns a shallow copy of the per-type defence mapping.
 *
 * @returns {([ string: float ])} Damage-type to defence-value
 *          mapping.
 */
mapping query_defense() {
  return copy(_defense);
}

/**
 * Returns the defence value for a single damage type.
 *
 * @param {string} type - The damage type to look up.
 * @returns {float} The defence value, or 0.0 if not configured.
 */
float query_defense_amount(string type) {
  if(!_defense)
    return 0.0;

  return _defense[type];
}

/**
 * Recomputes the per-type defence mapping and total AC by
 * summing the corresponding values from every equipped item.
 * Called whenever defences are set or equipment changes.
 *
 * @returns {([ string: float ])} The recomputed defence mapping.
 */
mapping adjust_protection() {
  mapping _equipment = query_equipped();
  object *obs = values(_equipment), ob;

  { // Defenses
    _defense = ([]);
    foreach(ob in obs) {
      mapping def = ob->query_defense();

      if(!mapp(def))
        continue;

      foreach(string type, float amount in def) {
        if(!_defense[type])
          _defense[type] = 0.0;

        _defense[type] += amount;
      }
    }
  }

  { // Armor Class
    _ac = 0.0;
    foreach(ob in obs)
      _ac += ob->query_ac();
  }

  return _defense;
}

/**
 * Returns the cached total armour class aggregated across all
 * equipped items by adjust_protection().
 *
 * @returns {float} The current total AC.
 */
float query_ac() {
  return _ac;
}

/**
 * Returns the body that most recently dealt damage to this one.
 *
 * @returns {STD_BODY} The last damager, or 0 if none recorded.
 */
object last_damaged_by() {
  return _last_damager;
}

/**
 * Records a body as the most recent source of damage. Called
 * from the damage pipeline.
 *
 * @param {STD_BODY} ob - The damaging body.
 * @returns {STD_BODY} The same body that was set.
 */
object set_last_damaged_by(object ob) {
  _last_damager = ob;
  return _last_damager;
}

/**
 * Returns the body credited with killing this one.
 *
 * @returns {STD_BODY} The killer, or 0 if not killed.
 */
object killed_by() {
  return _killed_by_ob;
}

/**
 * Records the body credited with killing this one. Called from
 * the death sequence.
 *
 * @param {STD_BODY} ob - The killer.
 * @returns {STD_BODY} The same body that was set.
 */
object set_killed_by(object ob) {
  _killed_by_ob = ob;
  return _killed_by_ob;
}

// The following are generally used by NPCs, but are available for special
// circumstances for players.

private nomask float _damage = 0.0;
private nomask string _weapon_name = "fist";
private nomask string _weapon_type = "bludgeoning";

/**
 * Sets the NPC default base damage. Negative values are
 * rejected.
 *
 * @param {float} x - The new base damage (must be >= 0).
 * @returns {float} The new damage, or 0.0 if rejected.
 */
float set_damage(float x) {
  if(x < 0.0)
    return 0.0;

  return _damage = x;
}

/**
 * Returns a randomised damage roll for unarmed/NPC attacks. If
 * no base damage has been configured, the roll scales with the
 * NPC's level.
 *
 * @returns {float} A randomised damage value.
 */
float query_damage() {
  if(_damage <= 0.0)
    return random_float(queryLevel() * 2.0);

  return random_float(_damage);
}

/**
 * Sets the NPC default weapon name (used in combat messages
 * when no real weapon is wielded). Non-string values are
 * ignored.
 *
 * @param {string} str - The new weapon name.
 * @returns {string} The current weapon name after the call.
 */
string set_weapon_name(string str) {
  if(!stringp(str))
    return _weapon_name;

  return _weapon_name = str;
}

/**
 * Returns the NPC default weapon name.
 *
 * @returns {string} The configured weapon name.
 */
string query_weapon_name() {
  return _weapon_name;
}

/**
 * Sets the NPC default damage type (used when no real weapon
 * is wielded). Non-string values are ignored.
 *
 * @param {string} str - The new damage type.
 * @returns {string} The current damage type after the call.
 */
string set_weapon_type(string str) {
  if(!stringp(str))
    return _weapon_type;

  return _weapon_type = str;
}

/**
 * Returns the NPC default damage type.
 *
 * @returns {string} The configured damage type.
 */
string query_weapon_type() {
  return _weapon_type;
}

/**
 * Tests whether combat with the given victim is allowed in the
 * current context. Probes optional hooks on the victim and the
 * environment via call_other — any responder that returns
 * truthy vetoes the attack. Targets that do not implement these
 * hooks simply return 0, which is the common case.
 *
 * @param {object} victim - The object that would be attacked.
 *                          Any object is accepted; the relevant
 *                          hooks (query_peaceful, query_no_combat)
 *                          are duck-typed.
 * @returns {int | string} 1 if combat is permitted, or a string
 *          message describing why it is forbidden.
 */
mixed prevent_combat(object victim) {
  if(call_if(victim, "query_peaceful", this_object()))
    return "You cannot attack a peaceful creature.\n";

  if(call_if(victim, "query_no_combat", this_object()))
    return "You cannot attack that.\n";

  if(call_if(environment(), "query_no_combat", this_object()))
    return "You cannot attack here.\n";

  return 1;
}

/**
 * Sets the no-combat flag. When set, this body cannot be
 * targeted by attackers (@link prevent_combat).
 *
 * @param {int} x - Truthy to enable, zero to disable.
 */
void set_no_combat(int x) {
  _no_combat = !!x;
}

/**
 * Returns the no-combat flag.
 *
 * @returns {int} 1 if no-combat is enabled, 0 otherwise.
 */
int query_no_combat() {
  return _no_combat;
}
