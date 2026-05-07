// /std/user/wealth.c
// Wealth management for livings
//
// Created:     2024/02/19: Gesslar
// Last Change: 2024/02/19: Gesslar
//
// 2024/02/19: Gesslar - Created

#include <wealth.h>
#include <daemons.h>
#include <gmcp_defines.h>
#include "/std/object/include/contents.h"

private nomask mapping _wealth = ([]);

int query_total_coins() {
  return sum(values(_wealth));
}

int query_total_wealth() {
  int total = 0;
  mixed *config = mud_config("CURRENCY");

  foreach(mixed *c in config)
    total += _wealth[c[0]] * c[1];

  return total;
}

mapping query_all_wealth() {
  return copy(_wealth);
}

int query_wealth(string currency) {
  return _wealth[currency];
}

mixed adjust_wealth(string currency, int amount) {
  int mass;

  if(nullp(_wealth))
    _wealth = ([]);

  if(!CURRENCY_D->valid_currency_type(currency))
    return "That is not a valid currency type.\n";

  if(amount < 0)
    if(_wealth[currency] - amount < 0)
      return "You don't have that many coins.\n";

  if(mud_config("USE_MASS")) {
    mass = amount;
    if(!can_hold_mass(mass))
      return "You are overburdened and cannot carry the coins.\n";
  }

  _wealth[currency] += amount;

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
    GMCP_LBL_CHAR_STATUS_WEALTH : ([ currency : sprintf("%d", _wealth[currency]) ])
  ]));

  rehash_capacity();

  return _wealth[currency];
}

mapping set_wealth(mapping w) {
  mixed *config = mud_config("CURRENCY");

  wipe_wealth();

  foreach(mixed *c in config) {
    if(!w[c[0]]) {
      w[c[0]] = 0;
      continue;
    }

    if(!adjust_wealth(c[0], w[c[0]]))
      w[c[0]] = 0;
  }

  rehash_capacity();

  return _wealth = w;
}

void init_wealth() {
  if(nullp(_wealth))
    _wealth = ([]);
}

void wipe_wealth() {
  _wealth = ([]);

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_STATUS, ([
    GMCP_LBL_CHAR_STATUS_WEALTH : ([ ])
  ]));

  rehash_capacity();
}
