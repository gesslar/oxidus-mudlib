/**
 * @file /std/modules/shop.c
 * @description Module to be inherited by shops
 *
 * @created 2024-08-01 - Gesslar
 * @last_modified 2024-08-01 - Gesslar
 *
 * @history
 * 2024-08-01 - Gesslar - Created
 */

#include <classes.h>
#include "/std/object/include/command.h"
#include "/std/object/include/object.h"

inherit M_CURRENCY;

inherit CLASS_STORAGE;

void add_shop_inventory(mixed args);
int query_cost(object tp, object ob, string transaction);
protected void remove_shop();
protected void reset_shop();
private nomask object create_storage();

protected nosave int shop_open = 1;
protected nosave int allow_npcs = 0;
protected nosave float sell_factor = 0.5; // when a player sells, use this
                                          // factor to determine the price
protected nosave string shop_keep_file;
protected nosave object store;
private nosave mixed *shop_inventory = ({});

void init_shop() {
  add_command("buy", "cmd_buy");
  add_command("sell", "cmd_sell");
  add_command("list", "cmd_list");

  create_storage();

  add_reset((:reset_shop:));
  add_destruct((:remove_shop:));
}

protected void remove_shop() {
  if(objectp(store))
    store->remove();
}

void add_shop_inventory(mixed args...) {
  mixed arg;

  if(!pointerp(args))
    args = ({ args });

  foreach(arg in args)
    shop_inventory += ({ arg });
}

protected void reset_shop() {
  mixed arg;

  create_storage();
  store->clean_contents();
return;
  foreach(arg in shop_inventory) {
    string file;
    int number;
    mixed *clone_args;
    int sz;
    object ob;

    if(!pointerp(arg))
      arg = ({ arg });

    sz = sizeof(arg);

    file = arg[0];
    if(!stringp(file))
      continue;

    sz = 0;

    if(sz > 1) {
      number = arg[1];
      if(sz > 2)
        clone_args = clone_args[2..];
      else
        number = 1;
    }

    while(number--) {
      string e;

      e = catch {
        if(sizeof(clone_args))
          ob = new(file, clone_args...);
        else
          ob = new(file);
      };

      if(e) {
        log_file("shop_errors", e);
        continue;
      }

      if(!objectp(ob))
        continue;

      if(ob->move(store))
        ob->remove();
    }
  }
}

mixed cmd_list(object tp) {
  object *items, item;
  string *lines = ({});
  string line;
  object ob;
  string short;
  int cost;

  create_storage();

  items = all_inventory(store);

  lines = ({ get_short(), "" });

  foreach(item in items) {
    cost = query_cost(tp, item, "list");
    short = get_short(item);
    line = sprintf("%s (%d)", short, cost);
    lines += ({ line });
  }

  return lines;
}

mixed cmd_buy(object tp, string str) {
  object ob;
  mixed result;
  string action;
  mixed cost;
  string short;
  mixed *paid, *change;

  create_storage();

  if(!allow_npcs && !userp(tp))
    return 0;

  if(!shop_open)
    return "The shop is closed.";

  if(!userp(tp))
    return "Only players can buy from the shop.";

  if(!ob = present(str, store))
    return "The shop does not have that item.";

  cost = query_cost(tp, ob, "buy");

  result = handle_transaction(tp, cost);

  if(stringp(result))
    return result;

  if(ob->move(tp)) {
    reverse_transaction(tp, result);

    return "You can't carry that much weight.";
  }

  short = get_short(ob);
  tp->other_action("$N $vbuy a $o.", short);

  paid = result[0];
  change = result[1];

  action = "$N $vbuy a $o for $o1";
  if(sizeof(change))
    action += " and receive $o2 in change";

  action += ".";

  tp->my_action(action,
    short,
    format_return_currency_string(paid),
    format_return_currency_string(change)
  );

  return 1;
}

mixed cmd_sell(object tp, string str) {
  object ob, *obs;
  int sz;
  int use_mass = mudConfig("USE_MASS");

  create_storage();

  if(!allow_npcs && !userp(tp))
    return 0;

  if(!shop_open)
    return "The shop is closed.";

  if(!userp(tp))
    return "Only players can sell to the shop.";

  if(!str)
    return "Sell what?";

  if(sscanf(str, "all %s", str))
    obs = find_targets(tp, str, tp);
  else if(str == "all")
    obs = find_targets(tp, 0, tp);
  else {
    if(ob = find_target(tp, str, tp))
      obs = ({ ob });
    else
      return "You don't have that item.";
  }

  if(!sz = sizeof(obs))
    return "You don't have any such items to sell.";

  foreach(ob in obs) {
    int cost = query_cost(tp, ob, "sell");
    string short = get_short(ob);
    int item_mass = ob->query_mass();
    mapping value = least_coins(cost);
    int coins = sum(values(value));
    string mess, *list;

    if(nullp(cost)) {
      tell(tp, "The shop refuses to buy your " + short + ".\n");
      continue;
    }

    if(ob->equipped())
      continue;

    if(use_mass) {
            int fill = tp->query_fill();
            int cap = tp->query_capacity();

      if(fill - item_mass + coins > cap) {
        tell(tp, "You are overburdened and cannot carry the coins.\n");
        continue;
      }
    }

    if(ob->move(store)) {
      tell(tp, "The shop refuses to buy your " + short + ".\n");
      continue;
    }

    list = ({ });
    foreach(string key, int val in value) {
      list += ({ sprintf("%d %s", val, key) });
      tp->adjust_wealth(key, val);
    }

    tp->other_action("$N $vsell $o.", short);
    mess = "You $vsell $o for $o1.\n";
    tp->my_action(mess, short, simple_list(list));
  }

  obs = filter(obs, (: objectp($1) && present($1, $(tp)) :));

  if(sz == sizeof(obs))
    return "You didn't sell anything.";

  return 1;
}

int query_cost(object tp, object ob, int transaction) {
  int value = ob->query_value();

  switch(transaction) {
    case "buy":
      return value;
    case "sell":
      return to_int(to_float(value) * sell_factor);
    case "list":
      return value;
  }

  return ob->query_value();
}

private nomask object create_storage() {
  class StorageOptions storage_options;

  if(objectp(store))
    return store;

  storage_options = new(class StorageOptions,
    storage_type: "public",
    storage_org: "olum_general_store"
  );

  store = load_object(sprintf("storage/%s", storage_options.storage_org));
  store->set_storage_options(storage_options);
  store->set_no_clean(1);

  return store;
}
