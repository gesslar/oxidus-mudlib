#include <simul_efun.h>

/* resolve_path.c

 Kenon @ LPUniversity
 06-Oct-06
 Resolve path simul-efun

*/

/* Last updated by Tacitus on October 6th, 2006 */

/**
 * Resolves a given path relative to the current path, handling
 * special cases such as user directories and relative paths.
 *
 * @param {string} base_dir - The current path.
 * @param {string} path - The next path to resolve.
 * @returns {string} The resolved absolute path.
 */
string resolve_path(string base_dir, string path) {
  int index;
  int temp;
  string* path_segments;

  if(path == "here")
    return file_name(environment(this_body())) + ".c";

  if(path[0] == '~') {
    if((path[1] == '/') || (sizeof(path) == 1))
      path = home_path(/** @type {STD_PLAYER} */ (this_body())->query_real_name()) + path[1..];
    else {
      index = strsrch(path, "/");
      if(index == -1) return home_path(path[1..]);
      else path = home_path(path[1..index-1]) + path[index..];
    }
  }

  if(path[0] != '/') path = base_dir + "/" + path;
  path_segments = explode(path, "/");

  for(index = 0; index < sizeof(path_segments); index++) {
    if(path_segments[index] == ".")
      path_segments[index] = "";

    if(path_segments[index] == "..") {
      path_segments[index] = "";
      temp = index - 1;

      while((temp >= 0) && (!sizeof(path_segments[temp])))
        temp--;

        if(temp != -1) path_segments[temp] = "";
      }
  }

  path = "/" + implode(path_segments, "/");
  while(pcre_match(path, "//"))
    path = replace_string(path, "//", "/");

  return path;
}

/**
 * Resolves and validates a path, checking if it exists as either
 * a file or directory.
 *
 * @param {string} base_dir - The base directory to resolve relative paths from.
 * @param {string} path - The path to resolve and validate.
 * @returns {string|int} The resolved absolute path if valid, or 0 if invalid.
 */
string valid_path(string base_dir, string path) {
  string resolved = resolve_path(base_dir, path);

  return (file_exists(resolved) || directory_exists(resolved)) ? resolved : 0;
}

/**
 * Resolves and validates a file path, checking if it exists as a
 * file.
 *
 * @param {string} base_dir - The base directory to resolve relative paths from.
 * @param {string} path - The file path to resolve and validate.
 * @returns {string|int} The resolved absolute file path if valid, or 0 if invalid.
 */
string valid_file(string base_dir, string path) {
  string resolved = resolve_path(base_dir, path);

  return file_exists(resolved) ? resolved : 0;
}

/**
 * Resolves and validates a directory path, checking if it exists
 * as a directory.
 *
 * @param {string} base_dir - The base directory to resolve relative paths from.
 * @param {string} path - The directory path to resolve and validate.
 * @returns {string|int} The resolved absolute directory path if valid, or 0 if invalid.
 */
string valid_dir(string base_dir, string path) {
  string resolved = resolve_path(base_dir, path);

  return directory_exists(resolved) ? resolved : 0;
}

/**
 * Resolves a file path without checking its existence.
 *
 * @param {string} base_dir - The base directory to resolve relative paths from.
 * @param {string} path - The file path to resolve.
 * @returns {string} The resolved absolute file path.
 */
string resolve_file(string base_dir, string path) {
  return resolve_path(base_dir, path);
}

/**
 * Resolves a directory path without checking its existence,
 * ensuring it ends with a slash.
 *
 * @param {string} base_dir - The base directory to resolve relative paths from.
 * @param {string} path - The directory path to resolve.
 * @returns {string} The resolved absolute directory path, ending with a slash.
 */
string resolve_dir(string base_dir, string path) {
  string resolved = resolve_path(base_dir, path);

  return (resolved[<1] == '/') ? resolved : resolved + "/";
}

/**
 * Resolves a path and returns an array of matching files, supporting * wildcard pattern.
 *
 * @param {string} base_dir - The base directory to resolve relative paths from.
 * @param {string} path - The path or pattern to resolve and search for files.
 * @returns {string*} An array of matching file paths, or ({}) if invalid.
 */
string *get_files(string base_dir, string path) {
  string resolved_path = resolve_path(base_dir, path);
  string *parts, *files;
  string dir, pattern;

  // Use valid_dir_file to split the path and check if it's valid
  parts = valid_dir_file(resolved_path);
  if(!parts)
    return ({});

  dir = parts[0];
  pattern = parts[1];

  // If pattern doesn't contain "*", append it
  if(strsrch(pattern, "*") == -1)
    pattern += "*";

  // Use get_dir() with the pattern
  files = get_dir(dir + pattern);
  files -= ({ ".", ".." });

  // Construct full paths for the results
  if(files && sizeof(files) > 0)
    files = map(files, (: $(dir) + $1 :));

  files = map(files, (: directory_exists($1) ? append($1, "/") : $1 :));

  return files;
}
