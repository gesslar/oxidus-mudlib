// /adm/daemons/db.c
// DB Daemon
//
// Created:     2024/02/27: Gesslar
// Last Change: 2024/02/27: Gesslar
//
// 2024/02/27: Gesslar - Created

inherit STD_DAEMON;

#ifndef __PACKAGE_DB__
#else
#ifndef __USE_SQLITE3__
#else

// Forward declarations
int valid_db(string db);
int valid_table(string db, string table);
mixed query(string db, string q, mixed *callback);
int allow_upsert(string db);
mixed sqlite_version(string db);
string statement_from_mapping(mapping data);
private mapping *collate_data(mixed *result);
private void execute_query(string db, string q, int offset, string query_id, mixed *callback);
mapping query_databases();
mapping query_tables(string db_name);
void lazy_query(string db, string q, mixed *callback);

private nosave mapping handle = ([ ]);
private nosave mapping databases = ([ ]);
private nosave mapping table_definitions = ([ ]);
private nosave int db_chunk_size = mudConfig("DB_CHUNK_SIZE");

void setup() {
    string db_path = mudConfig("DB_PATH");
    string db_suffix = mudConfig("DB_SUFFIX");
    string table_suffix = mudConfig("DB_TABLE_SUFFIX");

    databases = ([ ]);
    table_definitions = ([ ]);

    if(strlen(db_path) > 0) {
        // Find all table definition files first
        string *table_files = get_dir(db_path + "*" + table_suffix);

        foreach(string table_file in table_files) {
            string db_name = chop(table_file, table_suffix, -1);
            string table_file_name = db_path + table_file;

            if(file_size(table_file_name) > 0) {
                string line, *lines = explode_file(table_file_name);
                table_definitions[db_name] = ([ ]);

                foreach(line in lines) {
                    int pos;
                    string table_name, table_definition;

                    if(sscanf(line, "%s=%s", table_name, table_definition) == 2) {
                        table_definitions[db_name][table_name] = table_definition;
                    }
                }
            }
        }

        // Now, create databases and tables based on the table definitions
        foreach(string db_name, mapping tables in table_definitions) {
            string database_file = db_path + db_name + db_suffix;
            int con = -1;

            databases[db_name] = database_file;

            if(sizeof(tables)) {
                mixed err = catch {
                    int fd;
                    mixed result;
                    int close_result;
                    string current;

                    fd = db_connect("", database_file, "", __USE_SQLITE3__);
                    if(fd == 0) {
                        log_file("system/db", "Error connecting to " + db_name + " at " + database_file + "\n");
                        return;
                    }

                    foreach(string table_name, string table_definition in tables) {
                        current = table_name;
                        result = db_exec(fd, "CREATE TABLE IF NOT EXISTS " + table_name + " (" + table_definition + ")");

                        if(stringp(result)) {
                            log_file("system/db", "Error creating table " + table_name + " in " + db_name + ": " + result + "\n");
                            return;
                        }
                    }

                    close_result = db_close(fd);
                    if(close_result == 0) {
                        log_file("system/db", "Error closing connection to " + db_name + " at " + database_file + "\n");
                        return;
                    }
                };
                if(err) {
                    log_file("system/db", "Error creating tables in " + db_name + ": " + err + "\n");
                    continue;
                }
            }
        }
    }
}

/**
 * @function collate_data
 * @description Collates query result data into a more usable format.
 * @param {mixed[]} result - The raw query result.
 * @returns {mapping[]} - An array of mappings representing the collated data.
 */
private mapping *collate_data(mixed *result) {
    mapping *data = ({ });
    int i, sz = sizeof(result);

    for(i = 1; i < sz; i++) {
        mapping row = ([ ]);
        int j, sz2 = sizeof(result[i]);

        for(j = 0; j < sz2; j++) {
            row[result[0][j]] = result[i][j];
        }

        data += ({ row });
    }

    return data;
}

