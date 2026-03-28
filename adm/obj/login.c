/*login.c

* Tacitus @ LPUniversity
* 04-APR-05
* Login Daemon

* Recoded on October 7th, 2005 by Tacitus

*/

/* Last edited on October 6th, 2006 by Tacitus */

#include <daemons.h>
#include <gmcp_defines.h>
#include <logs.h>
#include <move.h>
#include <origin.h>

inherit STD_ITEM;

inherit M_MESSAGING;
inherit M_GMCP;
inherit M_LOG;

private nosave mapping login_gmcp_data = ([ "client" : null, "supports" : null, ]);

private nosave mapping __environ_data = ([]);
private nosave mapping account = null;

void get_account(string str);
void get_password(string str, int i);
void verify_password(string str, int i);
void new_account(string str, string name);
void auto_destruct();
void reconnect(string str, string name);
void setup_new();
void enter_world(string name, int reconnecting);
void idle_email(string str);
object create_body(string name);
string parse_tokens(string text);
void greet();
void character_menu();
void character_menu_input(string str, string prompt);
void first_admin_login();

string name, character;

object body;

string login_message = parse_tokens(read_file(mudConfig("LOGIN_MSG")));

int call_out_id;
int gmcp_login_status = 0;
int greet_call;
private nosave int attempts = 0;
void dest_me() { call_out_walltime((:remove:), 0.2); }

void create() {
  set_log_level(0);

  if(clonep())
    call_out_id = call_out_walltime("auto_destruct", 60.0);

  set_notify_destruct(1);
}

void logon() {
  tell(this_object(), login_message);
  greet_call = call_out_walltime((:greet:), 0.2);
}

void greet(int gmcp_auth) {
  if(gmcp_login_status && find_call_out(gmcp_login_status) == -1)
    return;

  tell(this_object(),
    "\n"
    "You can login as your account or as character@account.\n"
    "Login as which account? ");
  input_to("get_account");
}

void gmcp_authenticated(string name, string char) {
  int reconnecting = 0;
  object old_body;

  if(old_body = find_player(char)) {
    write_file(log_dir() + LOG_LOGIN, capitalize(old_body->query_real_name()) + " ("+getoid(old_body)+") reconnected from " +
      query_ip_number(this_object()) + " on " + ctime(time()) + "\n");

    if(interactive(old_body)) {
      _info(old_body, "Your body has been displaced by another login.\n");
      remove_interactive(old_body);
    }

    body = old_body;
    enter_world(char, 1);
  } else {
    enter_world(char, 0);
  }
}

void get_account(string str) {
  int i;

  load_object(LOCKDOWN_D);

  if(LOCKDOWN_D->is_ip_banned(query_ip_number(this_object()))) {
    _error("\nYour IP address, " + query_ip_number(this_object()) + " has been banned from " + mud_name() + ".\n");
    return dest_me();
  }

  if(!str || strlen(str) < 2) {
    _question("You must select an account: ");
    input_to("get_account");
    return;
  }

  if(str == "quit") {
    _ok("Come back soon!");
    return dest_me();
  }

  if(sscanf(str, "%s@%s", character, name) != 2)
    name = str;

  if(LOCKDOWN_D->query_dev_lock() && wizardp(name) && !adminp(name)) {
    _error(LOCKDOWN_D->query_dev_lock_msg());
    return dest_me();
  }

  if(LOCKDOWN_D->query_player_lock() && (!adminp(name) && !wizardp(name))) {
    _error(LOCKDOWN_D->query_player_lock_msg());
    return dest_me();
  }

  if(LOCKDOWN_D->query_vip_lock() && (!adminp(name) && !wizardp(name) && (member_array(name, LOCKDOWN_D->query_play_testers()) == -1))) {
    _error(LOCKDOWN_D->query_vip_lock_msg());
    return dest_me();
  }

  // TODO: Figure out how to do guest logins in new account paradigm
#if 0
  if(str == "guest") {
    if(LOCKDOWN_D->query_guest_locked()) {
      _error(LOCKDOWN_D->query_guest_lock_msg());
      return dest_me();
    }

    user->set_name("guest");
    set_privs(user, "guest");
    body = create_body("guest");
    write_file(log_dir() + LOG_LOGIN, capitalize(user->query_real_name()) + " ("+getoid(body)+") logged in from " +
      query_ip_number(this_object()) + " on " + ctime(time()) + "\n");
    enter_world(0);
    return;
  }
#endif

  account = ACCOUNT_D->loadAccount(name);
  if(!account) {
    if(LOCKDOWN_D->query_player_lock()) {
      _error(LOCKDOWN_D->query_player_lock_msg());
      return dest_me();
    }

    _info("The account %s does not exist.", name);
    _question("Would you like to create it? ");
    input_to("new_account", name);
    return;
  }

  if(character && pointerp(account["characters"]) && member_array(character, account["characters"]) == -1) {
    character = null;
  }

  if(!account["password"]) {
    _info("Your account has no password. All accounts must have a password.\n");
    _question("Please enter a new password: ");
    input_to("get_password", 1, 2);
    return;
  }

  _question("Please enter your password: ");
  input_to("get_password", 1, 0);
}

