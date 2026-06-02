/**
 * @file /adm/obj/security_editor.c
 *
 * Security editor object for managing group memberships and access
 * control lists. Must be cloned to be used.
 *
 * @created 2006-01-14 - Tacitus @ LPUniversity
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2006-01-14 - Tacitus @ LPUniversity - Created
 * 2026-03-23 - Gesslar - Modernised to current coding standards
 */

inherit STD_OBJECT;

#define FILE_GROUPDATA  "/adm/etc/groups"
#define FILE_ACCESSDATA "/adm/etc/access"

// Forward declarations
private void parse_files();
private void parse_group();
private void parse_access();
public void write_state(int flag);
private void write_group_file(int flag);
private void write_access_file(int flag);
private void integrity_check();

// Global variables
private mapping access = ([]);
private mapping groups = ([]);

void setup() {
  if(clonep())
    parse_files();
}

/**
 * Parses a raw file string into an array of non-comment,
 * whitespace-stripped lines. Null entries replace comment
 * lines.
 *
 * @param {string} str - The raw file contents.
 * @returns {string *} Array of parsed lines (with nulls for
 *                        comments).
 */
private string *parse(string str) {
  integrity_check();

  if(!str)
    return ({});

  string *arr = explode(str, "\n");
  int sz = sizeof(arr);

  for(int i = 0; i < sz; i++) {
    if(arr[i][0] == '#') {
      arr[i] = 0;
      continue;
    }
    arr[i] = replace_string(arr[i], " ", "");
    arr[i] = replace_string(arr[i], "\t", "");
  }

  return arr;
}

private void parse_files() {
  integrity_check();
  parse_group();
  parse_access();
}

void parse_group() {
  integrity_check();

  string *arr = parse(read_file(FILE_GROUPDATA));
  int sz = sizeof(arr);

  groups = ([]);

  for(int i = 0; i < sz; i++) {
    if(!arr[i])
      continue;

    string group, str;
    if(sscanf(arr[i], "(%s)%s", group, str) != 2) {
      tell_me("Error [security]: Invalid format of data in "
        "group data."
      );
      tell_me("Security alert: Ignoring group on line "
        + (i + 1)
      );
      continue;
    }

    string *members = explode(str, ":");
    int msz = sizeof(members);

    for(int n = 0; n < msz; n++) {
      string file = user_data_file(members[n]);

      if(!file_size(file) && !sscanf(members[n], "[%*s]")) {
        tell_me("Error [security]: Unknown user detected.");
        tell_me(
          "Security alert: User '" + members[n] + "' ignored for group '" + group + "'."
        );
        members -= ({ members[n] });
        msz--;
        continue;
      }
    }

    groups[group] = members;
  }
}

private void parse_access() {
  integrity_check();

  string *arr = parse(read_file(FILE_ACCESSDATA));
  int sz = sizeof(arr);

  access = ([]);

  for(int i = 0; i < sz; i++) {
    if(!arr[i])
      continue;

    string directory, str;
    if(sscanf(arr[i], "(%s)%s", directory, str) != 2) {
      tell_me("Error [security]: Invalid format of data in "
        "access data.");
      error("Security alert: Fatal error parsing access data "
        "on line " + (i + 1));
    }

    if(str[<1..<1] == ":") {
      tell_me("Error [security]: Incomplete data in access "
        "data (trailing ':').");
      error("Security alert: Fatal error parsing access data "
        "on line " + (i + 1));
    }

    string *entries = explode(str, ":");
    int esz = sizeof(entries);
    mapping data = ([]);

    for(int n = 0; n < esz; n++) {
      string identity, permissions;
      if(sscanf(entries[n], "%s[%s]", identity, permissions)
      != 2) {
        tell_me("Error [security]: Invalid entry(" + n
          + ") data format in access data.");
        error("Security alert: Fatal error parsing access "
          "data on line " + (i + 1));
      }

      // read, write, network, shadow, link, execute, bind,
      // ownership
      string *perm_arr = allocate(8);
      if(strsrch(permissions, "r") != -1) perm_arr[0] = "r";
      if(strsrch(permissions, "w") != -1) perm_arr[1] = "w";
      if(strsrch(permissions, "n") != -1) perm_arr[2] = "n";
      if(strsrch(permissions, "s") != -1) perm_arr[3] = "s";
      if(strsrch(permissions, "l") != -1) perm_arr[4] = "l";
      if(strsrch(permissions, "e") != -1) perm_arr[5] = "e";
      if(strsrch(permissions, "b") != -1) perm_arr[6] = "b";
      if(strsrch(permissions, "o") != -1) perm_arr[7] = "o";

      data[identity] = perm_arr;
    }

    access[directory] = data;
  }
}

/**
 * Creates a new group with the given members.
 *
 * @param {string} group - The group name.
 * @param {string *} members - Array of member names.
 * @returns {int} 1 on success, 0 on failure.
 */
public int create_group(string group, string *members) {
  integrity_check();

  if(groups[group])
    return 0;

  if(!sizeof(members))
    return 0;

  groups[group] = members;

  return 1;
}