/**
 * @daemon_function query
 * @description Executes a SQL query on the specified database.
 * @param {string} db - The name of the database to query.
 * @param {string} q - The SQL query to execute.
 * @param {mixed[]} callback - Optional callback function to handle the result.
 * @returns {mixed} - Query result or 1 if callback is provided.
 */
mixed query(string db, string q, mixed *callback) {
    string database_file = databases[db];
    int fd;
    int close_result, i;
    mixed rows, *result = ({ });

    if(!db || !q)
        return "Invalid db or query.";

    q = append(q, ";");
    fd = db_connect("", database_file, "", __USE_SQLITE3__);
    if(fd == 0) {
        log_file("system/db", "Error connecting to " + db + " at " + database_file + "\n");
        return 0;
    }

    rows = db_exec(fd, q);

    if(stringp(rows)) {
        log_file("system/db", "Error querying " + db + ": " + rows + "\n");
        return "Error querying " + db + ": " + rows + "\n";
    }

    if(rows == 0)
        return 0;

    result = allocate(rows+1);
    catch {
        for(i = 0; i <= rows; i++) {
            mixed info = db_fetch(fd, i);

            if(stringp(info))
                log_file("system/db", "Error fetching row " + i + " in " + db + ": '" + q + "' " + info + "\n");
            else
                result[i] = info;
        }
    };

    close_result = db_close(fd);
    if(close_result == 0) {
        log_file("system/db", "Error closing connection to " + db + " at " + database_file + "\n");
        return "Error closing connection to " + db + " at " + database_file + "\n";
    }

    if(callback) {
        call_back(callback, collate_data(result));
        return 1;
    }

    return collate_data(result);
}

/**
 * @daemon_function lazy_query
 * @description Initiates a lazy (chunked) query execution.
 * @param {string} db - The name of the database to query.
 * @param {string} q - The SQL query to execute.
 * @param {mixed[]} callback - Callback function to handle the result.
 */
void lazy_query(string db, string q, mixed *callback) {
    string query_id = db + "_" + time_ns();
    execute_query(db, q, 0, query_id, callback);
}

/**
 * @function execute_query
 * @description Executes a query in chunks, handling large result sets efficiently.
 * @param {string} db - The name of the database to query.
 * @param {string} q - The SQL query to execute.
 * @param {int} offset - The current offset for chunked queries.
 * @param {string} query_id - A unique identifier for this query execution.
 * @param {mixed[]} callback - Callback function to handle the result.
 */
private void execute_query(string db, string q, int offset, string query_id, mixed *callback) {
    string database_file, modified_query;
    int fd, close_result, i;
    mixed rows, *result;

    database_file = databases[db];
    result = ({ });

    // Modify the query to include LIMIT and OFFSET
    modified_query = q + " LIMIT " + db_chunk_size + " OFFSET " + offset;

    fd = db_connect("", database_file, "", __USE_SQLITE3__);
    if(fd == 0) {
        log_file("system/db", "Error connecting to " + db + " at " + database_file + "\n");
        if(callback) call_back(callback, "Error: Connection failed.");
        map_delete(handle, query_id);
        return;
    }

    rows = db_exec(fd, modified_query);

    if(stringp(rows)) {
        log_file("system/db", "Error querying " + db + ": " + rows + "\n");
        if(callback) call_back(callback, "Error: Query failed - " + rows);
        map_delete(handle, query_id);
        return;
    }

    if(rows == 0) {
        // If no more rows, finalize the result and invoke the callback
        if(callback) call_back(callback, handle[query_id]);
        map_delete(handle, query_id);
        return;
    }

    result = allocate(rows+1);
    for(i = 0; i <= rows; i++) {
        mixed info = db_fetch(fd, i);
        if(stringp(info)) {
            log_file("system/db", "Error fetching row " + i + " in " + db + ": '" + q + "' " + info + "\n");
        } else {
            result[i] = info;
        }
    }

    // Accumulate the results for this chunk
    if(!arrayp(handle[query_id])) {
        handle[query_id] = ({ });
    }
    handle[query_id] += result ; // Append the results from this chunk

    close_result = db_close(fd);
    if(close_result == 0) {
        log_file("system/db", "Error closing connection to " + db + " at " + database_file + "\n");
        if(callback) call_back(callback, "Error: Connection close failed.");
        map_delete(handle, query_id);
        return;
    }

    // If the chunk returned exactly db_chunk_size rows, there might be more data to fetch
    if(sizeof(rows) == db_chunk_size) {
        call_out("execute_query", 1, db, q, offset + db_chunk_size, query_id, callback);
    } else {
        // Final chunk, process the accumulated result
        if(callback) call_back(callback, handle[query_id]);
        map_delete(handle, query_id);
    }
}

