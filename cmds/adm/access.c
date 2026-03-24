// access.c

// Tacitus @ LPUniversity
// 09-OCT-05
// Advanced admin command

// THIS IS A VERY EARLY VERSION AND ISN'T VERY USERFRIENDLY YET!

/* Last edited on July 17th, 2006 by Tacitus */

inherit STD_CMD;

#define FILE_GROUPDATA "/adm/etc/groups"
#define FILE_ACCESSDATA "/adm/etc/access"

mapping access = ([]);
mapping groups = ([]);

void writeGroupFile(int flag);
void writeAccessFile(int flag);
string *parse(string str);
void parseFiles();
void parseGroup();
void parseAccess();

int inputMainMenu(string str, object caller);
int inputModMenu(string str, object caller);
int inputDisMenu(string str, object caller);
void inputContinueDisMenu(string str, object caller);
int inputQueryGroup(string str, object caller);
int inputCreateGroup(string str, object caller);
int inputDeleteGroup(string str, object caller);
int inputToggleMembership(string str, object caller);
int inputToggleMembership2(string str, object caller, string user);
int inputModDirectory(string str, object caller);
int inputModDirGroup(string str, object caller, string dir);
int inputModDirAccessKey(string str, object caller, string dir, string group);
int inputQueryDirectory(string str, object caller);

void setup() {
  parseFiles();
}

mixed main(/** @type {STD_PLAYER} */ object caller, string _arg) {
  if(!adminp(this_interactive()))
    return notify_fail("Error [access]: Access denied.\n");

  tell_me("\n\t--'Security System Interface'--\n"
    "\t\tVersion: 1.3RCDev\n\n");
  tell_me(" 1: Display current access settings\n");
  tell_me(" 2: Modify current access settings\n");
  tell_me(" 3: Reload data from file\n");
  tell_me(" 4: Save & Exit\n");
  tell_me(" 5: Exit without saving\n");
  input_to("inputMainMenu", caller);

  return 1;
}

int inputMainMenu(string str, object caller) {
  if(!str)
    return 1;

  switch(str) {
  case "1":
    tell_me("\n\tDISPLAY CURRENT ACCESS SETTINGS\n\n");
    tell_me(" 1: Display raw data\n");
    tell_me(" 2: List all groups\n");
    tell_me(" 3: List members of a group\n");
    tell_me(" 4: List access for directory\n");
    tell_me(" 5: Main menu\n");
    input_to("inputDisMenu", caller);
    return 1;
  case "2":
    tell_me("\n\tMODIFY CURRENT ACCESS SETTINGS\n\n");
    tell_me(" 1: Create group\n");
    tell_me(" 2: Delete group\n");
    tell_me(" 3: Toggle user membership to group\n");
    tell_me(" 4: Modify access to a directory\n");
    tell_me(" 5: Main menu\n");
    input_to("inputModMenu", caller);
    return 1;
  case "3":
    parseFiles();
    tell_me("\nSuccess: Data reloaded from file -- All unsaved changes lost.\n");
    main(caller, 0);
    return 1;
  case "4":
    tell_me("Attempting to write data to files...\n");
    writeGroupFile(0);
    writeAccessFile(0);
    parseFiles();
    tell_me("Success [access]: Exited with data saved.\n");
    return 1;
  case "5":
    tell_me("Success [access]: Exiting without saving.\n");
    parseFiles();
    return 1;
  default:
    tell_me("Error [access]: Unknown menu " + str + "\n");
    return 1;
  }
}

int inputModMenu(string str, object caller) {
  string *arr;

  arr = ({});

  switch(str) {
  case "1":
    tell_me("Enter the name of the group you wish to create: ");
    input_to("inputCreateGroup", caller);
    return 1;
  case "2":
    tell_me("Enter the name of the group you wish to delete: ");
    input_to("inputDeleteGroup", caller);
    return 1;
  case "3":
    tell_me("Enter the name of the user who you wish to toggle membership: ");
    input_to("inputToggleMembership", caller);
    return 1;
  case "4":
    tell_me("Enter the directory that you wish to modify the access to: ");
    input_to("inputModDirectory", caller);
    return 1;
  default:
    tell_me("\n");
    main(caller, 0);
    return 1;
  }
}

int inputModDirectory(string str, object caller) {
  if(!str) {
    tell_me("\n");
    inputMainMenu("2", caller);
    return 1;
  }

  if(str[<1..<1] != "/")
    str += "/";

  tell_me("\nEnter the name of the group/user whom's access you wish to edit: ");
  input_to("inputModDirGroup", caller, str);
  return 1;
}

