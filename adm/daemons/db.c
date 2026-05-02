/**
 * @file /adm/daemons/db.c
 *
 * Database daemon providing SQLite3 database management and query
 * execution. Handles database creation, table management, and
 * provides synchronous and chunked (lazy) query interfaces.
 *
 * @created 2024-02-27 - Gesslar
 * @last_modified 2026-05-02 - Gesslar
 *
 * @history
 * 2026-05-02 - Gesslar - Quote identifiers, close fd on error/zero-row,
 *                        skip empty .tbl files
 * 2026-03-30 - Gesslar - Added REST-style query interface
 * 2024-02-27 - Gesslar - Created
 */

inherit STD_DAEMON;
inherit EXT_HTTP;

#ifndef __PACKAGE_DB__
#else
#ifdef __USE_SQLITE3__

// Forward declarations
public int validDb(string db);
public int validTable(string db, string table);
public mixed query(string db, string q, mixed *callback);
public int allowUpsert(string db);
public mixed sqliteVersion(string db);
public string statementFromMapping(mapping data);
private mapping *collateData(mixed *result);
private void executeQuery(string db, string q, int offset, string queryId, mixed *callback);
public mapping queryDatabases();
public mapping queryTables(string dbName);
public void lazyQuery(string db, string q, mixed *callback);
public varargs mixed rest(string method, string url, mapping data, mixed *callback);
private mapping parseDbUrl(string url);
private string escapeValue(mixed v);
private string escapeIdent(string s);
private string whereFromMapping(mapping m);
private string setFromMapping(mapping m);

private nosave mapping __handle = ([]);
private nosave mapping __databases = ([]);
private nosave mapping __tableDefinitions = ([]);
private nosave int __dbChunkSize = mudConfig("DB_CHUNK_SIZE");

public void setup() {
  string dbPath = mudConfig("DB_PATH");
  string dbSuffix = mudConfig("DB_SUFFIX");
  string tableSuffix = mudConfig("DB_TABLE_SUFFIX");

  __databases = ([]);
  __tableDefinitions = ([]);

  if(strlen(dbPath) > 0) {
    // Find all table definition files first
    string *tableFiles = get_dir(dbPath + "*" + tableSuffix);

    foreach(string tableFile in tableFiles) {
      string dbName = chop(tableFile, tableSuffix, -1);
      string tableFileName = dbPath + tableFile;

      if(file_size(tableFileName) > 0) {
        string line, *lines = explode_file(tableFileName);
        __tableDefinitions[dbName] = ([]);

        foreach(line in lines) {
          string tableName, tableDefinition;

          if(sscanf(line, "%s=%s", tableName, tableDefinition) == 2)
            __tableDefinitions[dbName][tableName] = tableDefinition;
        }
      }
    }

    // Create databases and tables
    foreach(string dbName, mapping tables in __tableDefinitions) {
      string databaseFile = dbPath + dbName + dbSuffix;

      if(!sizeof(tables)) {
        log_file("system/db",
          "Skipping " + dbName + ": no parseable tables in .tbl.\n");
        continue;
      }

      __databases[dbName] = databaseFile;

      if(sizeof(tables)) {
        string err = catch {
          int fd;
          mixed result;
          int closeResult;

          fd = db_connect("", databaseFile, "", __USE_SQLITE3__);
          if(fd == 0) {
            log_file(
              "system/db",
              "Error connecting to " + dbName + " at " + databaseFile + "\n"
            );
            return;
          }

          foreach(string tableName,
            string tableDefinition in tables) {
            result = db_exec(
              fd,
              "CREATE TABLE IF NOT EXISTS " + tableName + " (" + tableDefinition + ")"
            );

            if(stringp(result)) {
              log_file("system/db",
                "Error creating table " +
                tableName + " in " +
                dbName + ": " +
                result + "\n");
              return;
            }
          }

          closeResult = db_close(fd);
          if(closeResult == 0) {
            log_file("system/db",
              "Error closing connection to " +
              dbName + " at " +
              databaseFile + "\n");
            return;
          }
        };
        if(err) {
          log_file("system/db",
            "Error creating tables in " +
            dbName + ": " + err + "\n");
          continue;
        }
      }
    }
  }
}