void get_password(string str, int i) {
  string pass;
  string curr;

  if(!i) {
    curr = account["password"];
    pass = crypt(str, curr);
    if(pass != curr) {
      if(++attempts >= 3) {
        _error("Too many failed login attempts.");
        _ok("Goodbye.");
        return dest_me();
      }

      _error("That password is incorrect.");
      _question("Please enter your password: ");
      input_to("get_password", 1, i);
      return;
    } else {
      int char_index = character ? member_array(character, account["characters"]) : null;
      remove_call_out(call_out_id);
      call_out_id = null;

      if(!character)
        character_menu();
      else
        character_menu_input(sprintf("%d", char_index + 1), null);

      return;
    }
  } else {
    if(!str) {
      _error("Your account must have a password.");
      _question("Please enter a password: ");
      input_to("get_password", 1, i);
      return;
    }

    if(!validAccount(account["name"])) {
      if(!ACCOUNT_D->createAccount(account["name"], crypt(str, 0))) {
        _error("There was a problem creating your account.");
        return dest_me();
      }

      account = ACCOUNT_D->loadAccount(account["name"]);
    }

    _question("Please enter your password again to verify: ");
    input_to("verify_password", 1, i);
    return;
  }
}

void verify_password(string str, int i) {
  string curr;

  curr = account["password"];
  str = crypt(str, curr);
  if(str != curr) {
    ACCOUNT_D->removeAccount(account["name"]);
    _error("Your passwords do not match.");
    _question("Please enter your password: ");
    input_to("get_password", 1, i);
    return;
  } else {
    _ok("You may now login with the password you have just set.");
    _question("Please enter your password: ");
    input_to("get_password", 1, 0);
    return;
  }
}

void new_account(string str, string name) {
  if(str == "y" || str == "yes" || str == "yup" || str == "sure" || str == "indeed") {
    account = ([
      "name" : name,
    ]);

    _question("Please enter a password for your account: ");
    input_to("get_password", 1, 1);
    return;
  }

  _question("Please select another account name then: ");
  input_to("get_account");
}

void character_menu() {
  string *characters = account["characters"];
  int sz, i;
  string prompt = "\nCharacter select:\n\n";

  characters = map(characters, (: capitalize($1) :));
  sz = sizeof(characters);

  if(sizeof(characters) > 0) {
    prompt += "Select a character to connect to or create a new character.\n\n";

    while(++i <= sz)
      prompt += sprintf("%2d. %s\n", i, characters[i-1]);

    prompt += "\n";
  }

  prompt += " n. Create a new character\n";
  prompt += " q. Quit\n";
  prompt += "\n";
  prompt += "Selection: ";

  tell(this_object(), prompt);
  input_to("character_menu_input", prompt);
}

