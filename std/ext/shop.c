/**
 * @file /std/ext/shop.c
 *
 * Inventory-based shop module. Inherited by rooms that wish to buy
 * and sell items via a persistent storage object. Provides the
 * `buy`, `sell`, and `list` commands and integrates with
 * EXT_CURRENCY for transaction handling. Items registered through
 * add_shop_inventory() are restocked into the storage object on
 * each reset.
 *
 * @created 2024-08-01 - Gesslar
 * @last_modified 2026-05-03 - Gesslar
 *
 * @history
 * 2024-08-01 - Gesslar - Created
 * 2026-05-03 - Gesslar - Added LPCDoc documentation
 */

#include <classes.h>
#include "/std/object/include/command.h"
#include "/std/object/include/object.h"

inherit EXT_CURRENCY;

inherit CLASS_STORAGE;

void add_shop_inventory(mixed args);
int query_cost(object tp, object ob, string transaction);
protected void remove_shop();
protected void reset_shop();
private nomask object create_storage();

protected nosave int __shop_open = 1;
protected nosave int __allow_npcs = 0;
protected nosave float __sell_factor = 0.5; // when a player sells, use this
                                            // factor to determine the price
protected nosave string __shop_keep_file;
/**
 * The persistent storage object holding the shop's inventory for
 * sale. Created lazily by create_storage().
 *
 * @type {STD_STORAGE_OBJECT}
 */
protected nosave object __store;

/**
 * Registered restock entries replayed by reset_shop(). Each entry
 * is either a blueprint file path (string) or an array of the form
 * ({ file, count, ...clone_args }) where count defaults to 1 and
 * clone_args are forwarded to new().
 *
 * @type {mixed*}
 */
private nosave mixed *__shop_inventory = ({});

/**
 * Initialises the shop module. Adds the buy, sell, and list
 * commands, creates the storage object, and registers reset and
 * destruct callbacks. Call from setup() after the inventory has
 * been registered with add_shop_inventory().
 */
void init_shop() {
  add_command("buy", "cmd_buy");
  add_command("sell", "cmd_sell");
  add_command("list", "cmd_list");

  create_storage();

  add_reset((:reset_shop:));
  add_destruct((:remove_shop:));
}

/**
 * Destruct callback that removes the shop's storage object when
 * the shop itself is destroyed.
 */
protected void remove_shop() {
  if(objectp(__store))
    __store->remove();
}

/**
 * Adds entries to the shop's restock list. Each call may pass one
 * or more entries. Each entry is either a blueprint file path or
 * an array of the form ({ file, count, ...clone_args }) where
 * count defaults to 1 and clone_args are forwarded to new() when
 * the item is restocked.
 *
 * @param {mixed*} args - One or more inventory entries.
 */
void add_shop_inventory(mixed args...) {
  mixed arg;

  if(!pointerp(args))
    args = ({ args });

  foreach(arg in args)
    __shop_inventory += ({ arg });
}

/**
 * Reset callback that wipes the storage object and restocks it
 * from __shop_inventory. Errors raised while cloning an entry are
 * logged to "shop_errors" and the entry is skipped.
 */
protected void reset_shop() {
  mixed arg;

  create_storage();
  __store->clean_contents();

  foreach(arg in __shop_inventory) {
    string file;
    int number = 1;
    mixed *clone_args;
    int sz;
    /** @type {STD_ITEM} */ object ob;

    if(!pointerp(arg))
      arg = ({ arg });

    sz = sizeof(arg);

    file = arg[0];
    if(!stringp(file))
      continue;

    if(sz > 1)
      number = arg[1];
    if(sz > 2)
      clone_args = arg[2..];

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

      if(ob->move(__store))
        ob->remove();
    }
  }
}

/**
 * Implements the `list` command. Returns the shop's storefront
 * heading followed by one line per item in storage, each formatted
 * as "<short> (<cost>)".
 *
 * @param {STD_BODY} tp - The acting body (unused).
 * @param {string} _str - The command argument (unused).
 * @returns {string*} The lines to display.
 */
