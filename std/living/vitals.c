/**
 * @file /std/user/vitals.c
 * Vitals for livings
 *
 * @created 2024-07-24 - Gesslar
 * @last_modified 2024-07-24 - Gesslar
 *
 * @history
 * 2024-07-24 - Gesslar - Created
 */

#include <gmcp_defines.h>
#include <vitals.h>
#include <runtime_config.h>
#include <boon.h>
#include <module.h>
#include <combat.h>

private nomask float __hp, __max_hp, __sp, __max_sp, __mp, __max_mp;
private nomask int __dead = false;
private nomask nosave int __tick;
private nomask nosave int __regen_interval_pulses; // Number of pulses to trigger a regen

void init_vitals() {
  __hp = __hp ?? 100.0;
  __max_hp = __max_hp ?? 100.0;
  __sp = __sp ?? 100.0;
  __max_sp = __max_sp ?? 100.0;
  __mp = __mp ?? 100.0;
  __max_mp = __max_mp ?? 100.0;

  // Calculate regen interval based on the product of HEART_PULSE and
  // HEARTBEATS_TO_REGEN.
  update_regen_interval();
}

float query_hp() { return __hp; }
varargs float query_max_hp(int raw) {
  if(raw)
    return __max_hp;

  return __max_hp + query_effective_boon("vital", "max_hp");
}
float hp_ratio() { return percent(query_hp(), query_max_hp()); }

float query_sp() { return __sp; }
varargs float query_max_sp(int raw) {
  if(raw)
    return __max_sp;

  return __max_sp + query_effective_boon("vital", "max_sp");
}
float sp_ratio() { return percent(query_sp(), query_max_sp()); }

float query_mp() { return __mp; }
varargs float query_max_mp(int raw) {
  if(raw)
    return __max_mp;

  return __max_mp + query_effective_boon("vital", "max_mp");
}
float mp_ratio() { return percent(query_mp(), query_max_mp()); }

void set_hp(float x) {
  __hp = to_float(x);

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_HP: sprintf("%.2f", __hp),
  ]));
}
void set_max_hp(float x) {
  __max_hp = to_float(x);

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_MAX_HP: sprintf("%.2f", __max_hp),
  ]));
}

void set_sp(float x) {
  __sp = to_float(x);

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_SP: sprintf("%.2f", __sp),
  ]));
}

void set_max_sp(float x) {
  __max_sp = to_float(x);

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_MAX_SP: sprintf("%.2f", __max_sp),
  ]));
}

void set_mp(float x) {
  __mp = to_float(x);

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_MP: sprintf("%.2f", __mp),
  ]));
}

void set_max_mp(float x) {
  __max_mp = to_float(x);

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_MAX_MP: sprintf("%.2f", __max_mp),
  ]));
}

float adjust_hp(float x) {
  __hp += to_float(x);

  if(__hp > __max_hp)
    __hp = __max_hp;

  if(__hp <= 0.0)
      __hp = 0.0;

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_HP: sprintf("%.2f", __hp),
  ]));

  return __hp;
}

float adjust_max_hp(float x) {
  __max_hp += to_float(x);

  if(__max_hp < 0.0)
    __max_hp = 0.0;

  if(__hp > __max_hp)
    __hp = __max_hp;

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_HP: sprintf("%.2f", __hp),
    GMCP_LBL_CHAR_VITALS_MAX_HP: sprintf("%.2f", __max_hp),
  ]));

  return __max_hp;
}

float adjust_sp(float x) {
  __sp += to_float(x);

  if(__sp > __max_sp)
    __sp = __max_sp;

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_SP: sprintf("%.2f", __sp),
  ]));

  return __sp;
}

float adjust_max_sp(float x) {
  __max_sp += to_float(x);

  if(__max_sp < 0.0)
    __max_sp = 0.0;

  if(__sp > __max_sp)
    __sp = __max_sp;

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_SP: sprintf("%.2f", __sp),
    GMCP_LBL_CHAR_VITALS_MAX_SP: sprintf("%.2f", __max_sp),
  ]));

  return __max_sp;
}

float adjust_mp(float x) {
  __mp += to_float(x);

  if(__mp > __max_mp)
    __mp = __max_mp;

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_MP: sprintf("%.2f", __mp),
  ]));

  return __mp;
}

