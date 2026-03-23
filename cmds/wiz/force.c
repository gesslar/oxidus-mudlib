/* force.c

 Tacitus @ LPUniversity
 05-JULY-06
 Developer Force Command

*/

inherit STD_CMD;

mixed main(object tp, string args) {
  string target, cmd;
  object ob, *obs, room;
  int admin = adminp(tp);

  if(!objectp(room = environment(tp))) {
    return 0;
  }

  // Supported syntax
  // non-admins
  // force <living> to <cmd> - non-interactives
  // force all to <cmd> - all present non-interactives
  // force all here to <cmd> - all present non-interactives
  // admins
  // force <living> to <cmd> - interactive or non-interactive
  // force all users to <cmd> - all users
  // force all npcs here to <cmd> - all npcs present
  // force all users here to <cmd> - all users present
  // force all to <cmd> - all present non-interactive
  // force all here to <cmd> - all present non-interactive

  if(!stringp(args) || sscanf(args, "%s to %s", target, cmd) != 2) {
    return _error("Syntax: force <living> to <cmd>");
  }

  if(sscanf(args, "all %s here to %s", target, cmd) == 2) {
    if(target == "users") {
      if(admin) {
        obs = present_players(room);
      } else {
        return _error("You may not force interactives.");
      }
    } else if(target == "npcs") {
      obs = present_livings(room);
      obs = filter(obs, (: !interactive($1) :));
    } else {
      return _error("Unsupported argument to force.");
    }
  } else if(sscanf(args, "all to %s", cmd) == 1) {
    if(admin) {
      obs = users();
    } else {
      obs = present_livings(room);
      obs = filter(obs, (: !interactive($1) :));
    }
  } else if(sscanf(args, "all here to %s", target, cmd) == 1) {
    if(admin) {
      obs = all_inventory(room);
    } else {
      obs = all_inventory(room);
      obs = filter_array(obs, (: !interactive($1) :));
    }
  } else if(sscanf(args, "all %s to %s", target, cmd) == 2) {
    if(target == "users") {
      if(admin) {
        obs = users();
      } else {
        return _error("You may not force interactives.");
      }
    } else {
      return _error("Unsupported argument to force.");
    }
  } else if(sscanf(args, "%s to %s", target, cmd) == 2) {
    target = lower_case(target);
    if(admin) {
      ob = present(target, room);
      if(!ob) {
        ob = find_player(target);
      }
    } else {
      ob = present(target, room);
      if(!ob) {
        return _error("1 %s not found.", target);
      }

      if(interactive(ob)) {
        return _error("You may not force interactive objects.");
      }
    }

    if(!ob) {
      return _error("%s not found.", target);
    }

    if(!living(ob)) {
      return _error("%s is not a living object.", target);
    }

    obs = ({ ob });
  } else {
    return _error("Syntax: force <living> to <cmd>");
  }

  obs = filter_array(obs, (: living($1) && !devp($1) :));

  if(!sizeof(obs)) {
    return _info("No valid targets found.");
  }

  foreach(ob in obs) {
    int result = ob->force_me(cmd);
    if(result == false) {
      _info(tp, "Unable to force %s to %s", ob->query_name(), cmd);
    } else {
      _ok(tp, "Forced %s to %s", ob->query_name(), cmd);
    }
  }

  return 1;
}

string query_help(object tp) {
  return
"SYNTAX: force <living> <cmd>\n\n"
"This command allows you to force a living object to execute a command as if "
"they had typed it themself. This means that the command must be in their "
"path or added by an add_action. Normal developers may not force interactive "
"objects (ie. users).";
}
