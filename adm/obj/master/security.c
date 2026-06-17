/**
 * @file /adm/obj/master/security.c
 *
 * Security subsystem inherited by the master object. Implements the
 * role/group model and (eventually) the driver validation applies.
 *
 * The model has three data sources:
 *
 *   - groups_base.lpml  - shipped base group compositions. Each group
 *                         confers a set of roles to its members. This
 *                         is where the owner > admin > developer
 *                         hierarchy is expressed (owner confers all
 *                         three roles, admin two, developer one) and
 *                         where the structural code-object identities
 *                         ([master], [daemon], ...) are listed.
 *   - groups.lpml       - per-MUD overrides. The MUD owner edits this
 *                         to assign real users to groups and to tweak
 *                         a group's roles. Entries are merged over the
 *                         base; a "-name" entry removes, a plain entry
 *                         adds, and removals take precedence.
 *   - roles.map         - per-MUD direct role grants, written by
 *                         add_role()/remove_role(). A directly granted
 *                         role that names a group expands to that
 *                         group's conferred roles, so granting "owner"
 *                         confers the whole hierarchy without group
 *                         membership.
 *
 * is_member() answers strict group membership (the forgiving "are
 * they an admin?" gate). has_role() answers a user's effective roles -
 * group-conferred plus direct - and is the precise "this operation
 * requires this role" gate.
 *
 * @created 2005-04-23 - Tacitus
 * @last_modified 2026-06-16 - Gesslar
 *
 * @history
 * 2005-04-23 - Tacitus - Created as valid.c
 * 2026-06-16 - Gesslar - Rebuilt as the role/group security model
 */

/* Function prototypes */

protected nomask void setup_security();
public nomask string *get_roles(string name);
public nomask int has_role(string name, string role);
public nomask int is_member(string user, string group);
public nomask string *add_role(string name, string role);
public nomask string *remove_role(string name, string role);
public nomask void purge_roles(string name);
public nomask string *query_group(string group);
public nomask mapping query_groups();
public nomask string *query_group_names();
private nomask mapping load_roles();
private nomask string *merge_list(string *base_list, string *custom_list);
private nomask string *group_roles(string group, mapping seen);
private nomask void restore_roles();

/* Global variables */

/**
 * The merged group definitions. Maps a group name to a record of the
 * groups it inherits, the roles it confers in its own right, and the
 * identities that are its members. A group's effective roles are its
 * own roles plus, transitively, the roles of every group it inherits
 * (so owner inherits admin inherits developer). Members may be real
 * user names or bracketed code-object privs (e.g. "[cmd_admin]").
 *
 * @type {([ string: ([ "inherits": string*, "roles": string*, "members": string* ]) ])}
 */
private nomask nosave mapping security_groups = ([]);

/**
 * In-memory authoritative copy of the direct role grants (roles.map),
 * loaded at setup and written through on every change. Permission
 * checks read this, never the file - a check that read the file would
 * trigger a permission check on that read and recurse.
 *
 * @type {([ string: string* ])}
 */
private nomask nosave mapping security_roles = ([]);

private nomask nosave string GROUPS_FILE = "adm/etc/security/groups_base.lpml";
private nomask nosave string GROUPS_FILE_CUSTOM = "adm/etc/security/groups.lpml";
private nomask nosave string ROLES_FILE_CUSTOM = "adm/etc/security/roles.map";

/* Functions */

/**
 * Loads and merges the group definitions. The shipped base
 * (groups_base.lpml) is loaded first, then the per-MUD overrides
 * (groups.lpml) are merged over it. Role and member names are
 * normalised to lower case so lookups are case-insensitive.
 *
 * Called by the master object at boot and whenever the security
 * object is reloaded.
 *
 * @returns {void}
 */