int inputModDirGroup(string str, object caller, string dir) {
  if(!str) {
    tell_me("\n");
    inputMainMenu("2", caller);
    return 1;
  } else {
    tell_me("\nEnter the access key: ");
    input_to("inputModDirAccessKey", caller, dir, str);
    return 1;
  }
}

int inputModDirAccessKey(string str, object caller, string dir, string group) {
  string *access_keys = allocate(8);

  if(!str) {
    tell_me("\n");
    inputMainMenu("2", caller);
    return 1;
  }

  // read, write, network, shadow, link, execute, bind, ownership
  if(strsrch(str, "r") != -1) access_keys[0] = "r";
  if(strsrch(str, "w") != -1) access_keys[1] = "w";
  if(strsrch(str, "n") != -1) access_keys[2] = "n";
  if(strsrch(str, "s") != -1) access_keys[3] = "s";
  if(strsrch(str, "l") != -1) access_keys[4] = "l";
  if(strsrch(str, "e") != -1) access_keys[5] = "e";
  if(strsrch(str, "b") != -1) access_keys[6] = "b";
  if(strsrch(str, "o") != -1) access_keys[7] = "o";

  if(!access[dir])
    access += ([dir : ([group : access_keys])]);
  else
    access[dir] += ([group : access_keys]);

  tell_me("\nSuccess: Access to '" + dir + "' for group/user '" + group + "' updated.\n");
  inputMainMenu("2", caller);
  return 1;
}

int inputToggleMembership(string str, object caller) {
  if(!str) {
    tell_me("\n");
    inputMainMenu("2", caller);
    return 1;
  }

  if(!user_data_file(str)) {
    tell_me("\nError: User '" + str + "' does not exist.\n");
    inputMainMenu("2", caller);
    return 1;
  } else {
    tell_me("Enter the name of the group that you wish to toggle " + capitalize(str) + "'s membership to: ");
    input_to("inputToggleMembership2", caller, str);
    return 1;
  }
}

int inputToggleMembership2(string str, object caller, string user) {
  string *user_list;

  if(!str) {
    tell_me("\n");
    inputMainMenu("2", caller);
    return 1;
  }

  user_list = groups[str];

  if(sizeof(user_list) == 0) {
    tell_me("\nError:  Group '" + str + "' does not exist.\n");
    inputMainMenu("2", caller);
    return 1;
  }

  if(member_array(user, user_list) == -1) {
    groups[str] += ({user});
    tell_me("\nSuccess: User '" + user + "' was added to group '" + str + "'\n");
    inputMainMenu("2", caller);
    return 1;
  } else {
    groups[str] -= ({user});
    tell_me("\nSuccess: User '" + user + "' was removed from the group '" + str + "'\n");
    inputMainMenu("2", caller);
    return 1;
  }
}

int inputCreateGroup(string str, object caller) {
  if(!str) {
    tell_me("\n");
    inputMainMenu("2", caller);
    return 1;
  }

  if(groups[str]) {
    tell_me("\nError:  a group with the name '" + str + "' already exists.\n");
    inputMainMenu("2", caller);
    return 1;
  } else {
    groups[str] = ({query_privs(this_body())});
    tell_me("\nSuccess:  group '" + str + "' created. Note: You were added to the new group.\n");
    inputMainMenu("2", caller);
    return 1;
  }
}

int inputDeleteGroup(string str, object caller) {
  if(!str) {
    tell_me("\n");
    inputMainMenu("2", caller);
    return 1;
  }

  if(!groups[str]) {
    tell_me("\nError:  group '" + str + "' does not exist.\n");
    inputMainMenu("2", caller);
    return 1;
  }

  if(str == "admin" || str == "soul") {
    tell_me("\nError:  group '" + str + "' cannot be deleted.\n\t"
      + "The group is a system group and is required for proper functionality of the mudlib.\n");
    inputMainMenu("2", caller);
    return 1;
  } else {
    map_delete(groups, str);
    tell_me("Success:  The group '" + str + "' was deleted.\n");
    inputMainMenu("2", caller);
    return 1;
  }
}

void inputContinueDisMenu(string _str, object caller) {
  inputMainMenu("1", caller);
}

