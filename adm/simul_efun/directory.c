/**
 * @file /adm/simul_efun/directory.c
 *
 * Directory manipulation and query simul-efuns. Provides secure methods
 * for creating directory hierarchies and retrieving directory paths.
 *
 * @created 1992-09-05 - Truilkan
 * @last_modified 2024-03-11
 *
 * @history
 * 1992-09-05 - Truilkan - Created
 */

#include <simul_efun.h>

/**
 * Creates a directory and all necessary parent directories.
 *
 * Temporarily assumes privileges of the calling object while creating
 * the directory structure to ensure proper permissions.
 *
 * @param {string} path - Full directory path to create
 * @returns {int} 1 if directory exists/created, 0 otherwise
 * @example
 * if(assure_dir("/d/players/" + name + "/letters")) {
 *     tell_me("Created mail directory.\n");
 * }
 */
mixed assure_dir(string path) {
  string *parts, dir;
  int j;

  string old_privs;

  if(file_size(path) == -2)
    return 1;

  old_privs = query_privs();
  set_privs(this_object(), query_privs(previous_object()));

  parts = explode(path, "/");
  dir = "";
  for(j = 0; j < sizeof(parts); j++) {
    dir += parts[j];
    mkdir(dir);
    dir += "/";
  }

  set_privs(this_object(), old_privs);

  return file_size(path) == -2;
}

/**
 * Gets the directory portion of an object's file path.
 *
 * Returns a directory path with leading and trailing slashes. Uses
 * previous_object() if no object is provided.
 *
 * @param {object} [ob] - Object to get directory for
 * @returns {string} Directory path with both leading and trailing slash
 * @errors If no valid object is available
 * @example
 * string dir = query_directory(sword);  // Returns "/obj/weapon/"
 */
varargs string query_directory(object ob) {
  string file, *parts;
  string dir;

  if(!objectp(ob))
    ob = previous_object();

  if(!objectp(ob))
    error("Bad argument 1 to query_directory()");

  file = base_name(ob);
  parts = explode(file, "/");
  parts = parts[0..<2];
  dir = implode(parts, "/");

  return append(prepend(dir, "/"), "/");
}
