/**
 * @file /adm/daemons/bank.c
 *
 * Bank daemon responsible for managing player bank accounts,
 * including creation, balance queries, deposits/withdrawals,
 * and transaction activity history.
 *
 * @created 2024-02-28 - Gesslar
 * @last_modified 2024-02-28 - Gesslar
 *
 * @history
 * 2024-02-28 - Gesslar - Created
 */

inherit STD_DAEMON;

// Forward declarations
public mixed new_account(string name);
public mixed query_balance(string name);
public mixed add_balance(string name, int amount);
public varargs mixed query_activity(string name, int limit);

/**
 * Creates a new bank account for the given name.
 *
 * @param {string} name - The name of the account holder
 * @returns {mixed} 1 if the account was created successfully,
 *                  or an error string if the account already
 *                  exists or there was a database error
 */
public mixed new_account(string name) {
  mixed result;

  name = capitalize(lower_case(name));
  result = query_balance(name);

  if(!nullp(result))
    return "Account already exists.";

  result = DB_D->rest("POST", "db://bank/balance", ([
    "name": name,
    "time": time(),
    "amount": 0,
  ]));

  if(stringp(result))
    return result;

  result = DB_D->rest("POST", "db://bank/activity", ([
    "name": name,
    "time": time(),
    "amount": 0,
  ]));

  if(stringp(result))
    return result;

  return 1;
}

/**
 * Retrieves the current balance for the given account name.
 *
 * @param {string} name - The name of the account holder
 * @returns {mixed} The current balance as an integer if the
 *                  account exists, null if the account doesn't
 *                  exist, or an error string on database failure
 */
public mixed query_balance(string name) {
  mixed result;

  name = capitalize(lower_case(name));
  result = DB_D->rest("GET", sprintf("db://bank/balance?name=%s", name));

  if(stringp(result))
    return result;

  if(sizeof(result) == 0)
    return null;

  return result[0]["amount"];
}

/**
 * Adds or subtracts the specified amount from the given
 * account.
 *
 * @param {string} name - The name of the account holder
 * @param {int} amount - The amount to add (positive) or
 *                       subtract (negative)
 * @returns {mixed} 1 on success, or an error string if the
 *                  account doesn't exist, there are
 *                  insufficient funds, or on database failure
 */
public mixed add_balance(string name, int amount) {
  mixed result, current_balance;
  int new_balance;

  name = capitalize(lower_case(name));
  current_balance = query_balance(name);

  if(stringp(current_balance))
    return current_balance;

  if(nullp(current_balance))
    return "Account does not exist.";

  new_balance = current_balance + amount;
  if(new_balance < 0)
    return "Insufficient funds.";

  result = DB_D->rest("PUT", "db://bank/balance?name="+name, ([
    "time": time(),
    "amount": new_balance,
  ]));

  if(stringp(result))
    return "Database error: " + result;

  result = DB_D->rest("POST", "db://bank/activity", ([
    "name": name,
    "time": time(),
    "amount": amount,
  ]));

  if(stringp(result))
    return "Database error: " + result;

  return 1;
}

/**
 * Retrieves the recent activity for the given account.
 *
 * @param {string} name - The name of the account holder
 * @param {int} [limit=10] - The maximum number of recent
 *                           activities to retrieve
 * @returns {mixed} An array of recent activities if
 *                  successful, null if there are no
 *                  activities, or an error string on
 *                  database failure
 */
public varargs mixed query_activity(string name,
  int limit: (: 10 :)) {
  mixed result;

  name = capitalize(lower_case(name));

  if(limit > 0)
    result = DB_D->rest("GET", sprintf("db://bank/activity?name=%s&_order=time:desc&_limit=%d", name, limit));
  else
    result = DB_D->rest("GET", sprintf("db://bank/activity?name=%s&_order=time:desc", name));

  if(stringp(result))
    return result;

  if(sizeof(result) == 0)
    return null;

  return result;
}
