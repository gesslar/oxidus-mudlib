/**
 * @file /adm/daemons/bank.c
 * @description Bank daemon
 *
 * @created 2024-02-28 - Gesslar
 * @last_modified 2024-02-28 - Gesslar
 *
 * @history
 * 2024-02-28 - Gesslar - Created
 */

inherit STD_DAEMON;

// Forward declarations
void setup();
mixed new_account(string name);
mixed query_balance(string name);
mixed add_balance(string name, int amount);
varargs mixed query_activity(string name, int limit);

private nosave string db = "bank";
// These statements are in sprintf format
private nosave mapping balance_statements = ([]);
private nosave mapping activity_statements = ([]);

void setup() {
  balance_statements = ([
    "select" : "SELECT amount FROM balance WHERE name = '%s' ;",
    "update" : "UPDATE balance SET amount = %d, time = %d WHERE name = '%s' ;",
    "add" : "INSERT INTO balance (name, amount, time) VALUES ('%s', %d, %d) ;",
  ]);
  activity_statements = ([
    "select": "SELECT * FROM activity WHERE name = '%s' ORDER BY time DESC ;",
    "select_limited": "SELECT * FROM activity WHERE name = '%s' ORDER BY time DESC LIMIT %d ;",
    "add" : "INSERT INTO activity (time, name, amount) VALUES (%d, '%s', %d);",
  ]);
}

/**
 * @daemon_function new_account
 * @description Creates a new bank account for the given name.
 * @param {string} name - The name of the account holder.
 * @returns {mixed} - 1 if the account was created successfully, an error message
 *                    if the account already exists or if there was a database error.
 */
mixed new_account(string name) {
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
 * @daemon_function query_balance
 * @description Retrieves the current balance for the given account name.
 * @param {string} name - The name of the account holder.
 * @returns {mixed} - The current balance as an integer if the account exists,
 *                    null if the account doesn't exist, or an error message
 *                    if there was a database error.
 */
mixed query_balance(string name) {
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
 * @daemon_function add_balance
 * @description Adds (or subtracts) the specified amount from the given account.
 * @param {string} name - The name of the account holder.
 * @param {int} amount - The amount to add (positive) or subtract (negative).
 * @returns {mixed} - A success message if the transaction was completed,
 *                    an error message if the account doesn't exist,
 *                    there are insufficient funds, or if there was a database error.
 */
mixed add_balance(string name, int amount) {
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

  printf("result %O\n", result);

  if(stringp(result))
    return "Database error: " + result;

  return 1;
}

/**
 * @daemon_function query_activity
 * @description Retrieves the recent activity for the given account.
 * @param {string} name - The name of the account holder.
 * @param {int} [limit=10] - The maximum number of recent activities to retrieve.
 * @returns {mixed} - An array of recent activities if successful,
 *                    null if there are no activities,
 *                    or an error message if there was a database error.
 */
varargs mixed query_activity(string name, int limit: (: 10 :)) {
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