protected nomask void setup_security() {
  mapping custom;

  security_groups = ([]);

  catch(security_groups = load_lpml(GROUPS_FILE));

  if(!mapp(security_groups))
    security_groups = ([]);

  catch(custom = load_lpml(GROUPS_FILE_CUSTOM));

  if(mapp(custom)) {
    foreach(string group, mapping data in custom) {
      if(!mapp(data))
        continue;

      if(!mapp(security_groups[group]))
        security_groups[group] = ([]);

      security_groups[group]["inherits"] =
        merge_list(security_groups[group]["inherits"], data["inherits"]);
      security_groups[group]["roles"] =
        merge_list(security_groups[group]["roles"], data["roles"]);
      security_groups[group]["members"] =
        merge_list(security_groups[group]["members"], data["members"]);
    }
  }

  // Normalise: guarantee the arrays exist and are lower-cased.
  foreach(string _group, mapping data in security_groups) {
    if(!pointerp(data["inherits"]))
      data["inherits"] = ({});

    if(!pointerp(data["roles"]))
      data["roles"] = ({});

    if(!pointerp(data["members"]))
      data["members"] = ({});

    data["inherits"] = map(data["inherits"], (: lower_case($1) :));
    data["roles"] = map(data["roles"], (: lower_case($1) :));
    data["members"] = map(data["members"], (: lower_case($1) :));
  }

  // Restore the persisted role grants into memory, deferred until after
  // master creation completes: the load runs through the cache daemon
  // and file layer, which during create() would try to compile std/*
  // objects before the master can serve their include paths.
  call_out_walltime((: restore_roles :), 0.01);
}

/**
 * Merges a custom override list over a base list. A plain entry is
 * added (if not already present); a "-name" entry removes that name.
 * Additions are applied before removals so a removal always wins over
 * an addition of the same name in the same override. A leading "--"
 * is an escaped no-op and is skipped.
 *
 * @param {string*} base_list - The base list to merge over.
 * @param {string*} custom_list - The override entries.
 * @returns {string*} The merged list.
 */
private nomask string *merge_list(string *base_list, string *custom_list) {
  string *result = pointerp(base_list) ? copy(base_list) : ({});

  if(!pointerp(custom_list))
    return result;

  custom_list = filter(custom_list, (: stringp($1) && truthy($1) :));

  // Sorting descending places plain entries (alphabetic) ahead of
  // "-" removals, so additions are processed first.
  foreach(string entry in sort_array(custom_list, -1)) {
    if(entry[0] == '-') {
      if(strlen(entry) > 1 && entry[1] == '-')
        continue;

      // TODO: collapse once https://github.com/fluffos/fluffos/pull/1207
      // is merged.
      string *tmp = result;
      eject_value_all(ref tmp, entry[1..]);
      result = tmp;
    } else if(!includes(result, entry)) {
      string *tmp = result;
      push(ref tmp, entry);
      result = tmp;
    }
  }

  return result;
}

/**
 * Resolves a group's effective roles: its own roles plus, transitively,
 * the roles of every group it inherits. The seen mapping guards against
 * inheritance cycles so a misconfigured chain cannot loop forever.
 *
 * @param {string} group - The group name to resolve.
 * @param {mapping} seen - Tracks visited groups to prevent cycles.
 * @returns {string*} The group's transitive role set.
 */
private nomask string *group_roles(string group, mapping seen) {
  if(!stringp(group) || seen[group])
    return ({});

  mapping data = security_groups[group];

  if(!mapp(data))
    return ({});

  seen[group] = 1;

  string *roles = pointerp(data["roles"]) ? copy(data["roles"]) : ({});

  if(pointerp(data["inherits"])) {
    foreach(string parent in data["inherits"])
      roles += group_roles(parent, seen);
  }

  return roles;
}

/**
 * Returns the effective roles of an identity: the roles conferred by
 * every group it is a member of, plus its direct grants from
 * roles.map. A direct grant that names a group is expanded to that
 * group's conferred roles, so a direct "owner" grant pulls in the
 * whole owner > admin > developer hierarchy.
 *
 * @param {string} name - The user name or code-object priv.
 * @returns {string*} The distinct list of effective role names.
 */
