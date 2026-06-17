/**
 * @file /adm/daemons/account.c
 *
 * Account daemon responsible for managing player accounts, including creation,
 * modification, and character associations.
 *
 * @created 2024-08-09 - Gesslar
 * @last_modified 2024-08-09 - Gesslar
 *
 * @history
 * 2024-08-09 - Gesslar - Created
 */

#ifdef MOO
#define aroo
#endif

#include "include/account.h"

inherit STD_DAEMON;

// Forward declarations
public int create_account(string name, string password);
public mapping load_account(string name);
public string write_account(string name, string key, mixed data);
public mixed read_account(string name, string key);
public int valid_manip(string name);
public int remove_account(string name);
public int add_character(string account_name, string str);
public int remove_character(string account_name, string str);
public string character_account(string str);
public string *account_characters(string account_name);

/** @type {AccountRecord} */ private nomask mapping accounts = ([ ]);
private nomask mapping reverse = ([ ]);

/**
 * Configures the daemon to prevent automatic cleanup and sets persistence.
 */
void setup() {
  set_no_clean(1);
  set_persistent(1);
}

/**
 * Creates a new account with the given name and password.
 *
 * The function performs several validation checks before creating the account:
 * - Verifies the caller has valid manipulation permissions
 * - Validates name and password are proper strings
 * - Ensures account doesn't already exist
 * - Confirms password is properly encrypted (starts with $6$)
 *
 * @param {AccountName} name - The account name to create
 * @param {AccountPassword} password - The encrypted password (must start with $6$)
 * @returns {Boolean} 1 on success, 0 on failure
 * @example
 * int success = create_account("testuser", "$6$randomsalt$hashedpassword");
 * if(success) {
 *     write("Account created successfully!\n");
 * }
 */
public int create_account(string name, string password) {
  if(!valid_manip(name))
    return false;

  if(!name || !stringp(name))
    return false;

  if(!password || !stringp(password))
    return false;

  if(valid_account(name))
    return false;

  if(password[0..2] != "$6$")
    return false;

  name = lower_case(name);

  mapping account = ([
    "password" : password,
    "characters" : ({})
  ]);

  accounts[name] = account;

  save_data();

  return true;
}

/**
 * Loads an account's data from storage.
 *
 * If the account is not in memory, it will be loaded from disk and cached.
 * The disk file will be removed after loading.
 *
 * @param {AccountName} name - The account name to load
 * @returns {AccountRecord} The account data if found, null otherwise
 */
public mapping load_account(string name) {
  if(!valid_manip(name))
    return null;

  if(!name || !stringp(name))
    return null;

  if(!accounts[name]) {
    string file = account_file(name);

    if(!file_exists(file))
      return null;

    accounts[name] = from_string(read_file(file));
    reverse[file] = name;

    rm(file);
  }

  return accounts[name];
}

/**
 * Updates a specific key-value pair in an account.
 *
 * @param {AccountName} name - The account name to modify
 * @param {string} key - The key to update
 * @param {mixed} data - The new value to store
 * @returns {mixed|undefined} The stored value on success, 0 on failure
 */
string write_account(string name, string key, mixed data) {
  if(!valid_manip(name))
    return false;

  if(!name || !stringp(name))
    return false;

  if(!key || !stringp(key))
    return false;

  if(!data)
    return false;

  mapping account = load_account(name);
  if(!account)
    return false;

  account[key] = data;

  write_file(account_file(name), pretty_map(account));

  save_data();

  return account[key];
}

/**
 * Retrieves a specific value from an account.
 *
 * @param {AccountName} name - The account name to read from
 * @param {string} key - The key to retrieve
 * @returns {mixed} The stored value if found, 0 otherwise
 */
mixed read_account(string name, string key) {
  if(!valid_manip(name))
    return false;

  if(!name || !stringp(name))
    return false;

  if(!key || !stringp(key))
    return false;

  mapping account = load_account(name);
  if(!account)
    return false;

  return account[key];
}

