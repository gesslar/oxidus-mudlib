/**
 * @file /std/user/player.c
 * @description Player object for user characters.
 *
 * @created 2024-07-29 - Gesslar
 * @last_modified 2024-07-29 - Gesslar
 *
 * @history
 * 2024-07-29 - Gesslar - Created
 */

#include <commands.h>
#include <origin.h>
#include <gmcp_defines.h>
#include <logs.h>
#include <player.h>

inherit STD_BODY;

inherit __DIR__ "living";

inherit M_GMCP;

private nosave mapping environ_data = ([]);
private int _last_login = 0;
private int ed_setup = 0;

/* Connection functions */
void setup_body() {
  set_living_name(query_real_name());
  set_id(({query_real_name()}));
  // set_real_name(name());
  set_heart_beat(mudConfig("DEFAULT_HEART_RATE"));
  if(!query_race()) set_race(mudConfig("DEFAULT_RACE"));
  if(!queryLevel()) setLevel(1.0);
  set_level_mod(0.0);
  if(!query_env("cwd")) set_env("cwd", "/doc");
  if(!query_short()) set_short(query_name());
  if(!query_pref("colour")) set_pref("colour", "on");
  if(!query_pref("auto_tune")) set_pref("auto_tune", "all");
  if(!query_pref("biff")) set_pref("biff", "on");
  if(!query_pref("prompt")) set_pref("prompt", ">");
  init_living();
  rehash_capacity();
  update_regen_interval();

  set_log_prefix(sprintf("(%O)", this_object()));

  slot(SIG_SYS_CRASH, "on_crash");
  slot(SIG_PLAYER_ADVANCED, "on_advance");
}

void enter_world(int reconnecting) {
  string *cmds, *ch;
  int i;
  object mail_client;

  if(!is_member(query_privs(previous_object()), "admin"))
    return;

  catch {
    ch = explode(query_pref("auto_tune"), " ");
    if(sizeof(ch) > 0)
      foreach(string channel in ch) {
        CMD_CHANNEL->tune(channel, query_privs(this_object()), 1, 1);
      }
  };

  set_last_login(time());
  tell_me("\n");
  tell_them(capitalize(query_name()) + " has entered.\n");

  if(!reconnecting) {
    restore_inventory();
    rehash_capacity();
  }
}

void exit_world() {
  string *cmds;
  int i;

  if(this_body() != this_object()) return;

  if(file_size(home_path(query_real_name()) + ".quit") > 0) {
    cmds = explode(read_file(home_path(query_real_name()) + ".quit"), "\n");
    if(sizeof(cmds) <= 0) return;
    for(i = 0; i < sizeof(cmds); i ++) catch(command(cmds[i]));
  }

  set_last_login(time());

  if(environment())
    tell_them(query_name()+ " leaves " + mud_name() + ".\n");

  save_body();
}

void set_last_login(int time) {
  _last_login = time;
}

int query_last_login() {
  return _last_login;
}

void net_dead() {
  if(origin() != ORIGIN_DRIVER)
    return;

  abort_edit();

  set_last_login(time());
  save_body();

  if(environment())
    tell_all(environment(), query_name()+ " falls into stupour.\n");

  add_extra_short("link_dead", "[stupour]");
  log_file(LOG_LOGIN, query_real_name() + " went link-dead on " + ctime(time()) + "\n");

  if(interactive(this_object()))
    emit(SIG_USER_LINKDEAD, this_object());
}

void reconnect() {
  restore_body();
  set_last_login(time());
  tell("You have reconnected to your body.\n");
  if(environment()) tell_them(query_name() + " has reconnected.\n");
  remove_extra_short("link_dead");
  /* reconnection logged in login object */
}

/* Body Object Functions */
void heart_beat() {
  clean_up_enemies();
  cooldown();

  if(userp()) {
    if(!interactive(this_object())) {
      if((time() - query_last_login()) > 3600) {
        if(environment())
          simple_action("$N $vfade out of existance.");
        log_file(LOG_LOGIN, query_real_name() + " auto-quit after 1 hour of net-dead at " + ctime(time()) + ".\n");
        remove();
        return;
      }
    } else {
      /* Prevent link-dead from idle */
      if(query_idle(this_object()) % 60 == 0 && query_idle(this_object()) > 300
          && query_pref("keepalive") && query_pref("keepalive") != "off") {
        telnet_nop();
      }
    }
  }

  if(!is_dead() && query_hp() <= 0.0) {
    set_dead(1);
    die();
    return;
  }

  heal_tick();
  evaluate_heart_beat();
  process_boon();

  if(gmcp_enabled())
    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, null);
}

int supports_unicode() {
  if(has_screenreader()) return 0;
  return query_pref("unicode") == "on";
}