/**
 * Collates raw query result data into an array of mappings, using
 * the first row as column headers and subsequent rows as values.
 *
 * @private
 * @param {mixed*} result - The raw query result where result[0]
 *                          contains column names
 * @returns {mapping*} An array of mappings keyed by column name
 */
private mapping *collateData(mixed *result) {
  mapping *data = ({});
  int i, sz = sizeof(result);

  for(i = 1; i < sz; i++) {
    mapping row = ([]);
    int j, sz2 = sizeof(result[i]);

    for(j = 0; j < sz2; j++)
      row[result[0][j]] = result[i][j];

    data += ({row});
  }

  return data;
}

/**
 * Executes a SQL query on the specified database. If a callback
 * is provided, the result is passed to it asynchronously;
 * otherwise the collated result is returned directly.
 *
 * All failure cases are logged to /log/system/db. The return is
 * intentionally narrow so callers don't need to branch on error
 * shapes — they only ever ask "did I get rows?"
 *
 * @param {string} db - The name of the database to query
 * @param {string} q - The SQL query to execute
 * @param {mixed*} callback - Optional callback to handle the
 *                            result
 * @returns {mapping* | int} Collated rows on success, 0 if no
 *                           usable data (bad input, connection
 *                           failure, SQL error, or zero rows),
 *                           or 1 if using a callback
 */
public mixed query(string db, string q, mixed *callback) {
  string databaseFile;
  int fd;
  int closeResult, i;
  mixed rows, *result = ({});

  if(!db || !q) {
    log_file("system/db",
      "Invalid call: missing db or query.\n");
    return 0;
  }

  databaseFile = __databases[db];

  q = append(q, ";");
  fd = db_connect("", databaseFile, "", __USE_SQLITE3__);

  if(fd == 0) {
    log_file(
      "system/db",
      "Error connecting to " + db + " at " + databaseFile + "\n"
    );

    return 0;
  }

  rows = db_exec(fd, q);

  if(stringp(rows)) {
    log_file(
      "system/db",
      "Error querying " + db + ": " + rows + "\n"
    );

    db_close(fd);
    return 0;
  }

  if(rows == 0) {
    db_close(fd);
    return 0;
  }

  result = allocate(rows + 1);

  catch {
    for(i = 0; i <= rows; i++) {
      mixed info = db_fetch(fd, i);

      if(stringp(info))
        log_file(
          "system/db",
          "Error fetching row " + i + " in " + db + ": '" + q + "' " + info + "\n");
      else
        result[i] = info;
    }
  };

  closeResult = db_close(fd);

  if(closeResult == 0) {
    log_file(
      "system/db",
      "Error closing connection to " + db + " at " + databaseFile + "\n");
    // Data is already fetched — the close failure is a separate
    // resource issue. Log it and return the rows we have.
  }

  if(callback) {
    call_back(callback, collateData(result));

    return 1;
  }

  return collateData(result);
}

/**
 * Initiates a lazy (chunked) query execution that processes
 * large result sets in batches to avoid blocking the driver.
 * On failure (logged to /log/system/db), the callback receives 0.
 *
 * @param {string} db - The name of the database to query
 * @param {string} q - The SQL query to execute
 * @param {mixed*} callback - Callback to receive the accumulated
 *                            results when complete, or 0 on
 *                            failure
 */
public void lazyQuery(string db, string q,
  mixed *callback) {
  string queryId = db + "_" + time_ns();
  executeQuery(db, q, 0, queryId, callback);
}