public nomask string *get_roles(string name) {
  if(!stringp(name) || !truthy(name))
    return ({});

  name = lower_case(name);

  string *direct = security_roles[name];
  string *roles = ({});

  if(pointerp(direct)) {
    foreach(string role in direct) {
      if(!stringp(role))
        continue;

      role = lower_case(role);
      roles += ({ role });

      // A grant naming a group confers that group's full inherited
      // role set, so granting "owner" pulls in admin and developer.
      if(mapp(security_groups[role]))
        roles += group_roles(role, ([]));
    }
  }

  foreach(string group, mapping data in security_groups) {
    if(!mapp(data) || !pointerp(data["members"]))
      continue;

    if(includes(data["members"], name))
      roles += group_roles(group, ([]));
  }

  return distinct_array(roles);
}

/**
 * Tests whether an identity has a specific role among its effective
 * roles. This is the precise access gate - use it when an operation
 * requires a particular role regardless of which group confers it.
 *
 * @param {string} name - The user name or code-object priv.
 * @param {string} role - The role to test for.
 * @returns {int} 1 if the identity has the role, 0 otherwise.
 */
public nomask int has_role(string name, string role) {
  if(!stringp(role) || !truthy(role))
    return 0;

  return includes(get_roles(name), lower_case(role));
}

/**
 * Tests whether an identity is a member of a group. This is strict
 * membership - it does not consider roles. It is the forgiving access
 * gate: the MUD owner curates membership knowing which roles each
 * group confers.
 *
 * @param {string} user - The user name or code-object priv.
 * @param {string} group - The group name.
 * @returns {int} 1 if the identity is a member of the group, 0
 *                otherwise.
 */
public nomask int is_member(string user, string group) {
  if(!stringp(user) || !stringp(group))
    return 0;

  user = lower_case(user);

  mapping data = security_groups[group];

  if(!mapp(data) || !pointerp(data["members"]))
    return 0;

  return includes(data["members"], user);
}

/* The role-mutation API (add_role/remove_role/purge_roles) below is
 * public so it can be reached via master()->, but it does not yet gate
 * on the caller. Authorization currently lives in the command files
 * (adminp(previous_object()) in makeadmin.c and friends), so a rogue
 * object holding a master() reference could call these directly and
 * self-grant. Intentionally open during the role/group build-out.
 *
 * @TODO Enforce caller authorization at this boundary (e.g. require
 *       previous_object() to be trusted/admin) as part of the
 *       path-access layer.
 */

/**
 * Grants a role to a user, writing it to roles.map. Idempotent - an
 * already-granted role is a no-op.
 *
 * @param {string} name - The user name to grant the role to.
 * @param {string} role - The role to grant.
 * @returns {string*} A copy of the user's direct role list after the
 *                    grant.
 * @errors If name or role is not a non-empty string.
 * @errors If role contains anything but alphanumerics and
 *         underscores, or is the reserved name "all".
 * @errors If the user's existing roles are malformed.
 */
public nomask string *add_role(string name, string role) {
  assert(stringp(name) && truthy(name), "Name must be a non-empty string.");
  assert(stringp(role) && truthy(role), "Role must be a non-empty string.");

  name = lower_case(name);
  role = lower_case(role);

  assert(pcre_match(role, "^\\w+$"), "Role may only contain alphanum and underscores.");
  assert(role != "all", "All is not assignable.");

  string *user_roles = security_roles[name] ? copy(security_roles[name]) : ({});

  assert(uniformp(user_roles, T_STRING), "Roles for '"+name+"' is not an array of strings.");
  assert(every(user_roles, (: truthy :)), "Roles for '"+name+"' is not an array of non-empty strings.");

  if(includes(user_roles, role))
    return copy(user_roles);

  push(ref user_roles, role);

  security_roles[name] = user_roles;

  write_file(ROLES_FILE_CUSTOM, pretty_map(security_roles), 1);

  return copy(user_roles);
}