void character_menu_input(string str, string prompt) {
  int i;
  string *characters = account["characters"];
  int sz = sizeof(characters);

  if(str == "q") {
    _ok("Goodbye.");
    return dest_me();
  }

  if(str == "n") {
    _question("Enter the name of your new character: ");
    input_to("new_character");
    return;
  }

  if(!str || !sscanf(str, "%d", i) || i < 1 || i > sz) {
    _error("Invalid selection.");
    character_menu();
    input_to("character_menu_input", prompt);
    return;
  }

  i--;

  if(!characters[i]) {
    _error("Invalid selection.");
    character_menu();
    input_to("character_menu_input", prompt);
    return;
  }

  if(!user_exists(characters[i])) {
    _error("That character does not exist.");

    if(!prompt) {
      character_menu();
      return;
    }

    tell(this_object(), prompt);
    input_to("character_menu_input", prompt);
    return;
  }

  body = find_player(characters[i]);
  if(body) {
    if(interactive(body)) {
      _question("Would you like to reconnect to your old body? ");
      input_to("reconnect", characters[i]);
      return;
    } else {
      enter_world(characters[i], 1);
      return;
    }
  }

  body = create_body(characters[i]);

  if(!body)
    return;

  write_file(log_dir() + LOG_LOGIN, capitalize(characters[i]) + " ("+getoid(body)+") logged in from " +
    query_ip_number(this_object()) + " on " + ctime(time()) + "\n");

  enter_world(characters[i], 0);
}

void new_character(string str) {
  if(!str || strlen(str) < 3 || strlen(str) > 12) {
    _error("Character names must be between 3 and 12 characters.");
    _question("Enter the name of your new character: ");
    input_to("new_character");
    return;
  }

  if(pcre_match(str, "[^a-z]")) {
    _error("Character names must be lowercase letters only.");
    _question("Enter the name of your new character: ");
    input_to("new_character");
    return;
  }

  if(user_exists(str)) {
    _error("That character already exists.");
    _question("Enter the name of your new character: ");
    input_to("new_character");
    return;
  }

  if(!ACCOUNT_D->addCharacter(name, str)) {
    _error("There was a problem creating your character.");
    return dest_me();
  }

  body = create_body(str);

  if(!body)
    return;

  write_file(log_dir() + LOG_LOGIN, capitalize(str) + " ("+getoid(body)+") logged in from " +
    query_ip_number(this_object()) + " on " + ctime(time()) + "\n");

  assure_dir(str);

  body->save_body();

  first_admin_login();

  enter_world(str, 0);
}

void first_admin_login() {
  if(!file_exists(mudConfig("FIRST_USER"))) {
    object security_editor;
    string home_path = home_path(body->query_real_name());
    string privs = query_privs(body);

    assure_dir(home_path);
    assure_dir(home_path + "public");
    assure_dir(home_path + "private");
    catch(cp("/d/std/workroom.c", home_path(privs)));
    body->add_wizard_paths();
    security_editor = new(OBJ_SECURITY_EDITOR);
    security_editor->enable_membership(privs, "developer");
    security_editor->enable_membership(privs, "admin");
    security_editor->write_state(0);
    write_file(mudConfig("FIRST_USER"), privs, 1);
    _ok(this_object(), "You are now an admin.");
  }
}

void reconnect(string str, string name) {
  str = lower_case(str);

  if(str == "y" || str == "yes" || str == "yup" || str == "sure" || str == "indeed") {
    write_file(log_dir() + LOG_LOGIN, capitalize(body->query_real_name()) + " reconnected from " +
      query_ip_number(this_object()) + " on " + ctime(time()) + "\n");

    if(interactive(body)) {
      _info(body, "Your body has been displaced by another login.\n");
      remove_interactive(body);
    }

    body->reconnect();
    enter_world(name, 1);
    return;
  } else {
    body->remove();
    tell(this_object(), "You have chosen not to reconnect to your old body.\n");
    write_file(log_dir() + LOG_LOGIN, capitalize(str) + " ("+getoid(body)+") logged in from " +
      query_ip_number(this_object()) + " on " + ctime(time()) + "\n");
    enter_world(name, 0);
    return;
  }
}

