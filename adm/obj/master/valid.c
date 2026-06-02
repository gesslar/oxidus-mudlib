/* valid.c

 Tacitus @ LPUniversity
 23-APR-05
 Valid hooks, inherited by Master Obj

 Refractored 21-DEC-05 by Tacitus @ LPUni

*/

/* Last edited on July 17th, 2006 by Tacitus */

/* Preprocessor Statements */

#define FILE_GROUPDATA "/adm/etc/groups"
#define FILE_ACCESSDATA "/adm/etc/access"
//#define DEBUG

/* Global Variable */

mapping access = ([]);
mapping groups = ([]);

/* Function prototypes */

void parse_group();
void parse_access();
string *parse(string *str);
string *query_group(string group);
string *expand_group(string group, mapping seen);
string *track_member(string id, string directory);
int query_access(string directory, string id, int type);
int is_member(string user, string group);

/* Functions */

void parse_group() {
    int i, n;
    string *arr = parse(explode_file(FILE_GROUPDATA));
    int sz_arr;

#ifdef DEBUG

    write_file("/log/security", "\tDebug [security]: Parsing group data file...\n");

#endif

    groups = ([]);

    for(i = 0, sz_arr = sizeof(arr); i < sz_arr; i++) {
        string group, str, *members;
        int sz_members;

        if(!arr[i]) continue;

        if(sscanf(arr[i], "(%s)%s", group, str) != 2) {
            tell_me("Error [security]: Invalid format of data in group data.\n");
            tell_me("Security alert: Ignoring group on line " + (i + 1) + "\n");
            continue;
        }

        members = explode(str, ":");

#ifdef DEBUG

        write_file("/log/security", "Debug [security]: Adding group '" + group + "' with " + sizeof(members) + " members.\n");

#endif

        for(n = 0, sz_members = sizeof(members); n < sz_members; n++) {
            // A member may be a real user, a [special] priv, or a
            // (group) reference for nesting one group inside another.
            if(!file_size(user_data_file(members[n]))
            && !sscanf(members[n], "[%*s]")
            && !sscanf(members[n], "(%*s)")) {
                tell_me("Error [security]: Unknown user detected.\n");
                tell_me("Security alert: User '" + members[n] + "' ignored for group '" + group + "'.\n");
                members -= ({ members[n] });
                continue;
            }
#ifdef DEBUG

            write_file("/log/security", "Debug [security]: Adding user '" + members[n] + "' to group '" + group + "'.\n");

#endif
        }

        groups += ([group : members]);
    }
}

void parse_access() {
    int i, n;
    string *arr = parse(explode_file(FILE_ACCESSDATA));
    int sz_arr;

#ifdef DEBUG

    write_file("/log/security", "\tDebug [security]: Parsing access data file...\n");

#endif

    access = ([]);

    for(i = 0, sz_arr = sizeof(arr); i < sz_arr; i++) {
        string directory, str, *entries;
        mapping data;
        int sz_entries;

        data = ([]);

        if(!arr[i]) continue;

        if(sscanf(arr[i], "(%s)%s", directory, str) != 2) {
            tell_me("Error [security]: Invalid format of data in access data.\n");
            error("Security alert: Fatal error parsing access data on line " + (i + 1) + "\n");
        }

        if(str[<1..< 1] == ":") {
            tell_me("Error [security]: Incomplete data in access data (trailing ':').\n");
            error("Security alert: Fatal error parsing access data on line " + (i + 1) + "\n");
        }

        entries = explode(str, ":");

#ifdef DEBUG

        write_file("/log/security", "Debug [security]: Parsing data for directory '" + directory + "'.\n");

#endif

        for(n = 0, sz_entries = sizeof(entries); n < sz_entries; n++) {
            string identity, permissions, *perm_array = allocate(8);
            if(sscanf(entries[n], "%s[%s]", identity, permissions) != 2) {
                tell_me("Error [security]: Invalid entry(" + n + ") data format in access data.\n");
                error("Security alert: Fatal error parsing access data on line " + (i + 1) + "\n");
            }

#ifdef DEBUG

            write_file("/log/security", "Debug [security]: Adding identity '" + identity + "' with permission string of '" + permissions + "'.\n");

#endif

            //read, write, network, shadow, link, execute, bind, ownership
            if(strsrch(permissions, "r") != -1) perm_array[0] = "r";
            if(strsrch(permissions, "w") != -1) perm_array[1] = "w";
            if(strsrch(permissions, "n") != -1) perm_array[2] = "n";
            if(strsrch(permissions, "s") != -1) perm_array[3] = "s";
            if(strsrch(permissions, "l") != -1) perm_array[4] = "l";
            if(strsrch(permissions, "e") != -1) perm_array[5] = "e";
            if(strsrch(permissions, "b") != -1) perm_array[6] = "b";
            if(strsrch(permissions, "o") != -1) perm_array[7] = "o";

            data += ([ identity : perm_array ]);
        }

        access += ([directory : data]);
    }
}

