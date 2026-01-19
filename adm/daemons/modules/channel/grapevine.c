/**
 * @file /adm/daemons/modules/channel/grapevine.c
 * @description Grapevine chat channels module
 *
 * @created 2024-07-18 - Gesslar
 * @last_modified 2024-07-18 - Gesslar
 *
 * @history
 * 2024-07-18 - Gesslar - Created
 */

inherit STD_DAEMON;

string *locals = mudConfig("GRAPEVINE")["local_only"];
string *ch_list = mudConfig("GRAPEVINE")["channels"] + locals;

mapping history = ([]);
private nosave string module_name = query_file_name(this_object());

void setup() {
  set_no_clean(1);
  set_persistent();
}

void post_setup_1() {
  int i;

  CHAN_D->register_module(module_name, file_name());

  for(i = 0; i < sizeof(ch_list); i++) {
    CHAN_D->register_channel(module_name, ch_list[i]);

    if(history[ch_list[i]] == 0)
      history[ch_list[i]] = ({ });
  }

  saveDatda();
}

int rec_msg(string chan, string usr, string msg) {
  object ob;
  string real_message;
  int num_lines;

  if(sscanf(msg, "/last %d", num_lines) == 1) {
    if(num_lines > sizeof(history[chan]))
      num_lines = sizeof(history[chan]);

    ob = find_player(usr);

    if(!sizeof(history[chan])) {
      tell(ob, "LocalNet: Channel " + chan + " has no history yet.\n");
    } else {
      foreach(string hist_line in history[chan][(sizeof(history[chan]) - num_lines)..(sizeof(history[chan]) - 1)]) {
        tell(ob, hist_line);
      }

      return 1;
    }
  } else {
    switch(msg) { /* We could do some neat stuff here! */
      case "/last": {
        ob = find_player(usr);

        if(!sizeof(history[chan])) {
          tell(ob, "LocalNet: Channel " + chan + " has no history yet.\n");
        } else {
          foreach(string hist_line in history[chan][(sizeof(history[chan]) - 15)..(sizeof(history[chan]) - 1)]) {
            tell(ob, hist_line);
          }

          return 1;
        }
        break;
      }

      case "/all" : {
        ob = find_player(usr);

        if(!sizeof(history[chan])) {
          tell(ob, "LocalNet: Channel " + chan + " has no history yet.\n");
        } else {
          foreach(string hist_line in history[chan][(sizeof(history[chan]) - 50)..(sizeof(history[chan]) - 1)]) {
            tell(ob, hist_line);
          }

          return 1;
        }
        break;
      }
    }
  }

  if(msg[0..0] == ":") {
    msg = msg[1..<1];
    real_message = sprintf(" %s", msg);
  } else {
    real_message = sprintf(": %s", msg);
  }

  CHAN_D->rec_msg(chan, lower_case(usr), "[" + capitalize(chan) + "] " + capitalize(usr) + real_message + "\n");
  history[chan] += ({ ldate(time(),1) +" "+ltime() + " [" + capitalize(chan) + "] " + capitalize(usr) + real_message + "\n" });

  if(!of(chan, locals))
    GRAPEVINE_D->grapevine_broadcast_message(chan, usr, msg);

  saveDatda();

  return 1;
}

int rec_grapevine_msg(string chan, string usr, string msg, string game) {
  string real_message;
  string name;

  sscanf(usr, "%s (%s in I3)", usr, game);

  if(msg[0..0] == ":") {
    msg = msg[1..<1];
    real_message = sprintf(" %s", msg);
  } else {
    real_message = sprintf(": %s", msg);
  }

  name = sprintf("%s@%s", usr, game);

  CHAN_D->rec_msg(chan, lower_case(name), "[" + capitalize(chan) + "] " + name + real_message + "\n");
  history[chan] += ({ ldate(time(),1) +" "+ltime() + " [" + capitalize(chan) + "] " + name + real_message + "\n" });

  saveDatda();

  return 1;
}

int is_allowed(string channel, string usr, int flag: (: 0 :)) {
  return 1;
}
