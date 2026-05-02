/**
 * @file /std/ext/edible.c
 * @description This module is inherited in order to make something edible.
 *
 * @created 2024-08-06 - Gesslar
 * @last_modified 2024-08-06 - Gesslar
 *
 * @history
 * 2024-08-06 - Gesslar - Created
 */

inherit EXT_USES;

private nomask int _edible = null;
private nomask mapping _default_actions = ([
  "consume": "$N $veat a $o.",
  "nibble" : "$N $vnibble on a $o.",
]);

private nomask mapping _actions = ([
  "consume" : ([
    "action": null,
    "self"  : null,
    "room"  : null
  ]),
  "nibble" : ([
    "action": null,
    "self"  : null,
    "room"  : null
  ]),
]);

void set_consume_action(string action) {
  _actions["consume"]["action"] = action;
}

void set_self_consume_action(string action) {
  _actions["consume"]["self"] = action;
}

void set_room_consume_action(string action) {
  _actions["consume"]["room"] = action;
}

void set_nibble_action(string action) {
  _actions["nibble"]["action"] = action;
}

void set_self_nibble_action(string action) {
  _actions["nibble"]["self"] = action;
}

void set_room_nibble_action(string action) {
  _actions["nibble"]["room"] = action;
}

int set_edible(int edible) {
  _edible = edible;

  return _edible;
}

int is_edible() {
  return _edible;
}

int consume(object tp) {
  if(!_edible)
    return 0;

  if(nullp(adjust_uses(-query_uses())))
    return 0;

  if(_actions["consume"]["action"]) {
    tp->simple_action(_actions["consume"]["action"], this_object());
  } else {
    if(!_actions["consume"]["self"] && !_actions["consume"]["room"]) {
      tp->simple_action(_default_actions["consume"], this_object());
    } else {
      if(_actions["consume"]["self"])
        tp->my_action(_actions["consume"]["self"], this_object());
      else
        tp->my_action(_default_actions["consume"], this_object());

      if(_actions["consume"]["room"])
        tp->other_action(_actions["consume"]["room"], this_object());
      else
        tp->other_action(_default_actions["consume"], this_object());
    }
  }

  return 1;
}

int nibble(object tp, int amount) {
  if(!_edible)
    return 0;

  if(nullp(adjust_uses(-amount)))
    return 0;

  if(_actions["nibble"]["action"]) {
    tp->simple_action(_actions["nibble"]["action"], this_object());
  } else {
    if(!_actions["nibble"]["self"] && !_actions["nibble"]["room"]) {
      tp->simple_action(_default_actions["nibble"], this_object());
    } else {
      if(_actions["nibble"]["self"])
        tp->my_action(_actions["nibble"]["self"], this_object());
      else
        tp->my_action(_default_actions["nibble"], this_object());

      if(_actions["nibble"]["room"])
        tp->other_action(_actions["nibble"]["room"], this_object());
      else
        tp->other_action(_default_actions["nibble"], this_object());
    }
  }

    return 1;
}

void reset_edible() {
  reset_uses();
}

mixed try_to_eat(object ob, string arg) {
  if(environment() != previous_object())
    return "#You must be holding something to eat it.";

  return 1;
}
