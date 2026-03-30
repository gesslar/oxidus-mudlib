/**
 * @file /std/consume/food.c
 * @description Food inheritable for objects that can be consumed.
 *
 * @created 2024-08-06 - Gesslar
 * @last_modified 2024-08-06 - Gesslar
 *
 * @history
 * 2024-08-06 - Gesslar - Created
 */

inherit STD_ITEM;
inherit EXT_EDIBLE;

string consume_message();

void mudlib_setup() {
  set_edible(1);
  save_var("_uses", "_max_uses", "_use_status_message");
  add_extra_long("consume", (: consume_message :));
}

void set_id(mixed str) {
  ::set_id(str);
  add_id("food");
}

int consume(object user) {
  int consumed;
  string mess;

  if(!::consume(user, 1))
    return 0;

  if(query_uses() < 1)
    remove();

  return 1;
}

int nibble(object user) {
  if(!::nibble(user, 1))
    return 0;

  if(query_uses() < 1)
    remove();

  return 1;
}

string consume_message() {
  int left;
  string mess;

  left = percent(query_uses(), query_max_uses());
  switch(left) {
    case 100:
      mess = sprintf("This %s hasn't been touched.", query_name());
      break;
    case 80..99:
      mess = sprintf("This %s has been nibbled on.", query_name());
      break;
    case 50..79:
      mess = sprintf("A lot of this %s has been eaten.", query_name());
      break;
    case 25..49:
      mess = sprintf("Most of this %s has been eaten.", query_name());
      break;
    case 0..24:
      mess = sprintf("There is very little left of this %s.", query_name());
      break;
  }

  return mess;
}

int is_food() { return 1 ; }