float adjust_max_mp(float x) {
  __max_mp += to_float(x);

  if(__max_mp < 0.0)
    __max_mp = 0.0;

  if(__mp > __max_mp)
    __mp = __max_mp;

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
    GMCP_LBL_CHAR_VITALS_MP: sprintf("%.2f", __mp),
    GMCP_LBL_CHAR_VITALS_MAX_MP: sprintf("%.2f", __max_mp),
  ]));

  return __max_mp;
}

protected void heal_tick(int force: (: 0 :)) {
  mapping rate = module("race", "query_regen_rate");

  if(nullp(rate))
    return;

  if(in_combat())
    return;

  if(++__tick >= __regen_interval_pulses || force) {
    if(!force)
      __tick = 0;

    if(__hp < __max_hp)
      adjust_hp(rate["hp"]);
    if(__sp < __max_sp)
      adjust_sp(rate["sp"]);
    if(__mp < __max_mp)
      adjust_mp(rate["mp"]);
  }
}

int set_heart_rate(int x) {
  if(x < 5)
    x = 5;
  else if(x > 100)
    x = 100;

  set_heart_beat(x);

  return x;
}

int add_heart_rate(int x) {
  return set_heart_rate(query_heart_beat() + x);
}

// This function calculates the number of pulses needed based on HEART_PULSE and HEARTBEATS_TO_REGEN
void update_regen_interval() {
  // Calculate the number of pulses for the regen interval
  __regen_interval_pulses = to_int((mud_config("HEART_PULSE") * mud_config("HEARTBEATS_TO_REGEN")) / 1000.0); // Convert ms to seconds
  __tick = 0;
}

// This function initializes the healing process
void initialize_healing() {
  // Calculate regen interval based on the product of HEART_PULSE and HEARTBEATS_TO_REGEN
  update_regen_interval();
}

int query_heart_rate() {
  return query_heart_beat();
}

int query_regen_duration() {
  return __regen_interval_pulses;
}

void restore() {
  set_hp(query_max_hp());
  set_sp(query_max_sp());
  set_mp(query_max_mp());
}

int set_dead(int x) {
  __dead = !!x;

  return __dead;
}

float *query_condition() {
  return ({
    hp_ratio(),
    sp_ratio(),
    mp_ratio(),
  });
}

string *query_condition_string() {
  string *result = allocate(3);
  float *ratio = query_condition();

  // HP Condition
  if(ratio[0] <= 0.0)
      result[0] = "dead";
  else if(ratio[0] <= 10.0)
      result[0] = "critical";
  else if(ratio[0] <= 30.0)
      result[0] = "severely injured";
  else if(ratio[0] <= 45.0)
      result[0] = "moderately injured";
  else if(ratio[0] <= 60.0)
      result[0] = "injured";
  else if(ratio[0] <= 75.0)
      result[0] = "hurt";
  else if(ratio[0] <= 90.0)
      result[0] = "wounded";
  else if(ratio[0] < 100.0)
      result[0] = "bruised and nicked";
  else
      result[0] = "healthy";

  // SP Condition
  if(ratio[1] <= 5.0)
      result[1] = "brain dead";
  else if(ratio[1] <= 15.5)
      result[1] = "depleted";
  else if(ratio[1] <= 30.0)
      result[1] = "unfocused";
  else if(ratio[1] <= 45.0)
      result[1] = "mentally fuzzy";
  else if(ratio[1] <= 60.0)
      result[1] = "losing focus";
  else if(ratio[1] <= 75.0)
      result[1] = "clear-headed";
  else if(ratio[1] <= 90.0)
      result[1] = "sharp";
  else if(ratio[1] < 100.0)
      result[1] = "focused";
  else
      result[1] = "fully charged";

  // MP Condition
  if(ratio[2] <= 5.0)
      result[2] = "exhausted";
  else if(ratio[2] <= 15.5)
      result[2] = "sluggish";
  else if(ratio[2] <= 30.0)
      result[2] = "fatigued";
  else if(ratio[2] <= 45.0)
      result[2] = "tired";
  else if(ratio[2] <= 60.0)
      result[2] = "somewhat tired";
  else if(ratio[2] <= 75.0)
      result[2] = "lively";
  else if(ratio[2] <= 90.0)
      result[2] = "energetic";
  else if(ratio[2] < 100.0)
      result[2] = "very lively";
  else
      result[2] = "full of stamina";

  return result;
}

int is_dead() { return __dead; }
