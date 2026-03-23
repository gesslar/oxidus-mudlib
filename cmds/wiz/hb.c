// /cmds/wiz/hb.c
// Determine all heartbeats or the heartbeat of a specific object
//
// Created:     2024/02/18: Gesslar
// Last Change: 2024/02/18: Gesslar
//
// 2024/02/18: Gesslar - Created

inherit STD_CMD;

mixed all_heartbeats(object tp);

mixed main(object tp, string arg) {
  object ob;
  int i;
  int hb_ms;

  if(!arg) {
    return "Usage: hb <object|all>";
  }

  if(arg == "all") {
    return all_heartbeats(tp);
  }

  if(!ob = get_object(arg)) {
    string file = resolve_path("cwd", arg);
    file = append(file, ".c");

    if(file_exists(file)) {
      ob = load_object(file);
    }
  }

  if(!ob) {
    return "No such object: " + arg;
  }

  i = query_heart_beat(ob);
  hb_ms = get_config(__RC_HEARTBEAT_INTERVAL_MSEC__);

  arg = ob->query_name();

  if(i) {
    return "Heartbeat is on for " + arg + " (" + i + "x" + hb_ms + "ms = " + (i * hb_ms) + "ms)";
  } else {
    return "Heartbeat is off for " + arg + ".";
  }
}

mixed all_heartbeats(object tp) {
  object *obs;
  int i;
  int hb_ms;
  string *ret;

  obs = heart_beats();
  ret = ({ });
  hb_ms = get_config(__RC_HEARTBEAT_INTERVAL_MSEC__);

  for(i = 0; i < sizeof(obs); i++) {
    int interval = query_heart_beat(obs[i]);
    if(query_heart_beat(obs[i])) {
      ret += ({ obs[i]->query_name() + " (" + interval + "x" + hb_ms + "ms = " + (interval * hb_ms) + "ms)" });
    }
  }

  if(sizeof(ret)) {
    return "All Heartbeats:\n\t" + implode(ret, "\n\t");
  } else {
    return "No heartbeats.";
  }
}
