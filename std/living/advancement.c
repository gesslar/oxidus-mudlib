/**
 * @file /std/user/advancement.c
 * @description Advancement object for players
 *
 * @created 2024-07-24 - Gesslar
 * @last_modified 2024-07-24 - Gesslar
 *
 * @history
 * 2024-07-24 - Gesslar - Created
 */

#include <advancement.h>
#include <gmcp_defines.h>

private float _level = 1.0;
private float _level_mod = 0.0;
private int _xp = 0;

int queryXp() {
  return _xp;
}

float query_tnl() {
  return ADVANCE_D->toNextLevel(_level);
}

float queryLevel() {
  return _level;
}

float query_effective_level() {
  return _level + _level_mod;
}

float setLevel(float l) {
  _level = to_float(l);

  if(userp()) {
    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
      GMCP_LBL_CHAR_STATUS_XP: _xp,
      GMCP_LBL_CHAR_STATUS_TNL: query_tnl(),
      GMCP_LBL_CHAR_STATUS_LEVEL: _level,
    ]));
  }

  return _level;
}

float add_level(float l) {
  _level += to_float(l);

  if(userp()) {
    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
      GMCP_LBL_CHAR_STATUS_XP: _xp,
      GMCP_LBL_CHAR_STATUS_TNL: query_tnl(),
      GMCP_LBL_CHAR_STATUS_LEVEL: _level,
    ]));
  }

  return _level;
}

float query_level_mod() {
  return _level_mod;
}

float set_level_mod(float l) {
  return adjust_level_mod(l - _level_mod);
}

float adjust_level_mod(float l) {
  _level_mod += to_float(l);

  return _level_mod;
}

int adjustXp(int amount) {
  _xp += amount;

  if(userp()) {
    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
      GMCP_LBL_CHAR_STATUS_XP: _xp,
      GMCP_LBL_CHAR_STATUS_TNL: query_tnl(),
      GMCP_LBL_CHAR_STATUS_LEVEL: _level,
    ]));
  }

  return _xp;
}

int setXp(int amount) {
  int delta;

  if(_xp < 1)
    delta = amount;
  else
    delta = amount;

  return adjustXp(delta);
}

void on_advance(object tp, float l) {
  tell(tp, "You have advanced to level " + to_int(l) + "!\n");
}
