/**
 * @file /std/living/body.c
 * @description Body object that is shared by players and NPCs.
 *
 * @created 2024-07-29 - Gesslar
 * @last_modified 2024-07-29 - Gesslar
 *
 * @history
 * 2024-07-29 - Gesslar - Created
 */

#include <body.h>
#include <commands.h>
#include <env.h>
#include <player.h>
#include <gmcp_defines.h>
#include <driver/origin.h>

inherit STD_CONTAINER;
inherit STD_ITEM;

inherit __DIR__ "act";
inherit __DIR__ "advancement";
inherit __DIR__ "alias";
inherit __DIR__ "appearance";
inherit __DIR__ "attributes";
inherit __DIR__ "boon";
inherit __DIR__ "combat";
inherit __DIR__ "communication";
inherit __DIR__ "ed";
inherit __DIR__ "env";
inherit __DIR__ "equipment";
inherit __DIR__ "module";
inherit __DIR__ "pager";
inherit __DIR__ "race";
inherit __DIR__ "skills";
inherit __DIR__ "visibility";
inherit __DIR__ "vitals";
inherit __DIR__ "wealth";

inherit M_ACTION;
inherit M_LOG;

/* Prototypes */

void mudlib_setup() {
  if(!clonep() &&
     origin() != ORIGIN_LOCAL &&
     previous_object() != body_d()
    ) {
    return;
  }

  enable_commands();
  add_standard_paths();
  if(wizardp())
    add_wizard_paths();

  if(!query_pref("prompt"))
    set_pref("prompt", ">");
  set_log_level(0);
  set_prevent_get(1);
  // TODO figure out how we were getting this twice and then
  //      fix that and then remove remove_action() heres
  remove_action("command_hook", "");
  add_action("command_hook", "", 1);
  set_ignore_mass(1);
}

private nosave string *body_slots = ({
  "head", "neck", "torso", "back", "arms", "hands", "legs", "feet"
});

private nosave string *weapon_slots = ({
  "right hand", "left hand"
});

string *query_body_slots() {
  return copy(body_slots);
}

string *query_weapon_slots() {
  return copy(weapon_slots);
}

void die() {
  object
  /** @type {STD_BODY} */   body,
  /** @type {LIB_CORPSE} */ corpse,
  /** @type {STD_ITEM} */   ob,
  /** @type {STD_ITEM} */   next;

  if(!environment())
    return;

  if(!is_dead())
    return;

  stop_all_attacks();

  if(objectp(body = query_su_body())) {
    exec(body, this_object());
    body->move(environment());
    body->simple_action("$N $vis violently ejected from the body of $o.", this_object());
    clear_su_body();
  }

  simple_action("$N $vhave perished.");
  save_body();
  emit(SIG_PLAYER_DIED, this_object(), killed_by());

  corpse = new(LIB_CORPSE);
  corpse->setup_corpse(this_object(), killed_by());

  if(function_exists("query_loot_table"))
    LOOT_D->loot_drop(killed_by(), this_object());
  if(function_exists("query_coin_table"))
    LOOT_D->coin_drop(killed_by(), this_object());

  ob = first_inventory(this_object());
  while(ob) {
    next = next_inventory(ob);
    if(ob->move(corpse))
      ob->remove();
    ob = next;
  }

  // Now move coin objects to the corpse
  if(query_total_wealth()) {
    mapping wealth = query_all_wealth();
    foreach(string currency, int amount in wealth) {
      object coin = new(LIB_COIN);
      coin->set_up(currency, amount);
      if(coin->move(corpse))
        if(coin)
          coin->remove();
    }
  }

  if(corpse->move(environment()))
    corpse->remove();

  if(userp())  {
    object ghost = BODY_D->create_ghost(query_privs());
    exec(ghost, this_object());
    ghost->setup_body();
    ghost->set_hp(-1.0);
    ghost->set_sp(-1.0);
    ghost->set_mp(-1.0);
    ghost->move(environment());
  } else {
    ADVANCE_D->killXp(killed_by(), this_object());
  }

  remove();
}

varargs int move(mixed ob) {
  int result;
  object env;

  env = environment();
  result = ::move(ob);

  if(result)
    return result;

  if(env)
    set_last_location(env);

  return result;
}

void event_remove(object _prev) {
  object
  /** @type {STD_ITEM} */ ob,
  /** @type {STD_ITEM} */ next;

  ob = first_inventory();
  while(ob) {
    next = next_inventory(ob);
    if(call_if(ob, "prevent_drop")) {
      ob->remove();
    } else {
      if(environment()) {
        int result = ob->move(environment());
        if(result)
          ob->remove();
      }
    }
    ob = next;
  }
}

varargs int move_living(mixed dest, string dir, string depart_message, string arrive_message) {
  int result;
  object curr = environment();
  string tmp;

  result = move(dest);
  if(result)
    return result;

  if(is_acting()) {
    tell(this_object(), "You stop what you are doing.\n");
    cancel_acts();
  }

  if(curr) {
    if(depart_message != "SILENT") {
      depart_message = depart_message || query_env("move_out") || "$N leaves $D.";
      dir = dir || "somewhere";

      tmp = replace_string(depart_message, "$N", query_name());
      tmp = replace_string(tmp, "$D", dir);

      tmp = append(tmp, "\n");

      tell_down(curr, tmp);
    }
  }

  if(arrive_message != "SILENT") {
    curr = environment();

    arrive_message = arrive_message || query_env("move_in") || "$N arrives.\n";
    tmp = replace_string(arrive_message, "$N", query_name());

    tmp = append(tmp, "\n");

    tell_down(curr, tmp, null, ({ this_object() }));
  }

  force_me("look");

  GMCP_D->send_gmcp(this_object(), GMCP_PKG_ROOM_INFO, environment());

  return result;
}

/** Body bodily functions */

/**
 * Reports whether the body is presently able. This is a general status,
 * such as, are they a player or monster and alive, can they manipulate
 * things, like not stunned, etc.
 *
 * TODO: For now, just returns 1 until more is implemented that will affect
 * this.
 *
 * @returns {1|string} Returns 1 if passes ability check, otherwise a string error message.
 */
int is_able() {
  return 1;
}

//Misc functions
void write_prompt() {
  string prompt = query_pref("prompt");

  receive(prompt + " ");
}

int query_log_level() {
  if(!query_pref("log_level")) return 0;

  return to_int(query_pref("log_level"));
}

/* Switch User Functionality */

/**
 * This variable holds the body we are inhabiting if not our own.
 *
 * @type {STD_BODY}
 */
object su_body;

/**
 * Register the body we have moved into.
 *
 * @param {STD_BODY} source - The body we are now inhabiting.
 */
void set_su_body(object source) {
  su_body = source;
}

/**
 * Returns the body we are inhabiting. Will be undefined if
 * we are already in our own body.
 *
 * @returns {STD_BODY|0} The body we are currently inhabiting, or null.
 */
object query_su_body() {
  return su_body;
}

/**
 * Resets our current switch user body to null.
 */
void clear_su_body() {
  su_body = null;
}
