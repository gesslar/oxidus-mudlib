/**
 * @file /std/user/advancement.c
 * Advancement object for players
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
private int __xp = 0;

int query_xp() {
  return __xp;
}

float query_tnl() {
  return ADVANCE_D->to_next_level(_level);
}

float query_level() {
  return _level;
}

float query_effective_level() {
  return _level + _level_mod;
}

float set_level(float l) {
  _level = to_float(l);

  if(userp()) {
    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
      GMCP_LBL_CHAR_STATUS_XP: __xp,
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
      GMCP_LBL_CHAR_STATUS_XP: __xp,
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

int adjust_xp(int amount) {
  __xp += amount;

  if(userp()) {
    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
      GMCP_LBL_CHAR_STATUS_XP: __xp,
      GMCP_LBL_CHAR_STATUS_TNL: query_tnl(),
      GMCP_LBL_CHAR_STATUS_LEVEL: _level,
    ]));
  }

  return __xp;
}

int set_xp(int amount) {
  int delta;

  if(__xp < 0)
    delta = amount;
  else
    delta = amount;

  return adjust_xp(delta);
}

void on_advance(object tp, float l) {
  tell(tp, "You have advanced to level " + to_int(l) + "!\n");
}
