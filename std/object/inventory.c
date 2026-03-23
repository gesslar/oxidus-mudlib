/**
 * @file /std/object/inventory.c
 * @description Inventory inheritable for objects that can hold other objects.
 *              Provides functionality for automated object spawning and tracking.
 *
 * @created 2024-07-23 - Gesslar
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * 2024-07-23 - Gesslar - Created
 * 2025-03-16 - GitHub Copilot - Added documentation
 */

// Functions from other objects
void set_spawn_info(mapping info);

// Elements have the format of: ([ "object" : string, "number" : int|function, "args" : array ])
private nosave mapping inventory;

/**
 * Adds or clones an object to the inventory of this object.
 *
 * If the file parameter is an object, it will be moved to this object.
 * If it's a string, a new object will be cloned and moved to this object.
 *
 * @param {object|string} file - Object to add or path of file to clone
 * @param {mixed} args... - Optional arguments to pass to the object
 * @returns {object|null} The object that was added, or null/0 if failed
 */
object add_inventory(mixed file, mixed args...) {
  object ob;
  string e;
  int result;
  int new_ob;

  if(nullp(file))
    return null;

  e = catch {
    if(objectp(file)) {
      if(environment(file) == this_object())
        return file;

      result = file->move(this_object());

      if(result)
        return 0;

      return file;
    }

    if(!stringp(file))
      return 0;

    new_ob = 1;

    if(sizeof(args))
      ob = new(file, args...);
    else
      ob = new(file);
  };

  if(e) {
    if(ob) {
      if(new_ob) {
        ob->remove();
      }
    }

    log_file("OBJECT", "%s %s add_inventory clone error: %s\n  file: %s\n",
      ctime(), file_name(), e, stringp(file) ? file : file_name(file));
  }

  if(!ob)
    return 0;

  ob->set_spawn_info(([
    "created" : time(),
    "spawn_point" : file_name(),
    "spawn_trace" : call_trace(),
  ]));

  e = catch(ob->move(this_object()));
  if(e) {
    log_file("OBJECT", "%s %s add_inventory move error: %s\n  ob: %s\n",
      ctime(), file_name(), e, file_name(ob));
    ob->remove();
    return 0;
  }

  if(environment(ob) != this_object()) {
    ob->remove();
    return 0;
  }

  return ob;
}

/**
 * Registers an object to be automatically spawned during reset.
 *
 * @param {string} arg[0] - Object path to clone
 * @param {int|function} [arg[1]=1] - Number of objects to create or function that returns count
 * @param {mixed} [arg[2..]] - Optional arguments to pass to the object constructor
 * @returns {string|void} UUID of the registered object, or void if invalid
 */
string add_object(mixed arg...) {
  mapping element;
  string uuid;
  int sz;

  if(!inventory)
    inventory = ([]);

  // Count the number of arguments
  sz = sizeof(arg);

  if(sz == 0)
    return;

  // Double-check we don't already have this object in our inventory mapping
  foreach(element in values(inventory))
    if(element["object"] == arg[0])
      return;

  element = ([]);
  element["object"] = arg[0];

  if(sz >= 2)
    element["number"] = arg[1];
  else
    element["number"] = 1;

  if(sz >= 3)
    element["args"] = arg[2..];
  else
    element["args"] = ({});

  uuid = generate_uuid();
  inventory[uuid] = element;

  return uuid;
}

/**
 * Removes an object definition from the inventory mapping.
 *
 * @param {string} uuid - UUID of the object to remove
 */
void remove_object(string uuid) {
  if(!inventory) return;
  if(!uuid) return;
  if(!inventory[uuid]) return;

  map_delete(inventory, uuid);
}

/**
 * Spawns missing inventory objects during room reset.
 *
 * Checks if the required number of each registered object exists in inventory,
 * and spawns any missing objects. Newly spawned objects receive spawn information
 * and an object_spawned event is triggered.
 */
void reset_objects() {
  string uuid;
  int sz, i;
  int result;
  mapping element;
  object ob;
  string e;

  if(!inventory)
    return;

  foreach(uuid, element in inventory) {
    object *clones = present_clones(element["object"], this_object());
    int num_clones = sizeof(clones);
    int diff;

    if(num_clones < element["number"]) {
      diff = element["number"] - num_clones;

      while(diff--) {
        ob = add_inventory(element["object"], element["args"]);
        if(ob) {
          ob->add_spawn_info("belongs_to", file_name());
          ob->add_spawn_info("object_uuid", uuid);
          ob->add_id(uuid);
          event(this_object(), "object_spawned", ob);
        }
      }
    }
  }
}

/**
 * Returns all registered inventory object definitions.
 *
 * @returns {mapping} Copy of the inventory object mapping
 */
mapping query_all_objects() {
  return copy(inventory);
}