string *parse(string *arr) {
    if(!sizeof(arr))
        return ({});

    arr = map(arr, (: replace_string($1, " ", "") :));
    arr = map(arr, (: replace_string($1, "\t", "") :));

    return arr;
}

int valid_shadow(object ob) {
    string location, name;
    location = base_name(ob);
    name = query_privs(ob);

    if(ob == this_object() || ob == master())
      return 0;

    if(query_access(location, name, 4) && !call_if(ob, "disallow_shadow", ob))
      return 1;

    return 0;
}

int valid_bind(object _obj, object owner, object victim) {
    string name;
    name = query_privs(previous_object());
    if(query_access(base_name(owner), name, 7) && query_access(base_name(victim), name, 7)) return 1;
    return 0;
}

int valid_compile_to_c() {
    return 0;
}

int valid_hide(object _ob) {
    return 0;
}

int valid_link(string from, string to) {
    string name;

    if(this_interactive()) name = query_privs(this_interactive());
    else name = query_privs(previous_object());
    if(query_access(from, name, 5) && query_access(to, name, 5))
        return 1;
    return 0;
}

int valid_object(object ob) {
    string location, name;

    location = file_name(ob);
    if(this_interactive()) name = query_privs(this_interactive());
    else name = query_privs(previous_object());
    if(!name) name = "noname";
    if(file_name(ob) == "/adm/daemons/login") return 1;
    if(query_access(location, name, 6)) return 1;
    return 0;
}

int valid_override(string _file, string efun_name, string mainfile) {
  if(mainfile == "/adm/obj/simul_efun.c") return 1;
  if(mainfile == "/adm/simul_efun/override.c") return 1;
  if(mainfile == "/adm/simul_efun/overrides.c") return 1;
  if(efun_name == "this_player" && mainfile == "/adm/simul_efun/object.c") return 1;
  if(efun_name == "destruct" && mainfile == "/std/object/object.c") return 1;
  if(efun_name == "ed") return 1;
  return 0;
}

int valid_socket(object _caller, string _func, mixed *_info) {
    //We might code a daemon or something that allows
    //us to ban connections to certain ports/connections

    /* string name;
    name = query_privs(caller);
    if(query_access(base_name(info[1]), name, 3)) return 1;
    return 0;  */

    return 1;
}

int valid_read(string file, object user, string func) {
    string name, tmp, tmp2;

    if(this_interactive() && query_privs(user) != "[daemon]")
        name = query_privs(this_interactive());

    else name = query_privs(user);
    if(!name) name = "noname";

    if(strlen(file) > strlen(user_data_directory(name))) {
        if(file[0..(strlen(user_data_directory(name))-1)] == user_data_directory(name))
            return 1;
    }

    if(func == "file_size") return 1;
    if(func == "restore_object" && member_array(find_object("/adm/daemons/finger_d.c"), all_previous_objects()) != -1) return 1;

    if(file && sscanf(file, "/home/%*s/%s/%s", tmp, tmp2)) {
        if(name == tmp || name == "[home_" + tmp + "]") return 1;
        if(tmp2 && tmp2[0..5] == "public") return 1;
        if(tmp2 && tmp2[0..6] == "private" && name == tmp) return 1;
        if(tmp2 && tmp2[0..6] == "private" && name != tmp && !is_member(name, "admin")) return 0;
    }

#ifdef DEBUG
    write_file("/log/security", "Debug [valid_read]: File: " + file + " Name: " + name + "\n");
#endif

    if(query_access(file, name, 1)) return 1;
    return 0;
}

int valid_write(string file, object user, string _func) {
    string name, tmp, tmp2;
    if(this_interactive() && query_privs(user) != "[daemon]")
    name = query_privs(this_interactive());
    else name = query_privs(user);
    if(!name) name = "noname";

    if(user == this_object() || user == master()) return 1;

    if(strlen(file) > strlen(user_data_directory(name))) {
        if(file[0..(strlen(user_data_directory(name))-1)] == user_data_directory(name)) return 1;
    }


    if(file && sscanf(file, "/home/%*s/%s/%s", tmp, tmp2)) {
        if(name == tmp || name == "[home_" + tmp + "]") return 1;
        if(tmp2 && tmp2[0..6] == "public/" && tmp2 != "public/") return 1;
        if(tmp2 && tmp2[0..6] == "private" && name == tmp) return 1;
    }


#ifdef DEBUG
    write_file("/log/security", "Debug [valid_write]: File: " + file + " Name: " + name + "\n");
#endif
    if(query_access(file, name, 2)) return 1;
    return 0;
}