void enter_world(string name, int reconnecting) {
  string loc;
  string e;
  object room;
  int result;

  if(!objectp(body))
    body = BODY_D->create_body(name);

  if(body->is_dead()) {
    body->remove();
    body = BODY_D->create_ghost(name);
  }

  exec(body, this_object());

  if(reconnecting)
    body->reconnect();

  body->setup_body(name);
  body->clear_gmcp_data();
  body->set_gmcp_client(login_gmcp_data["client"]);
  body->set_gmcp_supports(login_gmcp_data["supports"]);
  body->clear_environ_data();
  body->set_environ_data(__environ_data);

  // If they are not reconnecting, figure out where to put them
  if(!reconnecting) {
    if(devp(body)) {
      loc = body->query_env("start_location");
      if(loc == "last_location")
        loc = body->query_last_location();
      else
        loc = home_path(body) + "workroom";
    } else {
      loc = ROOM_START;
    }

    if(e = catch(room = load_object(loc))) {
      tell(body,"Unable to load your start location\n");
      tell(body,"Please contact an admin for assistance.\n");
    }

    if(e = catch(result = body->move_living(room, null, null, "SILENT"))) {
      tell(body,"Unable to move you to your start location.\n");
      tell(body,"Please contact an admin for assistance.\n");
    }

    if(result) {
      tell(body,"Unable to move you to your start location: " + MOVE_REASON[result] + "\n");
      tell(body,"Please contact an admin for assistance.\n");
    }

    if(devp(body) && !environment(body)) {
      // Try one more time to put them in the start location
      loc = ROOM_START;
      if(e = catch(room = load_object(loc))) {
        tell(body,"Unable to load your start location\n");
        tell(body,"Please contact an admin for assistance.\n");
      }

      if(e = catch(result = body->move_living(room, null, null, "SILENT"))) {
        tell(body,"Unable to move you to your start location.\n");
        tell(body,"Please contact an admin for assistance.\n");
      }

      if(result) {
        tell(body,"Unable to move you to your start location: " + MOVE_REASON[result] + "\n");
        tell(body,"Please contact an admin for assistance.\n");
      }
    }
  }

  if(!environment(body)) {
    body->remove();
    return dest_me();
  }

  body->enter_world(reconnecting);

  if(body->gmcp_enabled())
    GMCP_D->init_gmcp(body);

  if(reconnecting)
    emit(SIG_USER_LINK_RESTORE, body);
  else
    emit(SIG_USER_LOGIN, body);

  remove();
}

object create_body(string name) {
  string err;

  if(origin() != ORIGIN_LOCAL)
    return 0;

  err = catch(body = BODY_D->create_body(name));
  if(err || !body) {
    receive("\nThere was a problem creating your body.\n");
    debug(err);
    dest_me();
    return 0;
  }

  body->set_name(query_privs(body));
  set_privs(body, body->query_real_name());
  body->restore_body();

  return body;
}

string query_real_name() {
  return "login";
}

string parse_tokens(string text) {
  catch {
    text = replace_string(text, "%mud_name", mud_name());
    text = replace_string(text, "%users", implode(
      filter(users()->query_name(), (: $1 != "login" :))[0..<2], ", ") +
      ", and " + filter(users()->query_name(),
      (: $1 != "login" :))[<1]);
    text = replace_string(text, "%user_count", "" + sizeof(users()));
    text = replace_string(text, "%date", ctime(time()));
    text = replace_string(text, "%open_status", open_status());
    text = replace_string(text, "%email", admin_email());
    text = replace_string(text, "%lib_name", lib_name());
    text = replace_string(text, "%lib_version", lib_version());
    text = replace_string(text, "%baselib_version", baselib_version());
    text = replace_string(text, "%cap_mud_name",
      implode(explode(mud_name(), ""), (: capitalize($1)
      + capitalize($2) :)));
    text = replace_string(text, "%driver_version", driver_version());
  };

  return text;
}

void set_gmcp_client(mapping client) {
  login_gmcp_data["client"] = client;
}

mapping query_gmcp_client() {
  return login_gmcp_data["client"];
}

void set_gmcp_supports(mapping supports) {
  login_gmcp_data["supports"] = supports;

  if(!gmcp_login_status) {
    if(of("Char", supports) &&
       of("modules", supports["Char"]) &&
       of("Login", supports["Char"]["modules"])) {
      mapping payload = ([ "type" : ({ "password-credentials" }) ]);

      _info(this_object(), "\n["+mud_name()+"] GMCP Authentication available.");

      GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_LOGIN_DEFAULT, payload);

      gmcp_login_status = call_out_walltime((: greet :), 1.0);
    }
  }
}

mapping query_gmcp_supports() {
  return login_gmcp_data["supports"];
}

void receive_environ(string var, mixed value) {
  __environ_data[var] = value;
}

void auto_destruct() {
  if(!call_out_id)
    return;

  if(interactive(this_object()))
    tell(this_object(),
      "\n"
      "Sorry, but you are only allowed 60 seconds to attempt to login.\n");

  return dest_me();
}
