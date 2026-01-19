/**
 * @file /adm/simul_efun/exists.c
 *
 * Simul-efuns for checking existence of files, directories, and user data.
 * Provides consistent interfaces for validating filesystem resources.
 *
 * @created 2005-04-02 - Tacitus
 * @last_modified 2024-03-11
 *
 * @history
 * 2005-06-30 - Gwegster - Added user_exists()
 * 2005-04-02 - Tacitus - Created
 */

#include <simul_efun.h>

/**
 * Checks if a directory exists in the filesystem.
 *
 * @param {string} dirname - Path of directory to check
 * @returns {int} 1 if directory exists, 0 otherwise
 * @example
 * if(directory_exists("/d/wizards/" + name)) {
 *     tell_me("Wizard directory exists.\n");
 * }
 */
int directory_exists(string dirname) {
  return file_size(dirname) == -2;
}

/**
 * Checks if a regular file exists in the filesystem.
 *
 * @param {string} file - Path of file to check
 * @returns {int} 1 if file exists and is readable, 0 otherwise
 * @example
 * if(file_exists(user_file)) {
 *     restore_object(user_file);
 * }
 */
int file_exists(string file) {
  return file_size(file) >= 0;
}

/**
 * Checks if a LPC source file exists.
 *
 * Automatically appends ".c" to the filename before checking.
 *
 * @param {string} file - Base path of LPC file without .c extension
 * @returns {int} 1 if source file exists, 0 otherwise
 * @example
 * if(cfile_exists("/cmds/wiz/update")) {
 *     compile_object("/cmds/wiz/update");
 * }
 */
int cfile_exists(string file) {
  return file_exists(append(file, ".c"));
}

/**
 * Checks if a save file exists.
 *
 * Automatically appends the mud's save extension before checking.
 *
 * @param {string} file - Base path of save file without extension
 * @returns {int} 1 if save file exists, 0 otherwise
 * @example
 * if(ofile_exists("/save/players/" + name)) {
 *     restore_object("/save/players/" + name);
 * }
 */
int ofile_exists(string file) {
  return file_exists(append(file, __SAVE_EXTENSION__));
}

/**
 * Checks if a user's data file exists.
 *
 * Looks for user save file in the standard user directory structure.
 *
 * @param {string} user - Username to check
 * @returns {int} 1 if user data exists, 0 otherwise
 * @example
 * if(user_exists("alice")) {
 *     tell_me("User account exists.\n");
 * }
 */
int user_exists(string user) {
  return stringp(ACCOUNT_D->characterAccount(user)) ||
      file_exists(
      "/data/users/" + user[0..0] + "/" + user + "/" + user + __SAVE_EXTENSION__
     )
  ;
}