/**
 * Revokes a role from a user, writing the change to roles.map. If the
 * user has no roles left afterwards, their entry is removed entirely.
 *
 * @param {string} name - The user name to revoke the role from.
 * @param {string} role - The role to revoke.
 * @returns {string*} A copy of the user's direct role list after the
 *                    revocation.
 * @errors If name or role is not a non-empty string.
 * @errors If the user's existing roles are malformed.
 */
public nomask string *remove_role(string name, string role) {
  assert(stringp(name) && truthy(name), "Name must be a non-empty string.");
  assert(stringp(role) && truthy(role), "Role must be a non-empty string.");

  name = lower_case(name);
  role = lower_case(role);

  string *user_roles = security_roles[name] ? copy(security_roles[name]) : ({});

  assert(uniformp(user_roles, T_STRING), "Roles for '"+name+"' is not an array of strings.");
  assert(every(user_roles, (: truthy :)), "Roles for '"+name+"' is not an array of non-empty strings.");

  eject_value_all(ref user_roles, role);

  if(sizeof(user_roles))
    security_roles[name] = user_roles;
  else
    map_delete(security_roles, name);

  write_file(ROLES_FILE_CUSTOM, pretty_map(security_roles), 1);

  return copy(user_roles);
}

/**
 * Removes all of a user's direct role grants from roles.map. Used when
 * an account is deleted. Group membership lives in groups.lpml and is
 * not touched.
 *
 * @param {string} name - The user name to purge.
 * @returns {void}
 * @errors If name is not a non-empty string.
 */
public nomask void purge_roles(string name) {
  assert(stringp(name) && truthy(name), "Name must be a non-empty string.");

  name = lower_case(name);

  if(nullp(security_roles[name]))
    return;

  map_delete(security_roles, name);

  write_file(ROLES_FILE_CUSTOM, pretty_map(security_roles), 1);
}

/**
 * Returns the member list of a group.
 *
 * @param {string} group - The group name.
 * @returns {string*} A copy of the group's member list, or an empty
 *                    array if no such group exists.
 */
public nomask string *query_group(string group) {
  mapping data = security_groups[group];

  if(!mapp(data) || !pointerp(data["members"]))
    return ({});

  return copy(data["members"]);
}

/**
 * Returns a copy of the full merged group definitions.
 *
 * @returns {([ string: ([ "roles": string*, "members": string* ]) ])}
 *          A copy of all group definitions.
 */
public nomask mapping query_groups() {
  return copy(security_groups);
}

/**
 * Returns the names of all defined groups.
 *
 * @returns {string*} The list of group names.
 */
public nomask string *query_group_names() {
  return keys(security_groups);
}

/**
 * Restores the persisted role grants from disk into the in-memory
 * security_roles authority. Runs once per master (re)load. At runtime
 * the in-memory copy is the authority and is kept current by the
 * add_role()/remove_role()/purge_roles() write-through API, so this is
 * only the load-on-(re)load step, not a per-call refresh.
 *
 * @returns {void}
 */
private nomask void restore_roles() {
  security_roles = load_roles();
}

/**
 * Reads the direct-grant role map (roles.map) from disk via the cache
 * daemon. Called only by restore_roles() to populate the in-memory
 * security_roles authority; permission checks read that, never this.
 *
 * @returns {([ string: string* ])} A copy of the user-to-roles map.
 */
private nomask mapping load_roles() {
  mapping all_roles = CACHE_D->load_data(
    ROLES_FILE_CUSTOM,
    ([
      "kind": "auto",
    ])
  );

  if(!mapp(all_roles))
    all_roles = ([]);

  return copy(all_roles);
}

/* Driver validation applies
 *
 * These remain permissive pending the path-access layer, which will
 * resolve identity -> roles against per-directory permissions. They
 * are intentionally open during the role/group build-out.
 */

