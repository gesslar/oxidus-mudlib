/**
 * @file /std/room/items.c
 * Manages item descriptions within rooms for examination.
 *
 * @created 2024-09-13 - Gesslar
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * 2024-09-13 - Gesslar - Created
 * 2025-03-16 - GitHub Copilot - Added documentation
 */

private nosave mapping _items = ([]);

mapping set_items(mapping item_data);
mapping remove_item(string id);
mapping add_item(mixed id, string desc);
mapping query_items();
string query_item(string id);

/**
 * Sets up items that can be examined in the room.
 *
 * This replaces all existing items with the new set.
 *
 * @param {mapping} item_data - Mapping of item IDs to descriptions
 * @returns {mapping} The updated items mapping (copy)
 */
mapping set_items(mapping item_data) {
  mixed id;
  string key;
  mapping item = ([]);

  foreach(id, key in item_data) {
    if(pointerp(id)) {
      int i;
      for(i = 0; i < sizeof(id); i++)
          item += ([id[i] : key]);
    } else {
      item += ([id : key]);
    }
  }

  _items = item;

  return query_items();
}

/**
 * Returns a copy of all items.
 *
 * @returns {mapping} A copy of the items mapping
 */
mapping query_items() {
  return copy(_items);
}

/**
 * Returns the description for a specific item ID.
 *
 * The description can be:
 * - A simple string
 * - A function pointer that will be called with the ID
 * - A string starting with @@ to call a function in this object
 *
 * @param {string} id - The item ID to look up
 * @returns {string} The item description, or 0 if not found
 */
string query_item(string id) {
  mixed item, value;
  string result;

  if(!nullp(item))
    return 0;

  foreach(item, value in _items) {
    if(pointerp(item)) {
      int i;

      for(; i < sizeof(item); i++) {
        if(item[i] == id) {
          result = value;
          break;
        }
      }
    } else {
      if(item == id) {
        result = value;
        break;
      }
    }
  }

  if(valid_function(result)) {
    function f = (function)result;

    result = f(id);
  }

  if(stringp(result) && result[0..1] == "@@")
    return call_other(this_object(), result[2..], id);

  if(stringp(result))
    return result;

  return 0;
}

/**
 * Removes an item by ID.
 *
 * @param {string} id - The item ID to remove
 * @returns {mapping} The updated items mapping (copy)
 */
mapping remove_item(string id) {
  if(!_items[id])
    return query_items();

  map_delete(_items, id);

  return query_items();
}

/**
 * Adds a new item description to the room.
 *
 * @param {string|string*} id - ID or array of IDs for the item
 * @param {string} desc - Description shown when examining the item
 * @returns {mapping} The updated items mapping (copy)
 */
mapping add_item(mixed id, string desc) {
  if(pointerp(id)) {
    int i;

    for(; i < sizeof(id); i++) {
      if(_items[id[i]])
        map_delete(_items, id);

      _items += ([id[i] : desc]);
    }
  } else {
    if(_items[id])
      map_delete(_items, id);

    _items += ([id : desc]);
  }

  return query_items();
}
