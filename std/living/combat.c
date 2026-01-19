/**
 * @file /std/user/combat.c
 * @description Combat module. BANG! BOOM! POW!
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
private nosave mapping _current_enemies = ([]);
private nosave mapping _seen_enemies = ([]);
private nosave float _attack_speed = 2.0;
private nosave int _next_combat_round = 0;
private nosave mapping _defense = ([]);
private nosave float _ac = 0.0;
private nosave object _last_damager;
private nosave object _killed_by_ob;
private nosave string *_combat_memory = ({ });
private nosave int _no_combat = 0;

void combat_round() {
  object enemy;

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

  enemy = highest_threat();

  if(!valid_enemy(enemy))
    return;

  swing();

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
    GMCP_LBL_CHAR_STATUS_CURRENT_ENEMY: enemy->query_name(),
    GMCP_LBL_CHAR_STATUS_CURRENT_ENEMY_HEALTH: sprintf("%.2f", enemy->hp_ratio()),
    GMCP_LBL_CHAR_STATUS_CURRENT_ENEMIES: keys(_current_enemies),
  ]));

  if(find_call_out(_next_combat_round) == -1)
    next_round();
}

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

void swing(int count, int multi) {
  object enemy = highest_threat();
  object weapon;
  string *slots = query_weapon_slots();
  mapping wielded;
  int num;

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

  wielded = query_wielded();
  wielded = filter(wielded, (: objectp($2) :));
  if(sizeof(wielded)) {
    string main_slot = slots[0];
    if(multi) {
      object *poss;
      poss = filter(wielded, (: $1 != $(main_slot) :));
      poss = distinct_array(values(wielded));
      weapon = element_of(poss);
      multi = 0;
    } else {
      weapon = wielded[main_slot];
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

int next_round() {
  float speed = _attack_speed;

  speed += random_float(1.5);

  _next_combat_round = call_out_walltime("combat_round", speed);

  return _next_combat_round;
}

public int can_strike(object enemy, mixed weapon) {
  float ac;
  float chance = mudConfig("DEFAULT_HIT_CHANCE");
  float lvl = query_effective_level();
  float vlvl = enemy->query_effective_level();
  float result;
  string name, vname;
  object env;
  string *messes, mess;
  string skill_name, *skill_parts;
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

void strike_enemy(object enemy, object weapon) {
  string wname, wtype;
  string *messes, mess;
  float dam;
  string skill_name;
  float skill;
  float base, variance, rand, wbase;
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

int attacking(object victim) {
  if(!victim)
    return 0;

  if(_current_enemies[victim])
    return 1;

  return 0;
}

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

int in_combat() {
  return sizeof(_current_enemies) > 0;
}

int seen_enemy(object victim) {
  if(!victim)
    return 0;

  if(_seen_enemies[victim])
    return 1;

  return 0;
}

int current_enemy(object victim) {
  if(!victim)
    return 0;

  if(_current_enemies[victim])
    return 1;

  return 0;
}

mapping current_enemies() {
  return copy(_current_enemies);
}

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

varargs int valid_enemy(object enemy) {
  if(!same_env_check(this_object(), enemy))
    return 0;

  if(enemy->is_dead())
    return 0;

  return 1;
}

void clean_up_seen_enemies() {
  _seen_enemies = filter(_seen_enemies, (: valid_seen_enemy :));
}

varargs int valid_seen_enemy(object enemy, int threat) {
  if(!objectp(enemy))
    return 0;

  if(enemy->is_dead())
    return 0;

  return 1;
}

float add_threat(object enemy, float amount) {
  if(!valid_enemy(enemy))
    return 0.0;

  _current_enemies[enemy] += amount;

  return _current_enemies[enemy];
}

float add_seen_threat(object enemy, float amount) {
  if(!valid_seen_enemy(enemy))
    return 0.0;

  _seen_enemies[enemy] += amount;

  return _seen_enemies[enemy];
}

float add_attack_speed(float amount) {
  _attack_speed += amount;

  _attack_speed = clamp(0.5, 10.0, _attack_speed);

  return _attack_speed;
}

void set_attack_speed(float speed) {
  _attack_speed = speed;
}

float query_attack_speed() {
  return _attack_speed;
}

void set_defense(mapping def) {
  _defense = def;

  adjust_protection();
}

void add_defense(string type, float amount) {
  if(!_defense)
    _defense = ([ ]);

  _defense[type] = amount;

  adjust_protection();
}

mapping query_defense() {
  return copy(_defense);
}

float query_defense_amount(string type) {
  if(!_defense)
    return 0.0;

  return _defense[type];
}

mapping adjust_protection() {
  mapping equipment = query_equipped();
  object *obs = values(equipment), ob;

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

float query_ac() {
  return _ac;
}

object last_damaged_by() {
  return _last_damager;
}

object set_last_damaged_by(object ob) {
  _last_damager = ob;
  return _last_damager;
}

object killed_by() {
  return _killed_by_ob;
}

object set_killed_by(object ob) {
  _killed_by_ob = ob;
  return _killed_by_ob;
}

// The following are generally used by NPCs, but are available for special
// circumstances for players.

private nomask float _damage = 0.0;
private nomask string _weapon_name = "fist";
private nomask string _weapon_type = "bludgeoning";

float set_damage(float x) {
  if(x < 0.0)
    return 0.0;

  return _damage = x;
}

float query_damage() {
  if(_damage <= 0.0)
    return random_float(queryLevel() * 2.0);

  return random_float(_damage);
}

string set_weapon_name(string str) {
  if(!stringp(str))
    return _weapon_name;

  return _weapon_name = str;
}

string query_weapon_name() {
  return _weapon_name;
}

string set_weapon_type(string str) {
  if(!stringp(str))
    return _weapon_type;

  return _weapon_type = str;
}

string query_weapon_type() {
  return _weapon_type;
}

mixed prevent_combat(object victim) {
  if(victim->query_peaceful(this_object()))
    return "You cannot attack a peaceful creature.\n";

  if(victim->query_no_combat(this_object()))
    return "You cannot attack that.\n";

  if(environment()->query_no_combat(this_object()))
    return "You cannot attack here.\n";

  return 1;
}

void set_no_combat(int x) {
  _no_combat = !!x;
}

int query_no_combat() {
  return _no_combat;
}