/**
 * Executes a query in chunks using LIMIT/OFFSET, accumulating
 * results across call_out iterations to handle large result sets
 * without blocking. All failures log to /log/system/db; on any
 * failure the callback receives 0 and accumulated partial data
 * is discarded.
 *
 * @private
 * @param {string} db - The name of the database to query
 * @param {string} q - The SQL query to execute
 * @param {int} offset - The current offset for chunked retrieval
 * @param {string} queryId - Unique identifier for this query
 *                           execution
 * @param {mixed*} cb - Callback to receive the accumulated
 *                      results when complete, or 0 on failure
 */
private void executeQuery(
    string db,
    string q,
    int offset,
    string queryId,
    mixed *cb
  ) {
  string databaseFile, modifiedQuery;
  int fd, closeResult, i;
  mixed rows;

  databaseFile = __databases[db];

  // Modify the query to include LIMIT and OFFSET
  modifiedQuery = q + " LIMIT " + __dbChunkSize + " OFFSET " + offset;

  fd = db_connect("", databaseFile, "", __USE_SQLITE3__);
  if(fd == 0) {
    log_file(
      "system/db",
      "Error connecting to " + db + " at " + databaseFile + "\n"
    );

    if(cb)
      call_back(cb, 0);

    map_delete(__handle, queryId);

    return;
  }

  rows = db_exec(fd, modifiedQuery);

  if(stringp(rows)) {
    log_file(
      "system/db",
      "Error querying " + db + ": " + rows + "\n"
    );

    db_close(fd);

    if(cb)
      call_back(cb, 0);

    map_delete(__handle, queryId);

    return;
  }

  if(rows == 0) {
    // No more rows, finalise and invoke callback
    db_close(fd);

    if(cb)
      call_back(cb, __handle[queryId]);

    map_delete(__handle, queryId);

    return;
  }

  mixed *result = allocate(rows + 1);

  for(i = 0; i <= rows; i++) {
    mixed info = db_fetch(fd, i);

    if(stringp(info)) {
      log_file(
        "system/db",
        "Error fetching row " + i + " in " + db + ": '" + q + "' " + info + "\n"
      );
    } else {
      result[i] = info;
    }
  }

  // Accumulate the results for this chunk
  if(!pointerp(__handle[queryId]))
    __handle[queryId] = ({});

  __handle[queryId] += result;

  closeResult = db_close(fd);

  if(closeResult == 0) {
    log_file(
      "system/db",
      "Error closing connection to " + db + " at " + databaseFile + "\n"
    );

    if(cb)
      call_back(cb, 0);

    map_delete(__handle, queryId);

    return;
  }

  // Check if there might be more data to fetch
  if(rows == __dbChunkSize) {
    call_out("executeQuery", 1, db, q, offset + __dbChunkSize, queryId, cb);
  } else {
    // Final chunk, process accumulated result
    if(cb)
      call_back(cb, __handle[queryId]);

    map_delete(__handle, queryId);
  }
}

/**
 * Retrieves a copy of all available databases.
 *
 * @returns {([ string: string ])} A mapping of database names to
 *                                 their file paths
 */
public mapping queryDatabases() {
  return copy(__databases);
}

/**
 * Retrieves a copy of all table definitions for a specified
 * database.
 *
 * @param {string} dbName - The name of the database
 * @returns {([ string: string ])} A mapping of table names to
 *                                 their SQL definitions
 */
public mapping queryTables(string dbName) {
  return copy(__tableDefinitions[dbName]);
}

/**
 * Checks if a given database name is registered in the system.
 *
 * @param {string} db - The name of the database to check
 * @returns {int} 1 if the database exists, 0 otherwise
 */
public int validDb(string db) {
  return !nullp(__databases[db]);
}

/**
 * Checks if a given table exists in a specified database by
 * querying sqlite_master.
 *
 * @param {string} db - The name of the database
 * @param {string} table - The name of the table to check
 * @returns {int} 1 if the table exists, 0 otherwise
 */
public int validTable(string db, string table) {
  string statement;
  mixed result;

  if(!validDb(db))
    return 0;

  statement = sprintf(
    "SELECT name FROM sqlite_master " +
    "WHERE type='table' AND name=%s;",
    escapeValue(table)
  );

  result = query(db, statement, 0);

  if(!result)
    return 0;

  if(stringp(result))
    return 0;

  return sizeof(result) > 0;
}

