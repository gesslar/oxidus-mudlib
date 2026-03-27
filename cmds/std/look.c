/**
 * @file /cmds/std/look.c
 *
 * Look command for viewing rooms, objects, and livings.
 *
 * @created 2006-10-06 - Tacitus @ LPUniversity
 * @last_modified 2006-10-06 - Tacitus
 *
 * @history
 * 2006-10-06 - Tacitus - Created (Rewrite #3)
 */

inherit STD_ACT;

private mixed render_room(object tp, object room, int brief);
private mixed render_object(object tp, object room, string target);
private mixed render_container(object tp, object room, string target);
private mixed render_living(object tp, object room, object target, int brief);
private string highlight_view(object tp, string str, string *keys);

private nosave string default_highlight_colour = mudConfig("LOOK_HIGHLIGHT_COLOUR");
private nosave string look_highlight_enabled = mudConfig("LOOK_HIGHLIGHT");

mixed main(/** @type {STD_PLAYER} */ object tp, string arguments) {
  string target;
  object room = environment(tp);

  if(!stringp(arguments))
    return render_room(tp, room, 0);
  if(sscanf(arguments, "at %s", target))
    return render_object(tp, room, lower_case(target));
  if(sscanf(arguments, "in %s", target))
    return render_container(tp, room, lower_case(target));
  else
    return render_object(tp, room, arguments);
}

/**
 * Highlights specific keywords in a given string based on user preferences.
 *
 * This function will highlight the specified keywords in the provided string
 * using a colour determined by the user's preferences or a default value.
 *
 * @param {object} tp - The player object
 * @param {string} str - The string to be processed
 * @param {string*} keys - The list of keywords to highlight
 * @returns {string} The processed string with highlighted keywords
 */
private string highlight_view(object tp, string str, string *keys) {
  int i;
  string colour;

  if(tp->query_pref("highlight") != "on")
    return str;

  if(sizeof(keys) <= 0)
    return str;

  colour = tp->query_pref("highlight_colour") || default_highlight_colour;
  colour = "{{" + colour + "}}";

  // need to determine if number is from 0-256 with a leading 0
  if(!pcre_match(colour, TRUE_COLOUR_REGEX))
    colour = default_highlight_colour;

  // colour = COLOUR_D->substitute_too_dark(colour);

  for(i = 0; i < sizeof(keys); i++) {
    str = replace_string(str , " " + keys[i] + " ", " " + colour+keys[i] + "{{res}} ");
    str = replace_string(str , " " + capitalize(keys[i]) + " ", " " + colour+capitalize(keys[i]) + "{{res}} ");
    str = replace_string(str , " " + keys[i] + ",", " " + colour+keys[i] + "{{res}},");
    str = replace_string(str , " " + keys[i] + ".", " " + colour+keys[i] + "{{res}}.");
    str = replace_string(str , " " + keys[i] + "!", " " + colour+keys[i] + "{{res}}!");
    str = replace_string(str , " " + keys[i] + ";", " " + colour+keys[i] + "{{res}};");
    str = replace_string(str , " " + keys[i] + "'", " " + colour+keys[i] + "{{res}}'");
    str = replace_string(str , " " + keys[i] + ":", " " + colour+keys[i] + "{{res}}:");
  }

  return str;
}