int query_access(string directory, string id, int type) {
    mapping data;
    string *permissions;

#ifdef DEBUG

    write_file("/log/security", "Debug [security]: Permission query for '" + id + "' in '" + directory + "'.\n");
#endif

    data = access[directory];

    if(!mapp(data)) {
        string *exp;
        int size, i;

#ifdef DEBUG
        write_file("/log/security", "Debug [query_access]: " + directory + " not in mapping. Searching for parent permissions.\n");
#endif
        exp = explode(directory, "/");
        size = sizeof(exp);
        while(!access[directory] && size --) {
            directory = "/";
            for(i = 0; i < size; i ++) {
                directory += exp[i] + "/";
            }
        }

        if(!access[directory]) directory = "/";
    }

    if(!access[directory]) return 0;
    else data = access[directory];

#ifdef DEBUG

    write_file("/log/security", "Debug [query_access]: Final directory set to '" + directory + "'.\n");

#endif


    permissions = data[id];
    if(!permissions || !pointerp(permissions) || sizeof(permissions) < 1) permissions = track_member(id, directory);

    if(sizeof(permissions) < 1) {

#ifdef DEBUG

        write_file("/log/security", "Debug [security]: Using defalt permissions for '" + id + "' in '" + directory + "'.\n");

#endif
        permissions = data["(all)"];
    }


    if(sizeof(permissions) < 1 || !pointerp(permissions)) {

#ifdef DEBUG

        write_file("/log/security", "Debug [security]: No permissions found for '" + id + "' in '" + directory + "'.\n");

#endif
        return 0;
    }

    switch(type) {
    //read, write, network, shadow, link, execute, bind, ownership
        case 1 :
            if(member_array("r", permissions) != -1) {
#ifdef DEBUG
                write_file("/log/security", "Debug [query_access]: Permission granted to read for " + directory + "\n");
#endif
                return 1;
            }
            break;
        case 2 :
            if(member_array("w", permissions) != -1) {
#ifdef DEBUG
                write_file("/log/security", "Debug [query_access]: Permission granted to write for " + directory + "\n");
#endif
                return 1;
            }
            break;
        case 3 :
            if(member_array("n", permissions) != -1) {
#ifdef DEBUG
                write_file("/log/security", "Debug [query_access]: Permission granted to access network for " + directory + "\n");
#endif
                return 1;
            }
            break;
        case 4 :
            if(member_array("s", permissions) != -1) {
#ifdef DEBUG
                write_file("/log/security", "Debug [query_access]: Permission granted to shadow for " + directory + "\n");
#endif
                return 1;
            }
            break;
        case 5 :
            if(member_array("l", permissions) != -1) {
#ifdef DEBUG
                write_file("/log/security", "Debug [query_access]: Permission granted to link for " + directory + "\n");
#endif
                return 1;
            }
            break;
        case 6 :
            if(member_array("e", permissions) != -1) {
#ifdef DEBUG
                write_file("/log/security", "Debug [query_access]: Permission granted to execute for " + directory + "\n");
#endif
                return 1;
            }
            break;
        case 7 :
            if(member_array("b", permissions) != -1) {
#ifdef DEBUG
                write_file("/log/security", "Debug [query_access]: Permission granted to bind for " + directory + "\n");
#endif
                return 1;
            }
            break;
        case 8 :
            if(member_array("o", permissions) != -1) {
#ifdef DEBUG
                write_file("/log/security", "Debug [query_access]: Permission granted as ownership for " + directory + "\n");
#endif
                return 1;
            }
            break;
    }
#ifdef DEBUG
    write_file("/log/security", "Debug [query_access]: Permission denied (" + type + ").\n");
#endif
    return 0;
}

/*
 * Recursively resolves a group into the flat list of its concrete
 * member ids (real users and [special] privs), following any nested
 * (group) references. The seen mapping guards against cyclic group
 * definitions so a misconfigured file can't loop forever.
 */
string *expand_group(string group, mapping seen) {
    string *members, *result;
    int i, sz;

    // Normalise a "(group)" reference down to its bare group name.
    if(!groups[group]) sscanf(group, "(%s)", group);

    if(!group || seen[group]) return ({});
    seen[group] = 1;

    members = groups[group];
    if(!pointerp(members)) return ({});

    result = ({});
    for(i = 0, sz = sizeof(members); i < sz; i++) {
        string sub;

        if(stringp(members[i]) && sscanf(members[i], "(%s)", sub))
            result += expand_group(members[i], seen);
        else
            result += ({ members[i] });
    }

    return result;
}

string *track_member(string id, string directory) {
    mapping data = access[directory];
    string *cles = keys(data);
    string *group_data = ({});
    int i;
    int sz_keys;

#ifdef DEBUG

    write_file("/log/security", "Debug [security]: Tracking member '" + id + "' for '" + directory + "'.\n");

#endif

    for(i = 0, sz_keys = sizeof(cles); i < sz_keys; i++) {
        group_data = expand_group(cles[i], ([]));
        if(!pointerp(group_data) || sizeof(group_data) < 1) continue;
        if(member_array(id, group_data) != -1) return data[cles[i]];
    }

    return ({});
}

string *query_group(string group) {
    if(!groups[group]) sscanf(group, "(%s)", group);
    return groups[group];
}

mapping query_groups() {
    return copy(groups);
}

string *query_group_names() {
    return keys(groups);
}

int is_member(string user, string group) {
    if(!user || !group) return 0;

    return member_array(user, expand_group(group, ([]))) != -1;
}

mixed valid_database(object _caller, string _fun, mixed *_info) {
    // We are using SQLITE3, so just return 1.
    return 1;
}