/**
 * Validates if the caller has permission to manipulate an account.
 *
 * Permissions are granted if:
 * - The caller has admin privileges
 * - The caller's privs match the account name
 * - The caller is the GMCP Char module
 *
 * @param {AccountName} name - The account name to check permissions for
 * @returns {Boolean} 1 if manipulation is allowed, 0 otherwise
 */
int valid_manip(string name) {
  object prev = previous_object();
  object caller = this_caller();

  if(!prev && !caller)
    return false;

  if(adminp(caller))
    return true;

  if(!has_role(query_privs(prev), "admin")
      && query_privs(prev) != name
      && base_name(previous_object()) != H_GMCP_CHAR
      && (caller && query_privs(caller)) != name
  ) {
    return false;
  }

  return true;
}

/**
 * Removes an account and all its associated data.
 *
 * This removes both the account data and any character associations.
 *
 * @param {AccountName} name - The account name to remove
 * @returns {Boolean} 1 on success, 0 on failure
 */
int remove_account(string name) {
  if(!valid_manip(name))
    return false;

  if(!name || !stringp(name))
    return false;

  if(!valid_account(name))
    return false;

  map_delete(accounts, name);
  foreach(string key, string value in reverse) {
    if(value == name)
      map_delete(reverse, key);
  }

  save_data();

  return true;
}

/**
 * Associates a character with an account.
 *
 * @param {AccountName} account_name - The account to add the character to
 * @param {CharacterName} str - The character name to add
 * @returns {1|null} 1 on success, null on failure
 */
int add_character(string account_name, string str) {
  if(!account_name || !stringp(account_name))
    return null;

  if(!valid_manip(account_name))
    return null;

  if(!accounts[account_name])
    return null;

  if(!str || !stringp(str))
    return null;

  str = lower_case(str);

  mapping account = load_account(account_name);
  if(!account)
    return false;

  /** @type {CharacterName*} */ string *characters = account["characters"] || ({});
  characters += ({ str });
  account["characters"] = distinct_array(characters);
  accounts[account_name] = account;
  reverse[str] = account_name;

  assure_dir(user_data_directory(str));

  save_data();

  return true;
}

/**
 * Removes a character from an account.
 *
 * @param {AccountName} account_name - The account to remove the character from
 * @param {CharacterName} character_name - The character name to remove
 * @returns {int} 1 on success, null on failure
 */
int remove_character(string account_name, string character_name) {
  if(!account_name || !stringp(account_name))
    return null;

  if(!valid_manip(account_name))
    return null;

  if(!accounts[account_name])
    return null;

  if(!character_name || !stringp(character_name))
      return null;

  character_name = lower_case(character_name);

  mapping account = load_account(account_name);
  if(!account)
    return false;

  string *characters = account["characters"] || ({});
  characters -= ({ character_name });
  account["characters"] = characters;
  accounts[account_name] = account;

  map_delete(reverse, character_name);

  save_data();

  return true;
}

/**
 * Retrieves the account name associated with a character.
 *
 * @param {CharacterName} character_name - The character name to look up
 * @returns {AccountName} The associated account name if found, null otherwise
 */
string character_account(string character_name) {
  if(!character_name || !stringp(character_name))
    return null;

  if(!reverse[character_name])
    return null;

  return reverse[character_name];
}

/**
 * Retrieves all characters associated with an account.
 *
 * @param {AccountName} account_name - The account name to look up
 * @returns {CharacterName*} Array of character names if found, null otherwise
*/
string* account_characters(string account_name) {
  if(!account_name || !stringp(account_name))
    return null;

  if(!valid_manip(account_name))
    return null;

  if(!accounts[account_name])
    return null;

  mapping account = load_account(account_name);
  if(!account)
    return false;

  return account["characters"] || ({});
}

/**
 * @typedef {0|1} Boolean
 * 1 for true, 0 for false.
 */

 /**
  * @typedef {string} AccountName
  * An account name.
  */

/**
 * @typedef {string} AccountPassword
 * An account password.
 */


 /**
  * @typedef {string} CharacterName
  * A character name.
  */

 /**
  * An account record.
  * @typedef {mapping} AccountRecord
  * @property {string*} character_names - A list of character names for this account.
  * @property {string} password - The password for this account.
  */