private mixed render_room(object tp, object room, int brief) {
  string *exits, *doors;
  object *users, *objects;
  string result = "";
  mixed data, datum;

  if(!objectp(room))
    return "You see nothing because you have no environment!\n";

  if(devp(tp) && of(tp->query_pref("look_filename"), ({ "on", "all" }))) {
    result += "{{0066CC}}"+room+"{{res}}";
    if(virtualp(room))
      result += " {{3366CC}}(vm: " + room->query_virtual_master() + "){{res}}";

    result += "\n";
  }

  data = get_short(room, 1);
  if(stringp(data) && strlen(data))
    result += data + "\n";

  data = get_long(room, 1);
  if(stringp(data) && strlen(data))
    result += "\n" + highlight_view(tp, data, keys(room->query_items())) + "\n";

  exits = keys(room->query_exits());
  doors = keys(room->query_doors());

  if(sizeof(exits) > 0) {
/*
    exits = map(exits, function(string exit, object room, string *doors) {
      if(of(exit, doors)) {
        if(room->is_door_open(exit))
          return sprintf("(%s)", exit);
        else
          return sprintf("[%s]", exit);
      }
      return exit;
    }, room, doors);
*/
    data = "You may go " + simple_list(exits, "or") + ".\n";
  } else {
    data = "There are no exits here.\n";
  }
  if(data)
    result += "\n" + data;

  data = null;
  if(sizeof(doors) > 0) {
    data = "";
    foreach(string door in doors) {
      if(room->is_door_open(door))
        data += sprintf("The %s door is open.\n", door);
      else
        data += sprintf("The %s door is closed.\n", door);
    }
  }
  if(data)
    result += data;

  users = find_targets(tp, null, room, (: living($1) && $1 != $(tp) :));
  objects = find_targets(tp, null, room, (: !living($1) :));

  if(sizeof(users) > 0) {
    if(devp(tp))
      data = implode(map(users, (: get_short($1, 1) + " (" + file_name($1) + ")" :)), "\n");
    else
      data = implode(map(users, (: get_short($1, 1) :)), "\n");
    result += data + "\n";
  }

  if(sizeof(objects) > 0) {
    if(devp(tp))
      data = implode(map(objects, (: get_short($1, 1) + " (" + file_name($1) + ")" :)), "\n");
    else
      data = implode(map(objects, (: get_short($1, 1) :)), "\n");
    result += data + "\n";
  }

  tell(tp, result);
  return 1;
}

private mixed render_object(object tp, object room, string target) {
  object ob;
  string name = tp->query_name();
  string desc = "";
  string temp;
  int here_flag = 0;

  // Check if the target includes the "here" argument
  if(sscanf(target, "%s here", target))
    here_flag = 1;

  // Check for a room item first
  if(stringp(room->query_item(target))) {
    string result = highlight_view(tp, room->query_item(target), keys(room->query_items()));
    result = append(result, "\n");
    tell(tp, result);
    tell_direct(room, name + " looks at " + target + ".\n", null, ({ tp }));
    return 1;
  }

  // Determine the object to look at
  if(target == "me") {
    ob = tp;
  } else {
    if(here_flag) {
      // Look for the object in the room first if "here" is specified
      ob = find_target(tp, target, room);
      if(!ob)
        return "You do not see " + target + " here.\n";
    } else {
      // Look for the object in the player's inventory first
      ob = find_target(tp, target, tp);
      if(!ob) {
        // If not found in the inventory, look in the room
        ob = find_target(tp, target, room);
        if(!ob)
          return "You do not see " + target + ".\n";
      }
    }
  }

  // Render living objects
  if(living(ob))
    return render_living(tp, room, ob, 0);

  // Render non-living objects
  temp = get_short(ob, 1);
  if(stringp(temp))
    desc += temp + "\n";
  temp = get_long(ob, 1);
  if(stringp(temp))
    desc += "\n" + temp + "\n";

  if(strlen(desc))
    if(devp(tp) && tp->query_pref("look_filename") == "all")
      desc = "{{0066CC}}" + prepend(file_name(ob), "/") + "{{res}}\n" + desc;

  tell(ob, name + " looks at you.");
  tell_down(room, name + " looks at " + get_short(ob) + ".\n", null, ({ tp, ob }));

  tell(tp, desc);

  return 1;
}