mixed cmd_list(object tp, string _str) {
  object *items, item;
  string *lines = ({});
  string line;
  string short;
  int cost;

  create_storage();

  items = all_inventory(__store);

  lines = ({ get_short(), "" });

  foreach(item in items) {
    cost = query_cost(tp, item, "list");
    short = get_short(item);
    line = sprintf("%s (%d)", short, cost);
    lines += ({ line });
  }

  return lines;
}

/**
 * Implements the `buy` command. Locates the named item in the
 * storage object, charges the buyer via handle_transaction(), and
 * moves the item into the buyer's inventory. On move failure the
 * payment is reversed via reverse_transaction().
 *
 * @param {STD_BODY} tp - The buyer.
 * @param {string} str - The argument identifying the item.
 * @returns {int | string} 1 on success, 0 if the buyer is an NPC
 *                         and NPCs are not allowed, or an error
 *                         message on failure.
 */
mixed cmd_buy(object tp, string str) {
  /** @type {STD_ITEM} */
  object ob;
  mixed result;
  string action;
  mixed cost;
  string short;
  mixed *paid, *change;

  create_storage();

  if(!__allow_npcs && !userp(tp))
    return 0;

  if(!__shop_open)
    return "The shop is closed.";

  if(!userp(tp))
    return "Only players can buy from the shop.";

  if(!ob = present(str, __store))
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

/**
 * Implements the `sell` command. Accepts a single target, "all",
 * or "all <id>". For each matching item the seller is paid via
 * least_coins() based on query_cost(..., "sell"), the item is
 * moved into storage, and the seller's wealth is adjusted per
 * denomination. Equipped items and items the shop refuses to
 * value (cost == null) are skipped, as are items that would
 * overburden the seller when USE_MASS is enabled.
 *
 * @param {STD_BODY} tp - The seller.
 * @param {string} str - The argument identifying items to sell.
 * @returns {int | string} 1 if at least one item was sold, 0 if
 *                         the seller is an NPC and NPCs are not
 *                         allowed, or an error message on failure.
 */
mixed cmd_sell(object tp, string str) {
  /** @type {STD_ITEM | STD_ARMOUR | STD_CLOTHING | STD_WEAPON} */ object ob;
  /** @type {STD_ITEM*} */ object *obs;
  int sz;
  int use_mass = mud_config("USE_MASS");

  create_storage();

  if(!__allow_npcs && !userp(tp))
    return 0;

  if(!__shop_open)
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

    if(/** @type {STD_ARMOUR|STD_CLOTHING|STD_WEAPON} */ (ob)->equipped())
      continue;

    if(use_mass) {
      int fill = tp->query_fill();
      int cap = tp->query_capacity();

      if(fill - item_mass + coins > cap) {
        tell(tp, "You are overburdened and cannot carry the coins.\n");
        continue;
      }
    }

    if(ob->move(__store)) {
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

/**
 * Returns the price of an item for a given transaction context.
 * For "buy" and "list" the item's value is returned unchanged. For
 * "sell" the value is multiplied by __sell_factor. Override in a
 * subclass to implement haggling, faction pricing, or shop refusal
 * (return null to refuse to buy a sold item).
 *
 * @param {STD_BODY} _tp - The acting body (unused by default).
 * @param {STD_ITEM} ob - The item being priced.
 * @param {string} transaction - One of "buy", "sell", or "list".
 * @returns {int} The price in base (copper) units.
 */
int query_cost(object _tp, object ob, string transaction) {
  int value = ob->query_value();

  switch(transaction) {
    case "buy":
      return value;
    case "sell":
      return to_int(to_float(value) * __sell_factor);
    case "list":
      return value;
  }

  return ob->query_value();
}

/**
 * Lazily loads and configures the storage object that holds the
 * shop's inventory. Returns the existing object on subsequent
 * calls.
 *
 * @returns {STD_STORAGE_OBJECT} The shop's storage object.
 */
private nomask object create_storage() {
  class StorageOptions storage_options;

  if(objectp(__store))
    return __store;

  storage_options = new(class StorageOptions,
    storage_type: "public",
    storage_org: "olum_general_store"
  );

  __store = load_object(sprintf("storage/%s", storage_options.storage_org));
  __store->set_storage_options(storage_options);
  __store->set_no_clean(1);

  return __store;
}