int inputDisMenu(string str, object caller) {
  string *arr;

  arr = ({});

  switch(str) {
  case "1":
    tell_me("\nRaw group data:\n\n");
    writeGroupFile(1);
    tell_me("\nRaw access data:\n\n");
    writeAccessFile(1);
    tell_me("\n[Hit enter to continue]");
    input_to("inputContinueDisMenu", caller);
    return 1;
  case "2":
    arr = keys(groups);
    if(!sizeof(arr))
      arr = ({"[None]"});
    tell_me("\nThere is a total of " + sizeof(arr) + " groups on " + mud_name() + "\n\n");
    tell_me(simple_list(arr));
    tell_me("\n[Hit enter to continue]");
    input_to("inputContinueDisMenu", caller);
    return 1;
  case "3":
    tell_me("\n Enter name of group: ");
    input_to("inputQueryGroup", caller);
    return 1;
  case "4":
    tell_me("\n Enter name of directory: ");
    input_to("inputQueryDirectory", caller);
    return 1;
  default:
    tell_me("\n");
    main(caller, 0);
    return 1;
  }
}

int inputQueryDirectory(string str, object caller) {
  string *access_key_list, *arr;
  string output;
  mapping access_data;
  int j;

  arr = ({});
  output = "";

  if(!str) {
    tell_me("\n");
    inputMainMenu("1", caller);
    return 1;
  }

  if(!mapp(access[str])) {
    tell_me("\nError: No specific access settings have been set for directory '" + str + "'\n");
    tell_me("\n[Hit enter to continue]");
    input_to("inputContinueDisMenu", caller);
    return 1;
  }

  access_data = access[str];
  output += "(" + str + ") ";
  access_key_list = keys(access_data);
  arr = ({});
  for(j = 0; j < sizeof(access_key_list); j++)
    arr += ({(sprintf("%s[%s]", access_key_list[j], implode(access_data[access_key_list[j]], "")))});
  if(sizeof(arr) > 1)
    output += sprintf("%s%s\n", implode(arr[0..(sizeof(arr) - 2)], ":"), ":" + arr[sizeof(arr) - 1]);
  else
    output += arr[0] + "\n";
  tell_me("\n" + output);
  tell_me("\n[Hit enter to continue]");
  input_to("inputContinueDisMenu", caller);
  return 1;
}

int inputQueryGroup(string str, object caller) {
  string *arr;

  if(!str) {
    tell_me("\n");
    inputMainMenu("1", caller);
    return 1;
  }

  if(!sizeof(groups[str])) {
    tell_me("\nError: Group '" + str + "' doesn't exist.\n");
    tell_me("\n[Hit enter to continue]");
    input_to("inputContinueDisMenu", caller);
    return 1;
  }

  arr = groups[str];
  if(sizeof(arr) > 1)
    printf("\nThe following users are a member of the group '" + str + "':\n\t%s, and %s\n", implode(arr[0..(sizeof(arr) - 2)], ", "), arr[sizeof(arr) - 1]);
  else
    printf("\nThe following user is a member of the group '" + str + "':\n\t%s\n", arr[0]);
  tell_me("\n[Hit enter to continue]");
  input_to("inputContinueDisMenu", caller);
  return 1;
}

void writeGroupFile(int flag) {
  string file;
  string *group_list, *group_data;
  int i;

  file = "";
  i = 0;
  group_list = keys(groups);
  group_data = ({});

  for(i = 0; i < sizeof(group_list); i++) {
    group_data = groups[group_list[i]];
    file += "(" + group_list[i] + ")";
    if(sizeof(group_data) > 1)
      file += sprintf("%s%s\n", implode(group_data[0..(sizeof(group_data) - 2)], ":"), ":" + group_data[sizeof(group_data) - 1]);
    else if(sizeof(group_data) == 1)
      file += group_data[0] + "\n";
    else
      error("ERROR: Group '" + group_list[i] + "' has no members!");
  }

  if(flag)
    tell_me(file + "\n");
  else
    parseFiles();
}

void writeAccessFile(int flag) {
  string file;
  string *access_list, *access_key_list, *arr;
  mapping access_data;
  int i, j;

  file = "";
  i = 0;
  j = 0;
  access_data = ([]);
  access_list = keys(access);
  access_key_list = ({});
  arr = ({});

  for(i = 0; i < sizeof(access_list); i++) {
    access_data = access[access_list[i]];
    file += "(" + access_list[i] + ") ";
    access_key_list = keys(access_data);
    arr = ({});
    for(j = 0; j < sizeof(access_key_list); j++)
      arr += ({(sprintf("%s[%s]", access_key_list[j], implode(access_data[access_key_list[j]], "")))});
    if(sizeof(arr) > 1)
      file += sprintf("%s%s\n", implode(arr[0..(sizeof(arr) - 2)], ":"), ":" + arr[sizeof(arr) - 1]);
    else
      file += arr[0] + "\n";
  }

  if(flag)
    tell_me(file + "\n");
  else
    parseFiles();
}

