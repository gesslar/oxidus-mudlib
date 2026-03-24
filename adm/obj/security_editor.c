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

#include <simul_efun.h>

inherit STD_OBJECT;

#define FILE_GROUPDATA  "/adm/etc/groups"
#define FILE_ACCESSDATA "/adm/etc/access"

// Forward declarations
private void parseFiles();
private void parseGroup();
private void parseAccess();
public void writeState(int flag);
private void writeGroupFile(int flag);
private void writeAccessFile(int flag);
private void integrityCheck();

// Global variables
private mapping access = ([]);
private mapping groups = ([]);

void setup() {
  if(clonep())
    parseFiles();
}

/**
 * @description Parses a raw file string into an array of non-comment,
 *              whitespace-stripped lines. Null entries replace comment
 *              lines.
 * @param {string} str - The raw file contents.
 * @returns {string *} - Array of parsed lines (with nulls for
 *                        comments).
 */
private string *parse(string str) {
  integrityCheck();

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

private void parseFiles() {
  integrityCheck();
  parseGroup();
  parseAccess();
}

private void parseGroup() {
  integrityCheck();

  string *arr = parse(read_file(FILE_GROUPDATA));
  int sz = sizeof(arr);

  groups = ([]);

  for(int i = 0; i < sz; i++) {
    if(!arr[i])
      continue;

    string group, str;
    if(sscanf(arr[i], "(%s)%s", group, str) != 2) {
      tell_me("Error [security]: Invalid format of data in "
        "group data.");
      tell_me("Security alert: Ignoring group on line "
        + (i + 1));
      continue;
    }

    string *members = explode(str, ":");
    int msz = sizeof(members);

    for(int n = 0; n < msz; n++) {
      if(!file_size(user_data_file(members[n]))
      && !sscanf(members[n], "[%*s]")) {
        tell_me("Error [security]: Unknown user detected.");
        tell_me("Security alert: User '" + members[n]
          + "' ignored for group '" + group + "'.");
        members -= ({ members[n] });
        msz--;
        continue;
      }
    }

    groups[group] = members;
  }
}

private void parseAccess() {
  integrityCheck();

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
      string *permArr = allocate(8);
      if(strsrch(permissions, "r") != -1) permArr[0] = "r";
      if(strsrch(permissions, "w") != -1) permArr[1] = "w";
      if(strsrch(permissions, "n") != -1) permArr[2] = "n";
      if(strsrch(permissions, "s") != -1) permArr[3] = "s";
      if(strsrch(permissions, "l") != -1) permArr[4] = "l";
      if(strsrch(permissions, "e") != -1) permArr[5] = "e";
      if(strsrch(permissions, "b") != -1) permArr[6] = "b";
      if(strsrch(permissions, "o") != -1) permArr[7] = "o";

      data[identity] = permArr;
    }

    access[directory] = data;
  }
}

/**
 * @description Creates a new group with the given members.
 * @param {string} group - The group name.
 * @param {string *} members - Array of member names.
 * @returns {int} - 1 on success, 0 on failure.
 */
public int createGroup(string group, string *members) {
  integrityCheck();

  if(groups[group])
    return 0;

  if(!sizeof(members))
    return 0;

  groups[group] = members;

  return 1;
}

/**
 * @description Deletes a group.
 * @param {string} group - The group name.
 * @returns {int} - 1 on success, 0 if group does not exist.
 */
public int deleteGroup(string group) {
  integrityCheck();

  if(!groups[group])
    return 0;

  map_delete(groups, group);

  return 1;
}

/**
 * @description Adds a user to a group.
 * @param {string} user - The user name.
 * @param {string} group - The group name.
 * @returns {int} - 1 on success, 0 if group does not exist or
 *                  user is already a member.
 */
public int enableMembership(string user, string group) {
  integrityCheck();

  if(!groups[group])
    return 0;

  if(member_array(user, groups[group]) != -1)
    return 0;

  groups[group] += ({ user });

  return 1;
}

/**
 * @description Removes a user from a group.
 * @param {string} user - The user name.
 * @param {string} group - The group name.
 * @returns {int} - 1 on success, 0 if group does not exist or
 *                  user is not a member.
 */
public int disableMembership(string user, string group) {
  integrityCheck();

  if(!groups[group])
    return 0;

  if(member_array(user, groups[group]) == -1)
    return 0;

  groups[group] -= ({ user });

  return 1;
}

/**
 * @description Toggles a user's membership in a group.
 * @param {string} user - The user name.
 * @param {string} group - The group name.
 * @returns {int} - 1 on success, 0 if group does not exist.
 */
public int toggleMembership(string user, string group) {
  integrityCheck();

  if(!groups[group])
    return 0;

  if(member_array(user, groups[group]) == -1)
    groups[group] += ({ user });
  else
    groups[group] -= ({ user });

  return 1;
}

/**
 * @description Sets access permissions for an identity on a
 *              directory.
 * @param {string} dir - The directory path.
 * @param {string} id - The identity (user or group).
 * @param {string *} akeys - Array of permission keys.
 * @returns {int} - 1 on success, 0 if no keys provided.
 */
public int setAccess(string dir, string id, string *akeys) {
  integrityCheck();

  if(!sizeof(akeys))
    return 0;

  if(!access[dir])
    access[dir] = ([ id : akeys ]);
  else
    access[dir][id] = akeys;

  return 1;
}

/**
 * @description Returns the list of all group names.
 * @returns {string *} - Array of group names.
 */
public string *listGroups() {
  integrityCheck();

  return keys(groups);
}

public void writeState(int flag) {
  integrityCheck();

  if(!adminp(previous_object()) && !adminp(this_body()))
    return;

  writeGroupFile(flag);
  writeAccessFile(flag);
}

private void writeGroupFile(int flag) {
  integrityCheck();

  if(!adminp(previous_object()) && !adminp(this_body()))
    return;

  string *groupList = keys(groups);
  int sz = sizeof(groupList);
  string file = "";

  for(int i = 0; i < sz; i++) {
    string *groupData = groups[groupList[i]];
    int gsz = sizeof(groupData);

    file += "(" + groupList[i] + ")";

    if(gsz > 1)
      file += implode(groupData, ":") + "\n";
    else if(gsz == 1)
      file += groupData[0] + "\n";
    else
      error("ERROR: Group '" + groupList[i]
        + "' has no members!");
  }

  if(flag) {
    tell_me(file);
  } else {
    write_file(FILE_GROUPDATA, file, 1);
    parseFiles();

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

private void writeAccessFile(int flag) {
  integrityCheck();

  if(!adminp(previous_object()) && !adminp(this_body()))
    return;

  string *accessList = keys(access);
  int sz = sizeof(accessList);
  string file = "";

  for(int i = 0; i < sz; i++) {
    mapping accessData = access[accessList[i]];
    string *accessKeys = keys(accessData);
    int ksz = sizeof(accessKeys);
    string *arr = ({});

    file += "(" + accessList[i] + ") ";

    for(int j = 0; j < ksz; j++)
      arr += ({
        sprintf("%s[%s]", accessKeys[j],
          implode(accessData[accessKeys[j]], ""))
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
    parseFiles();

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

private void integrityCheck() {
  if(!clonep())
    error("Error [security_editor]: This object must be "
      "cloned to be used.");
}