/**
 * Retrieves the SQLite version for the specified database,
 * returned as an array of version components.
 *
 * @param {string} db - The name of the database
 * @returns {string* | int} An array of version strings
 *                          (e.g. ({ "3", "39", "0" })), 0 if
 *                          the database is invalid or version is
 *                          a string, or -1 on query failure
 */
public mixed sqliteVersion(string db) {
  string statement = "SELECT sqlite_version() ;";
  mixed result;

  if(!validDb(db))
    return 0;

  result = query(db, statement, 0);

  if(!result)
    return -1;

  if(stringp(result))
    return 0;

  return explode(result[0][0], ".");
}

/**
 * Generates a SQL INSERT values clause from a mapping of
 * column-value pairs. String values are escaped for single
 * quotes.
 *
 * @param {([ string: mixed ])} data - The mapping containing
 *                                     column names and values
 * @returns {string} The generated clause in the form
 *                   "(col1,col2) VALUES (val1,val2)", or null
 *                   if the mapping is empty
 */
public string statementFromMapping(mapping data) {
  if(!sizeof(data))
    return null;

  string *columns = ({});
  string *values = ({});

  foreach(mixed k, mixed v in data) {
    if(!stringp(k))
      continue;

    array_push(ref columns, escapeIdent(k));
    array_push(ref values, escapeValue(v));
  }

  if(!sizeof(columns))
    return null;

  return "(" + implode(columns, ",") + ") VALUES (" +
    implode(values, ",") + ")";
}

/**
 * Checks if the SQLite version supports upsert operations
 * (requires SQLite 3.24+).
 *
 * @param {string} db - The name of the database to check
 * @returns {int} 1 if upsert is supported, 0 otherwise
 */
public int allowUpsert(string db) {
  mixed version = sqliteVersion(db);

  if(!arrayp(version) || sizeof(version) < 3)
    return 0;

  return (version[0] > 3) ||
    (version[0] == 3 && version[1] >= 24);
}

/**
 * Executes a REST-style database operation using a URL-based
 * interface. Maps HTTP verbs to SQL operations:
 * GET = SELECT, POST = INSERT, PUT = UPDATE, DELETE = DELETE.
 * URLs follow the form: db://database/table?key=val&key=val
 * Special query params: _limit, _offset, _order (col:asc/desc).
 *
 * @param {string} method - The HTTP method (GET, POST, PUT, DELETE)
 * @param {string} url - The database URL (db://database/table?query)
 * @param {mapping} [data] - Column:value pairs for POST or PUT
 * @param {mixed*} [callback] - Optional callback for async results
 * @returns {mixed} Query results, error string, or 1 if using callback
 *
 * @errors If the URL format is invalid
 * @errors If the database or table does not exist
 * @errors If POST or PUT is missing a data mapping
 * @errors If PUT or DELETE has no query params for WHERE
 * @errors If the method is not GET, POST, PUT, or DELETE
 */
