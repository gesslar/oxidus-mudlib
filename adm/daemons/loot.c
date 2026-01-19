/**
 * @file /adm/daemons/loot.c
 * @description This daemon is responsible for handling the loot system.
 *
 * @created 2024-08-05 - Gesslar
 * @last_modified 2024-08-05 - Gesslar
 *
 * @history
 * 2024-08-05 - Gesslar - Created
 */

inherit STD_DAEMON;
inherit M_LOG;

private void drop_items(object tp, mixed item, object source);
private mixed process_loot_item(mixed item, object tp, object source);
private void autovalue_loot_item(mixed item, object tp, object source);
private string determine_currency_type(int value);
private void drop_coins(object tp, mixed item, object source);
public int determine_value_by_level(int level);

void setup() {
  set_log_level(0);
}

/**
 * @daemon_function loot_drop
 * @description Drops loot from the loot table from the source into the source.
 * @param {object} tp - The player to drop the loot for.
 * @param {object} source - The source of the loot.
 */
void loot_drop(object tp, object source) {
  mixed *loot_table, *loot;
  float chance;

  loot_table = source->query_loot_table();

  if(sizeof(loot_table)) {
    foreach(loot in loot_table) {
      mixed item = loot[0];
      float roll;
      chance = loot[1];

      roll = random_float(100.0);

      if(roll < chance) {
        catch(drop_items(tp, item, source));
      }
    }
  }
}

/**
 * @daemon_function coin_drop
 * @description Drops coins from the coin table from the source into the source.
 * @param {object} tp - The player to drop the coins for.
 * @param {object} source - The source of the coins.
 */
void coin_drop(object tp, object source) {
  mixed *coin_table, *loot;
  float chance;
  float roll;

  coin_table = source->query_coin_table();

  if(sizeof(coin_table)) {
    foreach(loot in coin_table) {
      mixed item = loot[0..1];
      chance = loot[2];

      roll = random_float(100.0);

      if(roll < chance)
        catch(drop_coins(tp, item, source));
    }
  }
}


/**
 * @function drop_items
 * @description Drops items from the item into the source.
 * @param {object} tp - The player to drop the items for.
 * @param {mixed} item - The item to drop.
 * @param {object} source - The source to drop the items into.
 */
private void drop_items(object tp, mixed item, object source) {
  mixed processed_item;
  object loot_ob;
  string file;
  mixed args;

  processed_item = process_loot_item(item, tp, source);

  if(pointerp(processed_item) && sizeof(processed_item) == 2) {
    file = processed_item[0];
    args = processed_item[1];
    if(pointerp(args))
      loot_ob = new(file, args...);
    else
      loot_ob = new(file, args);
  } else if(stringp(processed_item)) {
    file = processed_item;
    loot_ob = new(file);
  } else
    return;

  if(loot_ob) {
    int move_result;

    autovalue_loot_item(loot_ob, tp, source);

    move_result = loot_ob->move(source);
    if(move_result)
      loot_ob->remove();
  }
}

/**
 * @function process_loot_item
 * @description Processes the loot item.
 * @param {mixed} item - The item to process.
 * @param {object} tp - The player to process the item for.
 * @param {object} source - The source to process the item for.
 * @returns {mixed} - The processed item.
 */
private mixed process_loot_item(mixed item, object tp, object source) {
  int size, index;
  mixed selected;

  if(valid_function(item)) {
    function f = item;

    return process_loot_item(f(tp, source), tp, source);
  }

  if(pointerp(item)) {
    size = sizeof(item);
    if(size == 0)
      return 0;

    index = random(size);
    selected = item[index];

    if(stringp(selected)) {
      if(index < size - 1 && pointerp(item[index + 1])) {
        return ({ selected, item[index + 1] });
      } else {
        return selected;
      }
    } else if(pointerp(selected) && index > 0 && stringp(item[index - 1]))
      return ({ item[index - 1], selected });

    return 0;
  }

  if(mapp(item))
    return process_loot_item(element_of_weighted(item), tp, source);
  if(stringp(item))
    return item;

  return 0;
}

/**
 * @function autovalue_loot_item
 * @description Autovalue the loot item if the autovalue property is set.
 * @param {mixed} item - The item to autovalue.
 * @param {object} tp - The player to autovalue the item for.
 * @param {object} source - The source to autovalue the item for.
 */
private void autovalue_loot_item(mixed item, object tp, object source) {
  // Determine if we have autovalue on this item
  if(item->query_loot_property("autovalue") == true) {
    int value = determine_value_by_level(source->query_level());

    item->set_value(value);
  }
}

/**
 * @function drop_coins
 * @description Drops coins from the coin table from the source into the source.
 * @param {object} tp - The player to drop the coins for.
 * @param {mixed} item - The value array of currency type and number.
 * @param {object} source - The source to drop the coins into.
 */
private void drop_coins(object tp, mixed item, object source) {
  string type;
  int num;
  object coin_ob;
  int move_result;

  if(sizeof(item) == 2) {
    type = item[0];
    num = item[1];
  } else
    return;

  coin_ob = new(LIB_COIN, type, num);

  move_result = coin_ob->move(source);
  if(move_result)
    coin_ob->remove();
}

/**
 * @daemon_function determine_value_by_level
 * @description Determines the value of the coins based on the level of the source.
 * @param {int} level - The level of the source.
 * @returns {int} - The value of the coins.
 */
public int determine_value_by_level(int level) {
  int value;
  float config_variance;
  int value_per_level, ivariance, subtract;
  float variance;

  value_per_level = mudConfig("COIN_VALUE_PER_LEVEL");
  config_variance = mudConfig("COIN_VARIANCE");

  value = level * value_per_level;
  variance = config_variance * to_float(value);
  ivariance = to_int(variance);
  subtract = ivariance / 2;
  value -= subtract;
  value += random(ivariance + 1);

  return to_int(value);
}