void on_crash(mixed arg...) {
  int result;

  if(previous_object() != signal_d())
    return;

  catch(result = save_body());
}

void mudlib_unsetup() {
  remove_all_modules();

  if(interactive())
    emit(SIG_USER_LOGOUT, this_object());
}

/**
 * @description This function is called by the driver when the environment of
 *              an object is destructed. It will attempt to move the object to
 *              the void or hall if possible. If it cannot move the object, it
 *              will attempt to save the object and then destruct it.
 * @param {object} ob - The environment of the object that was destructed.
 */
void move_or_destruct(object ob) {
  object env = environment();
  object new_env;

  // If we didn't come from the driver, we don't need to do anything
  if(origin() != ORIGIN_DRIVER)
    return;

  // Were we in the VOID? If not, temporarily set the new destination
  // to the VOID.
  if(!ob && env != find_object(ROOM_VOID))
    catch(ob = load_object(ROOM_VOID));

  // Were we in the starting room? If not, temporarily set the new
  // destination to the starting room.
  if(!ob && env != find_object(ROOM_START))
    catch(ob = load_object(ROOM_START));

  // If we still don't have a destination, we need to clone a copy of a
  // safe room that doesn't have anything in it, but will allow us to
  // move the object to it. Which just happens to be the void. So, let's get
  // a copy of the void.
  if(!ob) {
    /* This is bad.  Try to save them anyway. */
    catch(ob = new(ROOM_VOID));
    if(!ob)
      return;

    // We don't want this object persisting.
    ob->set_no_clean(0);
  }

  move_object(ob);
}

mixed query_environ(string key) {
  return environ_data[key];
}

void clear_environ_data() {
  environ_data = ([ ]);
}

mapping query_all_environ() {
  return copy(environ_data);
}

void set_environ_option(string key, mixed value) {
  environ_data[key] = fromString(value);

  _log(1, "Setting environ option: %s = %O", key, value);
}

void receive_environ(string var, mixed value) {
  set_environ_option(var, value);
}

void set_environ(mapping data) {
  if(base_name(previous_object()) != LOGIN_OB)
    return;

  if(!mapp(data))
    return;

  foreach(string key, mixed value in data) {
    if(stringp(value))
      set_environ_option(key, value);
    else
      environ_data[key] = value;
  }
}

void save_inventory() {
  string save;

  if(!is_member(query_privs(previous_object() ? previous_object() : this_body()), "admin") && this_body() != this_object()) return 0;

  save = save_to_string(1);
  write_file(user_inventory_data(query_privs(this_object())), save, 1);
}

void restore_body() {
  if(!is_member(query_privs(previous_object() ? previous_object() : this_body()), "admin") && this_body() != this_object()) return 0;
  if(is_member(query_privs(previous_object()), "admin") || query_privs(previous_object()) == this_body()->query_real_name()) restore_object(user_body_data(query_real_name()));
}

void restore_inventory() {
  string e;
  string file, data;

  if(!is_member(query_privs(previous_object() ? previous_object() : this_body()), "admin") && this_body() != this_object()) return 0;

  file = user_inventory_data(query_privs(this_object()));

  if(file_exists(file)) {
    e = catch {
      data = read_file(file);
      load_from_string(data, 1);
    };

    if(e) {
      tell_me("Error [restore_inventory]: Unable to restore inventory data.\n");
    }
  }

  wipe_inventory();
}

void wipe_inventory() {
  string file;

  if(!is_member(query_privs(previous_object() ? previous_object() : this_body()), "admin") && this_body() != this_object()) return 0;

  file = user_inventory_data(query_real_name());
  rm(file);
}

int save_body() {
  int result;

  if(!is_member(query_privs(previous_object() ? previous_object() : this_body()), "admin") &&
    this_body() != this_object() &&
    base_name(previous_object()) != CMD_QUIT) return 0;

  catch(result = save_object(user_body_data(query_real_name())));

  save_inventory();

  return result;
}

int has_screenreader() {
  if(query_environ("SCREEN_READER") == true)
    return true;

  if(query_pref("screenreader") == "on")
    return true;

  return false;
}

int set_ed_setup(int value) {
  ed_setup = value;
  return 1;
}

void event_gmcp_item_remove(object item, object prev) {
  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_ITEMS_REMOVE, ({ item, prev }));
}

void event_gmcp_item_add(object item, object dest) {
  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_ITEMS_ADD, ({ item, dest }));
}

void event_gmcp_item_update(object item, object dest) {
  GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_ITEMS_UPDATE, ({ item, dest }));
}

int query_ed_setup() { return ed_setup ; }

int reclaim_this_player() {
  evaluate(bind(enable_commands, this_object()));

  return this_player() == this_object();
}

int is_pc() { return 1 ; }