/**
 * @daemon_function query_databases
 * @description Retrieves a list of all available databases.
 * @returns {mapping} - A mapping of database names to their file paths.
 */
mapping query_databases() {
    return copy(databases);
}

/**
 * @daemon_function query_tables
 * @description Retrieves a list of tables for a specified database.
 * @param {string} db_name - The name of the database.
 * @returns {mapping} - A mapping of table names to their definitions.
 */
mapping query_tables(string db_name) {
    return copy(table_definitions[db_name]);
}

/**
 * @daemon_function valid_db
 * @description Checks if a given database is valid and exists in the system.
 * @param {string} db - The name of the database to check.
 * @returns {int} - 1 if the database is valid, 0 otherwise.
 */
int valid_db(string db) {
    return !nullp(databases[db]);
}

/**
 * @daemon_function valid_table
 * @description Checks if a given table exists in a specified database.
 * @param {string} db - The name of the database.
 * @param {string} table - The name of the table to check.
 * @returns {int} - 1 if the table exists, 0 otherwise.
 */
int valid_table(string db, string table) {
    string statement = sprintf("SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", table);
    mixed result;

    if(!valid_db(db))
        return 0;

    result = query(db, statement, 0);

    if(!result)
        return 0;

    if(stringp(result))
        return 0;

    return sizeof(result) > 0;
}

/**
 * @daemon_function sqlite_version
 * @description Retrieves the SQLite version for the specified database.
 * @param {string} db - The name of the database.
 * @returns {mixed} - An array of version numbers or an error code.
 */
mixed sqlite_version(string db) {
    string statement = "SELECT sqlite_version() ;";
    mixed result;

    if(!valid_db(db))
        return 0;

    result = query(db, statement, 0);

    if(!result)
        return -1;

    if(stringp(result))
        return 0;

    return explode(result[0][0], ".");
}

/**
 * @daemon_function statement_from_mapping
 * @description Generates a SQL statement from a mapping of column-value pairs.
 * @param {mapping} data - The mapping containing column names and values.
 * @returns {string} - The generated SQL statement or null if the mapping is empty.
 */
string statement_from_mapping(mapping data) {
    string *values = ({ });
    string statement;

    if(!sizeof(data))
        return null;

    statement = "(" + implode(keys(data), ",") + ") VALUES (";
    foreach(mixed k, mixed v in data) {
        if(!stringp(k))
            continue;

        if(typeof(v) == T_STRING)
            values += ({ "'" + replace_string(v, "'", "''") + "'" });
        else
            values += ({ (string)v });
    }

    statement += implode(values, ",") + ")";

    return statement;
}

/**
 * @daemon_function allow_upsert
 * @description Checks if the SQLite version supports upsert operations.
 * @param {string} db - The name of the database to check.
 * @returns {int} - 1 if upsert is supported, 0 otherwise.
 */
int allow_upsert(string db) {
    mixed version = sqlite_version(db);

    if(!arrayp(version) || sizeof(version) < 3)
        return 0;

    return (version[0] > 3) || (version[0] == 3 && version[1] >= 24);
}

#endif // __USE_SQLITE3__
#endif // __PACKAGE_DB__
