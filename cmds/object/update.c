/**
 * @file /cmds/object/update.c
 * Command for updating (reloading) objects, with optional
 * recursive inheritance updating.
 *
 * @created 2005-04-05 - Byre@LPUniversity
 * @last_modified 2024-02-04 - Gesslar
 *
 * @history
 * 2005-04-05 - Byre@LPUniversity - Created
 * 2006-07-11 - Tacitus - Edited
 * 2024-02-04 - Gesslar - Added recursion to the update command
 */

inherit STD_CMD;

private void do_update(string file);
private string *collect_inherits(object obj, int depth);
private string *deep_collect_inherits(
  object obj, mapping seen, int depth
);

public mixed main(
  /** @type {STD_PLAYER} */ object caller, string arg
) {
  /** @type {STD_ROOM|STD_OBJECT} */ object obj;
  object room;
  string *parts, start, file;
  string *interactives;
  string e;
  int depth = 0;
  int is_room_obj;

  if(!objectp(room = environment(caller)))
    return "You are not in a valid environment.";

  if(arg) {
    if(arg == "-r" || arg == "-R") {
      if(arg == "-r")
        depth = 1;
      else if(arg == "-R")
        depth = 2;
    } else if(sizeof(parts = pcre_extract(
      arg, "^-([rR]) (.*)$"
    )) == 2) {
      if(parts[0] == "r")
        depth = 1;
      else
        depth = 2;

      file = parts[1];
    } else {
      depth = 0;
      file = arg;
    }
  }

  if(!file) {
    if(!caller->query_env("cwf"))
      return _error(
        "You must provide an argument. "
        "Syntax: update [-rR] [<file>]"
      );
    file = caller->query_env("cwf");
  } else {
    obj = get_object(file);

    if(obj) {
      if(clonep(obj))
        file = base_name(obj);
      else
        file = file_name(obj);
    }
  }

  file = resolve_path(caller->query_env("cwd"), file);
  file = append(file, ".c");

  if(file == append(file_name(), ".c"))
    return _error(
      "You cannot update the update command. "
      "Destruct it instead."
    );

  if(file[0..<3] == ROOM_VOID)
    return _error("You cannot update the void object.");

  start = file;

  if(!obj)
    obj = find_object(file);

  is_room_obj = objectp(obj) && obj->is_room();

  if(objectp(obj)) {
    if(clonep(obj))
      _info(caller, "Updating master of %s", file_name(obj));
    else
      _info(caller, "Updating %s", file);

    if(virtualp(obj)) {
      file = obj->query_virtual_master();
      file = append(file, ".c");
    }

    if(is_room_obj) {
      interactives = filter_array(
        all_inventory(obj), (: interactive :)
      );
      interactives->move(load_object(ROOM_VOID), 1);
      obj->remove();
      if(obj)
        destruct(obj);
    }
  } else {
    _info(caller, "Updating %s", file);
  }

  caller->set_env("cwf", start);

  e = catch(obj = load_object(file));
  if(e)
    return _error("Failed to load: %s\n%s", file, e);

  if(!objectp(obj))
    return _error("Failed to load: %s", file);

  string *files = collect_inherits(obj, depth);
  files = map(files, (: append($1, ".c") :));
  filter(files, (: do_update :));

  obj = load_object(start);
  if(is_room_obj && pointerp(interactives))
    interactives->move(obj, 1);

  return 1;
}

private void do_update(string file) {
  object obj;
  string err;

  if(obj = find_object(file)) {
    obj->remove();
    if(obj)
      destruct(obj);
  }

  if(!file_exists(file)) {
    _error("%s does not exist.", file);
    return;
  }

  err = catch(obj = load_object(file));

  if(err) {
    _error("Failed to load: %s\n%s", file, err);
    return;
  }

  _ok("%s was updated.", file);
}

private string *collect_inherits(object obj, int depth) {
  string fname = file_name(obj);
  mapping seen = ([ fname: 1 ]);
  string *files;

  if(depth == 0)
    return ({ fname });

  files = deep_collect_inherits(obj, seen, depth);
  if(!sizeof(files) || files[<1] != fname)
    files += ({ fname });

  return files;
}

private string *deep_collect_inherits(
  object obj, mapping seen, int depth
) {
  string *files = ({});

  if(depth > 0) {
    foreach(string file in inherit_list(obj)) {
      if(!seen[file]) {
        object inherit_obj =
          find_object(file) || load_object(file);
        seen[file] = 1;

        if(depth == 2 && inherit_obj)
          files += deep_collect_inherits(
            inherit_obj, seen, depth
          );

        files += ({ file });
      }
    }
  }

  return files;
}

string query_help(object _caller) {
  return
"SYNTAX: update [-rR] [<file|here>]\n\n"
"If file is not specified, the current working file will "
"be updated. If the file is specified, it will be updated. "
"If the -r or -R flag is specified, the file will be "
"updated along with all of its inherited files. The -r "
"flag will only update the first level of inheritance, "
"while the -R flag will update all levels of "
"inheritance.\n\n"
"Examples:\n"
"update - Update the cwf.\n"
"update here - Update the current room.\n"
"update -r here - Update the current room and its "
"inherit_list().\n"
"update -R here - Update the current room and its "
"deep_inherit_list().\n"
"update /cmds/std/who - Update the who command.\n"
"update -r /cmds/std/who - Update the who command and "
"its inherit_list().\n"
"update -R /cmds/std/who - Update the who command and "
"its deep_inherit_list().\n"
"update -r - Update the cwf and its inherit_list().\n"
"update -R - Update the cwf and its "
"deep_inherit_list().\n";
}
