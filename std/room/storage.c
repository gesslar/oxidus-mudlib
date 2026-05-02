/**
 * @file /std/room/storage.c
 * @description This is a room that enables players to access storage. Inherit
 *              from this class to create a storage room.
 *
 * @created 2024-08-12 - Gesslar
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * 2024-08-12 - Gesslar - Created
 * 2025-03-16 - GitHub Copilot - Added documentation
 */

#include <classes.h>
#include "/std/object/include/command.h"
#include "/std/object/include/init.h"
#include "/std/object/include/object.h"

inherit CLASS_STORAGE;

private nomask void destructing();
private nomask mixed cmd_list(object tp, string arg);
private nomask mixed cmd_store(object tp, string arg);
private nomask mixed cmd_take(object tp, string arg);

private class StorageOptions storageOptions;

/**
 * Initialises a storage room with commands and cleanup handler.
 *
 * This adds the 'list', 'store', and 'take' commands to the room
 * and sets up object cleanup on destruction.
 */
void initStorageRoom() {
  addCommand("list", cmd_list);
  addCommand("store", cmd_store);
  addCommand("take", cmd_take);

  addDestruct(destructing);
}

/**
 * Creates or retrieves the storage object for this room.
 *
 * Creates or finds the appropriate storage object based on configuration.
 * For "public" storage, uses /storage/[organization].
 * For "private" storage, uses /storage/[organization]/[id].
 * For custom storage, loads /obj/storage/[organization].
 *
 * @returns {STD_STORAGE_OBJECT} The storage object
 * @errors If storage organization is invalid
 * @errors If storage ID for private storage is invalid
 * @errors If custom storage file is not found
 */
object store() {
  if(!stringp(storageOptions.storage_org))
    error("Invalid storage organization specified");

  string org = storageOptions.storage_org;
  string file;

  /** @type {STD_STORAGE_OBJECT} */ object storageObject;

  switch(storageOptions.storage_type) {
    case "public": {
      file = sprintf("/storage/%s", org);
      break;
    }

    case "private": {
      mixed id = storageOptions.storage_id;
      function f = valid_function(id) ? id : null;

      id = f ? f() : id;

      if(!stringp(id))
        error("Invalid storage ID specified for private storage");

      file = sprintf("/storage/%s/%s", org, id);
      break;
    }

    default: {
      // Default to custom storage
      file = sprintf("/obj/storage/%s", org);

      if(file_size(file + ".c") < 0) {
        error("Custom storage file not found: " + file + ".c");
      }

      storageObject = new(file);
      storageObject->set_storage_options(storageOptions);
      return storageObject;
    }
  }

  if(storageObject = find_object(file))
    return storageObject;

  storageObject = load_object(file);
  storageObject->set_storage_options(storageOptions);
  storageObject->set_link(file_name());

  return storageObject;
}

/**
 * Sets storage options for this room.
 *
 * @param {class StorageOptions} options - The storage configuration options
 */
void set_storage_options(class StorageOptions options) {
  storageOptions = options;
}

/**
 * Returns the current storage options.
 *
 * @returns {class StorageOptions} The storage configuration options
 */
class StorageOptions query_storage_options() {
  return storageOptions;
}

/**
 * Command to list items in storage.
 *
 * @param {object} _tp - The player issuing the command
 * @param {string} _arg - Command arguments
 * @returns {string*} Array of strings describing the storage contents
 */
mixed cmd_list(object _tp, string _arg) {
  string *list = ({});
  object ob, next;
  object storageObject = store();

  ob = first_inventory(storageObject);

  while(ob) {
    next = next_inventory(ob);
    list += ({ get_short(ob) });
    ob = next;
  }

  if(sizeof(list) > 0)
    list = ({ "You see the following items in storage", "" }) + list;
  else
    list = ({ "You see no items in storage." });

  return list;
}

/**
 * Command to store items from the player's inventory into storage.
 *
 * Handles "store all" and "store all [item]" syntax for batch storing.
 *
 * @param {object} tp - The player issuing the command
 * @param {string} arg - Command arguments
 * @returns {string} Result message
 */
mixed cmd_store(object tp, string arg) {
  /** @type {STD_ITEM} */ object ob;
  /** @type {STD_ITEM*} */ object *obs;
  string out = "";
  int result;
  object storageObject = store();

  if(!arg)
    return "Usage: store <item|all|all <item>>";

  if(arg == "all")
    obs = find_targets(tp, null, tp);
  else if(sscanf(arg, "all %s", arg))
    obs = find_targets(tp, arg, tp);
  else
    obs = ({ find_target(tp, arg, tp) });

  obs -= ({ 0 });

  if(!sizeof(obs))
    return "You don't possess any such thing to store.";

  foreach(ob in obs) {
    if(result = ob->move(storageObject))
      out += get_short(ob) + " could not be stored.\n";
    else
      out += "You store " + get_short(ob) + ".\n";
  }

  if(strlen(out)) {
    if(storageOptions.restore_on_load)
      storageObject->save_contents();

      return out;
  } else {
    return "You were unable to store anything.";
  }
}

/**
 * Command to take items from storage into player's inventory.
 *
 * Handles "take all" and "take all [item]" syntax for batch retrieval.
 *
 * @param {object} tp - The player issuing the command
 * @param {string} arg - Command arguments
 * @returns {string} Result message
 */
mixed cmd_take(object tp, string arg) {
  /** @type {STD_ITEM} */ object ob;
  /** @type {STD_ITEM*} */ object *obs;
  string out = "";
  int result;
  object storageObject = store();

  if(!arg)
    return "Usage: take <item|all|all <item>>";

  if(arg == "all")
    obs = all_inventory(storageObject);
  else if(sscanf(arg, "all %s", arg))
    obs = filter(all_inventory(storageObject), (: $1->id($2) :), arg);
  else
    obs = ({ present(arg, storageObject) });

  obs -= ({ 0 });

  if(!sizeof(obs))
    return "There is no such item in storage.";

  foreach(ob in obs) {
    if((result = ob->move(tp)))
      out += get_short(ob) + " could not be taken.\n";
    else
      out += "You take " + get_short(ob) + " from storage.\n";
  }

  if(strlen(out)) {
    if(storageOptions.restore_on_load)
      storageObject->save_contents();

    return out;
  } else {
    return "You were unable to take anything from storage.";
  }
}

/**
 * Cleanup function called when the room is being destroyed.
 *
 * Finds and removes any storage objects linked to this room.
 */
void destructing() {
  object *storageObjects = clones(STD_STORAGE_OBJECT);

  storageObjects = filter(storageObjects, (: interactive :));

  filter(storageObjects, (: $1->remove() :));
}