/**
 * Driver apply: decides whether one object may shadow another.
 *
 * No shadows. None. Git.
 *
 * @apply
 * @param {object} _ob - The object to be shadowed.
 * @returns {int} Always 0 - Oxidus does not use shadows.
 */
private int valid_shadow(object _ob) {
  return 0;
}

/**
 * Driver apply: decides whether a function may be bound from one
 * object onto another.
 *
 * Yeah, I'm fine with this being always 1. At least until I discover a use
 * case otherwise.
 *
 * @apply
 * @param {object} _obj - The object whose function is being bound.
 * @param {object} _owner - The object that owns the function.
 * @param {object} _target - The object the function is bound to.
 * @returns {int} Always 1.
 */
private int valid_bind(object _obj, object _owner, object _target) {
  return 1;
}

/**
 * Driver apply: decides whether an object may hide itself.
 *
 * @NOTE That's a no from me, dawg.
 *
 * @apply
 * @param {object} _ob - The object requesting to be hidden.
 * @returns {int} Always 0 - Oxidus does not hide.
 */
private int valid_hide(object _ob) {
  return 0;
}

/**
 * Driver apply: decides whether a hard link may be created.
 *
 * @NOTE Undecided if I want to allow hard linking. I think that I do not.
 *
 * @apply
 * @param {string} _from - The link source path.
 * @param {string} _to - The link destination path.
 * @returns {int} Always 0 - linking is not used.
 */
public int valid_link(string _from, string _to) {
  return 0;
}

/**
 * Driver apply: decides whether a newly loaded object may exist.
 *
 * @TODO Update later.
 *
 * @apply
 * @param {object} _ob - The object being validated.
 * @returns {int} Always 1.
 */
private int valid_object(object _ob) {
  return 1;
}

/**
 * Driver apply: decides whether a file may override an efun.
 *
 * @TODO Update later.
 *
 * @apply
 * @param {string} _file - The file attempting the override.
 * @param {string} _efun_name - The name of the efun being overridden.
 * @param {string} _mainfile - The file defining the override.
 * @returns {int} Always 1.
 */
private int valid_override(string _file, string _efun_name, string _mainfile) {
  return 1;
}

/**
 * Driver apply: decides whether a socket operation is permitted. We aren't
 * doing granular anything here yet.
 *
 * @apply
 * @param {object} _caller - The object requesting the socket call.
 * @param {string} _func - The socket function being invoked.
 * @param {mixed*} _info - Socket call information.
 * @returns {int} Always 1.
 */
private int valid_socket(object _caller, string _func, mixed *_info) {
  return 1;
}

/**
 * Driver apply: decides whether a file may be read.
 *
 * @TODO Update later.
 *
 * @apply
 * @param {string} _file - The file being read.
 * @param {object} _user - The object on whose behalf the read occurs.
 * @param {string} _func - The efun performing the read.
 * @returns {int} Always 1 pending the path-access layer.
 */
public int valid_read(string _file, object _user, string _func) {
  return 1;
}

/**
 * Driver apply: decides whether a file may be written.
 *
 * @TODO Update later.
 *
 * @apply
 * @param {string} _file - The file being written.
 * @param {object} _user - The object on whose behalf the write occurs.
 * @param {string} _func - The efun performing the write.
 * @returns {int} Always 1 pending the path-access layer.
 */
public int valid_write(string _file, object _user, string _func) {
  return 1;
}

/**
 * Driver apply: decides whether a database operation is permitted.
 *
 * This is permissive because we're using SQLite3 and don't need any specific
 * gating here.
 *
 * @apply
 * @param {object} _caller - The object requesting the database call.
 * @param {string} _fun - The database function being invoked.
 * @param {mixed*} _info - Database call information.
 * @returns {mixed} Always 1 - the mudlib uses SQLite3.
 */
private mixed valid_database(object _caller, string _fun, mixed *_info) {
  return 1;
}
