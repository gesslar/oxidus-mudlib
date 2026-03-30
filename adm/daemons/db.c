/**
 * @file /adm/daemons/db.c
 *
 * Database daemon providing SQLite3 database management and query
 * execution. Handles database creation, table management, and
 * provides synchronous and chunked (lazy) query interfaces.
 *
 * @created 2024-02-27 - Gesslar
 * @last_modified 2024-02-27 - Gesslar
 *
 * @history
 * 2024-02-27 - Gesslar - Created
 */

inherit STD_DAEMON;

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

      __databases[dbName] = databaseFile;

      if(sizeof(tables)) {
        mixed err = catch {
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
 * @param {string} db - The name of the database to query
 * @param {string} q - The SQL query to execute
 * @param {mixed*} callback - Optional callback to handle the
 *                            result
 * @returns {mapping* | string | int} Collated query results, an
 *                                    error string, 0 on failure,
 *                                    or 1 if using a callback
 */
public mixed query(string db, string q,
  mixed *callback) {
  string databaseFile = __databases[db];
  int fd;
  int closeResult, i;
  mixed rows, *result = ({});

  if(!db || !q)
    return "Invalid db or query.";

  q = append(q, ";");
  fd = db_connect("", databaseFile, "", __USE_SQLITE3__);

  if(fd == 0) {
    log_file("system/db",
      "Error connecting to " + db +
      " at " + databaseFile + "\n");
    return 0;
  }

  rows = db_exec(fd, q);

  if(stringp(rows)) {
    log_file("system/db",
      "Error querying " + db +
      ": " + rows + "\n");
    return "Error querying " + db +
      ": " + rows + "\n";
  }

  if(rows == 0)
    return 0;

  result = allocate(rows + 1);
  catch {
    for(i = 0; i <= rows; i++) {
      mixed info = db_fetch(fd, i);

      if(stringp(info))
        log_file("system/db",
          "Error fetching row " + i +
          " in " + db + ": '" + q +
          "' " + info + "\n");
      else
        result[i] = info;
    }
  };

  closeResult = db_close(fd);
  if(closeResult == 0) {
    log_file("system/db",
      "Error closing connection to " + db +
      " at " + databaseFile + "\n");
    return "Error closing connection to " +
      db + " at " + databaseFile + "\n";
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
 *
 * @param {string} db - The name of the database to query
 * @param {string} q - The SQL query to execute
 * @param {mixed*} callback - Callback to receive the accumulated
 *                            results when complete
 */
public void lazyQuery(string db, string q,
  mixed *callback) {
  string queryId = db + "_" + time_ns();
  executeQuery(db, q, 0, queryId, callback);
}

/**
 * Executes a query in chunks using LIMIT/OFFSET, accumulating
 * results across call_out iterations to handle large result sets
 * without blocking.
 *
 * @private
 * @param {string} db - The name of the database to query
 * @param {string} q - The SQL query to execute
 * @param {int} offset - The current offset for chunked retrieval
 * @param {string} queryId - Unique identifier for this query
 *                           execution
 * @param {mixed*} cb - Callback to receive the accumulated
 *                            results when complete
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
  mixed rows, *result;

  databaseFile = __databases[db];
  result = ({});

  // Modify the query to include LIMIT and OFFSET
  modifiedQuery = q + " LIMIT " + __dbChunkSize + " OFFSET " + offset;

  fd = db_connect("", databaseFile, "", __USE_SQLITE3__);
  if(fd == 0) {
    log_file(
      "system/db",
      "Error connecting to " + db + " at " + databaseFile + "\n"
    );

    if(cb)
      call_back(cb, "Error: Connection failed.");

    map_delete(__handle, queryId);

    return;
  }

  rows = db_exec(fd, modifiedQuery);

  if(stringp(rows)) {
    log_file(
      "system/db",
      "Error querying " + db + ": " + rows + "\n"
    );

    if(cb)
      call_back(cb,"Error: Query failed - " + rows);

    map_delete(__handle, queryId);

    return;
  }

  if(rows == 0) {
    // No more rows, finalise and invoke callback
    if(cb)
      call_back(cb, __handle[queryId]);

    map_delete(__handle, queryId);

    return;
  }

  result = allocate(rows + 1);
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
  if(!arrayp(__handle[queryId]))
    __handle[queryId] = ({});

  __handle[queryId] += result;

  closeResult = db_close(fd);

  if(closeResult == 0) {
    log_file(
      "system/db",
      "Error closing connection to " + db + " at " + databaseFile + "\n"
    );

    if(cb)
      call_back(cb, "Error: Connection close failed.");

    map_delete(__handle, queryId);

    return;
  }

  // Check if there might be more data to fetch
  if(sizeof(rows) == __dbChunkSize) {
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
  string statement = sprintf(
    "SELECT name FROM sqlite_master " +
    "WHERE type='table' AND name='%s';",
    table
  );

  mixed result;

  if(!validDb(db))
    return 0;

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

  string statement = "(" + implode(keys(data), ",") + ") VALUES (";

  string *values = ({});
  foreach(mixed k, mixed v in data) {
    if(!stringp(k))
      continue;

    if(typeof(v) == T_STRING)
      array_push(ref values, "'" + replace_string(v, "'", "''") + "'");
    else
      array_push(ref values, (string)v);
  }

  statement += implode(values, ",") + ")";

  return statement;
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

#endif // __USE_SQLITE3__
#endif // __PACKAGE_DB__
