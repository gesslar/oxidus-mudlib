/**
 * @file /adm/daemons/bank.c
 *
 * Bank daemon responsible for managing player bank accounts,
 * including creation, balance queries, deposits/withdrawals,
 * and transaction activity history.
 *
 * @created 2024-02-28 - Gesslar
 * @last_modified 2026-05-02 - Gesslar
 *
 * @history
 * 2026-05-02 - Gesslar - Drop dead DB-error string checks; DB_D
 *                        no longer returns strings on failure
 * 2026-03-30 - Gesslar - Refactored to use DB_D REST interface
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
 * @returns {mixed} 1 if the account was created, or
 *                  "Account already exists." if it does
 */
public mixed new_account(string name) {
  name = capitalize(lower_case(name));

  if(!nullp(query_balance(name)))
    return "Account already exists.";

  DB_D->rest("POST", "db://bank/balance", ([
    "name": name,
    "time": time(),
    "amount": 0,
  ]));

  DB_D->rest("POST", "db://bank/activity", ([
    "name": name,
    "time": time(),
    "amount": 0,
  ]));

  return 1;
}

/**
 * Retrieves the current balance for the given account name.
 *
 * @param {string} name - The name of the account holder
 * @returns {mixed} The current balance as an integer if the
 *                  account exists, otherwise null
 */
public mixed query_balance(string name) {
  mixed result;

  name = capitalize(lower_case(name));
  result = DB_D->rest("GET", sprintf("db://bank/balance?name=%s", name));

  if(!pointerp(result))
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
 * @returns {mixed} 1 on success, "Account does not exist." if
 *                  the account is missing, or "Insufficient
 *                  funds." if the new balance would be negative
 */
public mixed add_balance(string name, int amount) {
  mixed current_balance;
  int new_balance;

  name = capitalize(lower_case(name));
  current_balance = query_balance(name);

  if(nullp(current_balance))
    return "Account does not exist.";

  new_balance = current_balance + amount;
  if(new_balance < 0)
    return "Insufficient funds.";

  DB_D->rest("PUT", "db://bank/balance?name="+name, ([
    "time": time(),
    "amount": new_balance,
  ]));

  DB_D->rest("POST", "db://bank/activity", ([
    "name": name,
    "time": time(),
    "amount": amount,
  ]));

  return 1;
}

/**
 * Retrieves the recent activity for the given account.
 *
 * @param {string} name - The name of the account holder
 * @param {int} [limit=10] - The maximum number of recent
 *                           activities to retrieve
 * @returns {mixed} An array of recent activities, or null if
 *                  there are none
 */
public varargs mixed query_activity(string name,
  int limit: (: 10 :)) {
  mixed result;

  name = capitalize(lower_case(name));

  if(limit > 0)
    result = DB_D->rest("GET", sprintf("db://bank/activity?name=%s&_order=time:desc&_limit=%d", name, limit));
  else
    result = DB_D->rest("GET", sprintf("db://bank/activity?name=%s&_order=time:desc", name));

  if(!pointerp(result))
    return null;

  return result;
}
