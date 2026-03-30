/**
 * @file /std/consume/drink.c
 * @description Drink inheritable for objects that can be consumed.
 *
 * @created 2024-08-06 - Gesslar
 * @last_modified 2024-08-06 - Gesslar
 *
 * @history
 * 2024-08-06 - Gesslar - Created
 */

inherit STD_ITEM;

inherit EXT_POTABLE;

string consume_message();

void mudlib_setup() {
  set_potable(1);
  save_var("_uses", "_max_uses", "_use_status_message");
  add_extra_long("consume", (: consume_message :));
}

void set_id(mixed str) {
  ::set_id(str);

  add_id("drink");
}

private mixed try_to_drink(object ob, string arg, string verb) {
  if(environment() != previous_object())
    return "You must be holding something to " + verb + " it.";

  return 1;
}

public mixed direct_drink_obj(object ob, string arg) { return try_to_drink(ob, arg, "drink"); }
public mixed direct_sip_obj(object ob, string arg) { return try_to_drink(ob, arg, "sip"); }

/**
 *
 * @param {STD_BODY} user - The user drinking the object.
 * @param {string} arg - The argument supplied by the user.
 * @returns {int} The result of the drinking.
 */
public mixed e_drink_obj(object user, string arg) {
  mixed result = drink(user);

  if(result == 1 && query_uses() < 1) {
    this_body()->my_action("$N $vhave drunk the last of the $o.", this_object());
    remove();
  }

  return result;
}

public int e_sip_obj(object user, string arg) {
  mixed result = sip(user, 1);

  if(result == 1 && query_uses() < 1) {
    this_body()->my_action("$N $vhave drunk the last of the $o.", this_object());
    remove();
  }

  return result;
}

public string consume_message() {
  int left;
  string mess;

  left = percent(query_uses(), query_max_uses());
  switch(left) {
    case 100:
      mess = sprintf("This %s is full.", query_name());
      break;
    case 80..99:
      mess = sprintf("This %s has barely been touched.", query_name());
      break;
    case 50..79:
      mess = sprintf("A lot of this %s has been drunk.", query_name());
      break;
    case 25..49:
      mess = sprintf("Most of this %s has been drunk.", query_name());
      break;
    case 0..24:
      mess = sprintf("There is very little left of this %s.", query_name());
      break;
  }

  return mess;
}

public int is_drink() { return 1; }