/**
 * Deletes a group.
 *
 * @param {string} group - The group name.
 * @returns {int} 1 on success, 0 if group does not exist.
 */
public int delete_group(string group) {
  integrity_check();

  if(!groups[group])
    return 0;

  map_delete(groups, group);

  return 1;
}

/**
 * Adds a user to a group.
 *
 * @param {string} user - The user name.
 * @param {string} group - The group name.
 * @returns {int} 1 on success, 0 if group does not exist or
 *                  user is already a member.
 */
public int enable_membership(string user, string group) {
  integrity_check();

  if(!groups[group])
    return 0;

  if(member_array(user, groups[group]) != -1)
    return 0;

  groups[group] += ({ user });

  return 1;
}

/**
 * Removes a user from a group.
 *
 * @param {string} user - The user name.
 * @param {string} group - The group name.
 * @returns {int} 1 on success, 0 if group does not exist or
 *                  user is not a member.
 */
public int disable_membership(string user, string group) {
  integrity_check();

  if(!groups[group])
    return 0;

  if(member_array(user, groups[group]) == -1)
    return 0;

  groups[group] -= ({ user });

  return 1;
}

/**
 * Toggles a user's membership in a group.
 *
 * @param {string} user - The user name.
 * @param {string} group - The group name.
 * @returns {int} 1 on success, 0 if group does not exist.
 */
public int toggle_membership(string user, string group) {
  integrity_check();

  if(!groups[group])
    return 0;

  if(member_array(user, groups[group]) == -1)
    groups[group] += ({ user });
  else
    groups[group] -= ({ user });

  return 1;
}

/**
 * Sets access permissions for an identity on a
 * directory.
 *
 * @param {string} dir - The directory path.
 * @param {string} id - The identity (user or group).
 * @param {string *} akeys - Array of permission keys.
 * @returns {int} 1 on success, 0 if no keys provided.
 */
public int set_access(string dir, string id, string *akeys) {
  integrity_check();

  if(!sizeof(akeys))
    return 0;

  if(!access[dir])
    access[dir] = ([ id : akeys ]);
  else
    access[dir][id] = akeys;

  return 1;
}

/**
 * Returns the list of all group names.
 *
 * @returns {string *} Array of group names.
 */
public string *list_groups() {
  integrity_check();

  return keys(groups);
}

public void write_state(int flag) {
  integrity_check();

  if(!adminp(previous_object()) && !adminp(this_body()))
    return;

  write_group_file(flag);
  write_access_file(flag);
}

private void write_group_file(int flag) {
  integrity_check();

  if(!adminp(previous_object()) && !adminp(this_body()))
    return;

  string *group_list = keys(groups);
  int sz = sizeof(group_list);
  string file = "";

  for(int i = 0; i < sz; i++) {
    string *group_data = groups[group_list[i]];
    int gsz = sizeof(group_data);

    file += "(" + group_list[i] + ")";

    if(gsz > 1)
      file += implode(group_data, ":") + "\n";
    else if(gsz == 1)
      file += group_data[0] + "\n";
    else
      error("ERROR: Group '" + group_list[i]
        + "' has no members!");
  }

  if(flag) {
    tell_me(file);
  } else {
    write_file(FILE_GROUPDATA, file, 1);
    parse_files();

    string err = "";
    err += catch(destruct(master()));
    err += catch(
      destruct(find_object("/adm/obj/master/valid"))
    );
    err += catch(load_object("/adm/obj/master/valid"));
    err += catch(load_object("/adm/obj/master"));
    err += catch(CONFIG_D->rehash_config());

    if(err != "00000")
      tell_me(err);
  }
}

private void write_access_file(int flag) {
  integrity_check();

  if(!adminp(previous_object()) && !adminp(this_body()))
    return;

  string *access_list = keys(access);
  int sz = sizeof(access_list);
  string file = "";

  for(int i = 0; i < sz; i++) {
    mapping access_data = access[access_list[i]];
    string *access_keys = keys(access_data);
    int ksz = sizeof(access_keys);
    string *arr = ({});

    file += "(" + access_list[i] + ") ";

    for(int j = 0; j < ksz; j++)
      arr += ({
        sprintf("%s[%s]", access_keys[j],
          implode(access_data[access_keys[j]], ""))
      });

    if(sizeof(arr) > 1)
      file += implode(arr, ":") + "\n";
    else
      file += arr[0] + "\n";
  }

  if(flag) {
    tell_me(file);
  } else {
    write_file(FILE_ACCESSDATA, file, 1);
    parse_files();

    string err = "";
    err += catch(destruct(master()));
    err += catch(
      destruct(find_object("/adm/obj/master/valid"))
    );
    err += catch(load_object("/adm/obj/master/valid"));
    err += catch(load_object("/adm/obj/master"));
    err += catch(CONFIG_D->rehash_config());

    if(err != "00000")
      tell_me(err);
  }
}

private void integrity_check() {
  if(!clonep())
    error("Error [security_editor]: This object must be "
      "cloned to be used.");
}
