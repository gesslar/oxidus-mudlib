//goto.c

//Tacitus @ LPUniversity
//29-JUNE-05
//Wizard goto command
//   * Thanks to Icoz

//Last edited on July 22nd, 2006 by Parthenon

inherit STD_CMD;

mixed main(object tp, string arg) {
  string custom, tmp;
  object ob, target, env, my_env;
  string file;
  string e;
  int result;

  if(!arg) {
    return _info("Where would you like to go?");
  }

  my_env = environment(tp);

  if(target = find_player(arg)) {
    if((env = top_environment(target)) == my_env) {
      return _info("You are already with %s.", target->query_name());
    }

    if(tp->query_env("teleport_out") && wizardp(tp)) {
      custom = tp->query_env("teleport_out");

      tmp = custom;
      tmp = replace_string(tmp, "$N", tp->query_name());
      tmp = replace_string(tmp, "$D", target->query_name());
      tmp = append(tmp, "\n");
      tell_down(my_env, capitalize(tmp), 0, tp);
    } else {
      tell_down(my_env, tp->query_name() + " teleports to another location.\n", 0, tp);
    }

    result = tp->move_living(env, null, "SILENT", "SILENT");
    if(result) {
      return _error("Unable to move to %s (%d).", target->query_name(), result);
    }

    if(tp->query_env("teleport_in") && wizardp(tp)) {
      custom = tp->query_env("teleport_in");

      tmp = custom;
      tmp = replace_string(tmp, "$N", tp->query_name());
      tmp = replace_string(tmp, "$D", "here");
      tmp = append(tmp, "\n");
      tell_down(env, capitalize(tmp), 0, tp);
    } else {
      tell_down(env, tp->query_name() + " has teleported to this room.\n", 0, tp);
    }

    _ok(tp, "You have teleported to %s at %s", target->query_name(), env->query_short());

    return 1;
  } else {
    file = resolve_path(tp->query_env("cwd"), arg);
    e = catch(ob = load_object(file));
    if(e) {
      return _error("Unable to move to %s.", file);
    }

    if(my_env == ob) {
      return _info("You are already there.");
    }

    if(tp->query_env("teleport_out") && wizardp(tp)) {
      custom = tp->query_env("teleport_out");

      tmp = custom;
      tmp = replace_string(tmp, "$D", get_short(ob));
      tmp = replace_string(tmp, "$N", tp->query_name());

      tmp = append(tmp, "\n");
      tell_down(my_env, capitalize(tmp), 0, tp);
    } else {
      tell_down(my_env, tp->query_name() + " teleports to another location.\n", 0, tp);
    }

    result = tp->move_living(ob, null, "SILENT", "SILENT");
    if(result) {
      return _error("Unable to move to %s (%d).", file, result);
    }

    if(tp->query_env("teleport_in") && wizardp()) {
      custom = tp->query_env("teleport_in");

      tmp = custom;
      tmp = replace_string(tmp, "$D", "here");
      tmp = replace_string(tmp, "$N", tp->query_name());

      tmp = append(tmp, "\n");
      tell_down(ob, capitalize(tmp), 0, tp);
    } else {
      tell_down(ob, tp->query_name() + " has teleported to this room.\n", 0, tp);
    }

    _ok(tp, "You have teleported to %s",  capitalize(get_short(ob)));

    return 1;
  }

  return _error("Invalid argument.");
}

string help(object tp) {
  return
"SYNTAX: goto <room/user>\n\n"
"The goto command allows you to transport to another room by filename or "
"transport to another user. If the user or room specified is not found, you "
"will not be moved.\n";
}