public varargs mixed rest(string method, string url,
  mapping data, mixed *callback) {
  mapping parsed = parseDbUrl(url);

  if(!parsed)
    error("Invalid URL format. Expected: db://database/table");

  string db = parsed["db"];
  string table = parsed["table"];
  mapping params = parsed["query"];

  if(!validDb(db))
    error("Unknown database: " + db);

  if(!validTable(db, table))
    error("Unknown table: " + table + " in " + db);

  // Separate special params from WHERE conditions
  string orderClause = "";
  string limitClause = "";
  string offsetClause = "";

  if(mapp(params)) {
    if(!nullp(params["_order"])) {
      string *orderParts = explode(params["_order"], ":");
      string col = orderParts[0];
      string dir = sizeof(orderParts) > 1 ?
        upper_case(orderParts[1]) : "ASC";

      if(dir != "ASC" && dir != "DESC")
        dir = "ASC";

      orderClause = " ORDER BY " + escapeIdent(col) + " " + dir;
      map_delete(params, "_order");
    }

    if(!nullp(params["_limit"])) {
      limitClause = " LIMIT " + to_int(params["_limit"]);
      map_delete(params, "_limit");
    }

    if(!nullp(params["_offset"])) {
      offsetClause = " OFFSET " + to_int(params["_offset"]);
      map_delete(params, "_offset");
    }
  }

  string where = whereFromMapping(params);
  string q;

  switch(method) {
    case "GET":
      q = "SELECT * FROM " + escapeIdent(table) + where +
        orderClause + limitClause + offsetClause;
      break;
    case "POST": {
      if(!mapp(data) || !sizeof(data))
        error("POST requires a data mapping.");

      string values = statementFromMapping(data);

      if(!values)
        error("Failed to build INSERT statement.");

      q = "INSERT INTO " + escapeIdent(table) + " " + values;
      break;
    }
    case "PUT": {
      if(!mapp(data) || !sizeof(data))
        error("PUT requires a data mapping.");

      if(!strlen(where))
        error("PUT requires query params for WHERE clause.");

      string set = setFromMapping(data);
      q = "UPDATE " + escapeIdent(table) + " " + set + where;
      break;
    }
    case "DELETE": {
      if(!strlen(where))
        error("DELETE requires query params for WHERE clause.");

      q = "DELETE FROM " + escapeIdent(table) + where;
      break;
    }
    default:
      error("Unsupported method: " + method);
  }

  debug("query %O\n", q);
  return query(db, q, callback);
}

/**
 * Parses a db:// URL into its components.
 *
 * @private
 * @param {string} url - URL in the form db://database/table?query
 * @returns {mapping} Parsed components with keys: db, table, query
 */
private mapping parseDbUrl(string url) {
  string *matches = pcre_extract(url,
    "^db://([^/?]+)/([^/?]+)(?:\\?(.*))?$"
  );

  if(!sizeof(matches) || sizeof(matches) < 2)
    return 0;

  mapping result = ([
    "db" : matches[0],
    "table" : matches[1],
    "query" : sizeof(matches) > 2 ?
      parse_query(matches[2]) : ([]),
  ]);

  return result;
}

/**
 * Escapes a value for safe inclusion in a SQL statement.
 *
 * @private
 * @param {mixed} v - The value to escape
 * @returns {string} The escaped value as a SQL literal
 */
private string escapeValue(mixed v) {
  if(typeof(v) == T_STRING)
    return "'" + replace_string(v, "'", "''") + "'";

  return "" + v;
}

/**
 * Escapes a SQL identifier (table or column name) using SQL-standard
 * double-quote quoting, with embedded double quotes doubled.
 *
 * @private
 * @param {string} s - The identifier to escape
 * @returns {string} The quoted identifier
 */
private string escapeIdent(string s) {
  return "\"" + replace_string(s, "\"", "\"\"") + "\"";
}

/**
 * Builds a SQL WHERE clause from a mapping of conditions.
 *
 * @private
 * @param {mapping} m - Mapping of column:value pairs
 * @returns {string} WHERE clause string, or empty string if no
 *                   conditions
 */
private string whereFromMapping(mapping m) {
  if(!mapp(m) || !sizeof(m))
    return "";

  string *conditions = ({});

  foreach(string k, mixed v in m)
    conditions += ({ escapeIdent(k) + " = " + escapeValue(v) });

  return " WHERE " + implode(conditions, " AND ");
}

/**
 * Builds a SQL SET clause from a mapping of column:value pairs.
 *
 * @private
 * @param {mapping} m - Mapping of column:value pairs
 * @returns {string} SET clause string
 */
private string setFromMapping(mapping m) {
  string *assignments = ({});

  foreach(string k, mixed v in m)
    assignments += ({ escapeIdent(k) + " = " + escapeValue(v) });

  return "SET " + implode(assignments, ", ");
}

#endif // __USE_SQLITE3__
#endif // __PACKAGE_DB__