string *parse(string str) {
  string *arr;
  int i;

  if(!str)
    return ({});

  arr = explode(str, "\n");

  for(i = 0; i < sizeof(arr); i++) {
    if(arr[i][0] == '#') {
      arr[i] = 0;
      continue;
    }
    arr[i] = replace_string(arr[i], " ", "");
    arr[i] = replace_string(arr[i], "\t", "");
  }

  return arr;
}

void parseFiles() {
  parseGroup();
  parseAccess();
}

void parseGroup() {
  int i, n;
  string *arr;

  arr = parse(read_file(FILE_GROUPDATA));

#ifdef DEBUG
  write_file("/log/security", "\tDebug [security]: Parsing group data file...\n");
#endif

  groups = ([]);

  for(i = 0; i < sizeof(arr); i++) {
    string group, str, *members;

    if(!arr[i])
      continue;

    if(sscanf(arr[i], "(%s)%s", group, str) != 2) {
      tell_me("Error [security]: Invalid format of data in group data.\n");
      tell_me("Security alert: Ignoring group on line " + (i + 1) + "\n");
      continue;
    }

    members = explode(str, ":");

#ifdef DEBUG
    write_file("/log/security", "Debug [security]: Adding group '" + group + "' with " + sizeof(members) + " members.\n");
#endif

    for(n = 0; n < sizeof(members); n++) {
      if(!file_size(user_data_file(members[n])) && !sscanf(members[n], "[%*s]")) {
        tell_me("Error [security]: Unknown user detected.\n");
        tell_me("Security alert: User '" + members[n] + "' ignored for group '" + group + "'.\n");
        members -= ({members[n]});
        continue;
      }
#ifdef DEBUG
      write_file("/log/security", "Debug [security]: Adding user '" + members[n] + "' to group '" + group + "'.\n");
#endif
    }

    groups += ([group : members]);
  }
}

void parseAccess() {
  int i, n;
  string *arr;

  arr = parse(read_file(FILE_ACCESSDATA));

#ifdef DEBUG
  write_file("/log/security", "\tDebug [security]: Parsing access data file...\n");
#endif

  access = ([]);

  for(i = 0; i < sizeof(arr); i++) {
    string directory, str, *entries;
    mapping data;

    data = ([]);

    if(!arr[i])
      continue;

    if(sscanf(arr[i], "(%s)%s", directory, str) != 2) {
      tell_me("Error [security]: Invalid format of data in access data.\n");
      error("Security alert: Fatal error parsing access data on line " + (i + 1) + "\n");
    }

    if(str[<1..<1] == ":") {
      tell_me("Error [security]: Incomplete data in access data (trailing ':').\n");
      error("Security alert: Fatal error parsing access data on line " + (i + 1) + "\n");
    }

    entries = explode(str, ":");

#ifdef DEBUG
    write_file("/log/security", "Debug [security]: Parsing data for directory '" + directory + "'.\n");
#endif

    for(n = 0; n < sizeof(entries); n++) {
      string identity, permissions, *perm_array;

      perm_array = allocate(8);

      if(sscanf(entries[n], "%s[%s]", identity, permissions) != 2) {
        tell_me("Error [security]: Invalid entry(" + n + ") data format in access data.\n");
        error("Security alert: Fatal error parsing access data on line " + (i + 1) + "\n");
      }

#ifdef DEBUG
      write_file("/log/security", "Debug [security]: Adding identity '" + identity + "' with permission string of '" + permissions + "'.\n");
#endif

      // read, write, network, shadow, link, execute, bind, ownership
      if(strsrch(permissions, "r") != -1) perm_array[0] = "r";
      if(strsrch(permissions, "w") != -1) perm_array[1] = "w";
      if(strsrch(permissions, "n") != -1) perm_array[2] = "n";
      if(strsrch(permissions, "s") != -1) perm_array[3] = "s";
      if(strsrch(permissions, "l") != -1) perm_array[4] = "l";
      if(strsrch(permissions, "e") != -1) perm_array[5] = "e";
      if(strsrch(permissions, "b") != -1) perm_array[6] = "b";
      if(strsrch(permissions, "o") != -1) perm_array[7] = "o";

      data += ([identity : perm_array]);
    }

    access += ([directory : data]);
  }
}