private mixed render_living(object tp, object room, object target, int brief) {
  string temp, result = "";
  string name;
  mapping equipment, wielded;
  object ob;
  string slot;
  string race, gender, hair, eyes;

  temp = get_short(target, 1);
  if(stringp(temp))
    result += temp + "\n";
  temp = get_long(target, 1);
  if(stringp(temp))
    if(strlen(result) && strlen(temp))
      temp += "\n";
  result += temp;

  race = target->query_race();
  gender = target->query_gender();
  hair = target->query_hair_string();
  eyes = target->query_eyes_string();

  if(race) {
    if(gender) {
      if(gender == "male" || gender == "female")
        temp = " a "+gender+" " + race;
      else
        temp = " "+add_article(race);
    } else {
      temp = " "+add_article(race);
    }
  } else {
    if(gender)
      temp = gender;
    else
      temp = "";
  }

  if(hair && eyes && hair != "no hair" && eyes != "no eyes") {
    if(strlen(temp))
      temp += ", with " + hair + " and " + eyes;
    else
      temp = hair + " and " + eyes;
  } else if(hair && hair != "no hair") {
    if(strlen(temp))
      temp += ", with " + hair;
    else
      temp = hair;
  } else if(eyes && eyes != "no eyes") {
    if(strlen(temp))
      temp += ", with " + eyes;
    else
      temp = eyes;
  }

  if(strlen(temp)) {
    if(tp == target)
      temp = "You are" + temp;
    else
      temp = capitalize(subjective(target)) + " is" + temp;
  }

  if(strlen(temp))
    temp = append(temp, ".");

  if(strlen(temp) && strlen(result))
    result += temp + "\n";

  name = tp->query_name();
  if(target == tp) {
    string refl = reflexive(tp);
    tell_down(room, name + " looks at "+refl+".\n", null, ({ tp }));
  } else {
    string vname = target->query_name();
    tell(target, name + " looks at you.\n");
    tell_down(room, name + " looks at " + vname + ".\n", null, ({ tp, target }));
  }

  if(strlen(result))
    if(devp(tp) && tp->query_pref("look_filename") == "all")
      result = "{{0066CC}}"+prepend(file_name(target), "/") + "{{res}}\n" + result;

  wielded = target->query_wielded();
  equipment = target->query_equipped();
  if(sizeof(equipment) || sizeof(wielded)) {
    string *slots = keys(wielded) + keys(equipment);
    int max = max(map(slots, (: strlen :)));

    result += "\n";
    foreach(slot in slots) {
      ob = wielded[slot] || equipment[slot];
      if(!ob)
        continue;

      result += sprintf("%*s : %s\n", max, capitalize(slot), get_short(ob, 1));
    }
  }

  tell(tp, result);

  return 1;
}

private mixed render_container(object tp, object room, string arg) {
  object ob;
  string target;
  int here_flag;
  string desc = "";

  if(sscanf(arg, "%s here", target) == 1)
    here_flag = 1;
  else
    target = arg;

  if(here_flag) {
    if(!ob = find_target(tp, target))
      return "You do not see that here.";
  } else {
    if(!ob = find_target(tp, target, tp))
      if(!ob = find_target(tp, target))
        return "You do not see " + target + " anywhere.";
  }

  if(ob->is_locked())
    desc += ob->query_short()+" is locked.\n";
  else if(ob->is_closed())
    desc += ob->query_short()+" is closed.\n";

  if(!ob->is_closed() || (ob->is_closed() && !ob->is_opaque())) {
    object *contents = find_targets(tp, null, ob);

    if(sizeof(contents) > 0) {
      desc += ob->query_short()+" contains:\n";
      desc += implode(map(contents, (: get_short($1, 1) :)), "\n") + "\n";
    } else {
      desc += ob->query_short()+" is empty.\n";
    }
  }

  if(present(ob, tp))
    tp->simple_action("$N $vlook inside $p $o.", ob);
  else
    tp->simple_action("$N $vlook inside a $o.", ob);

  return desc;
}

string query_help(object _caller) {
  return(
    "SYNTAX: look [<in/at>] <object> [<on/in> <object>]\n\n"
    "This command will allow you to look at objects in your environment. "
    "If no argument is supplied, it will show you the whole room. You can "
    "also look at specific objects by typing 'look <object>' or 'look at object'. "
    "You might also try looking into an object by typing 'look in <object>'.");
}
