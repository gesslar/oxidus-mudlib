/*
  Old valid.c has been removed, but to prevent runtime errors, all necessary
  functions have been stubbed out with blanket true/false results until a
  new permission system has been built to replace this one.

  It's gon be _chaos_.

  Live. Laugh. Linkin Park.
*/

protected void parse_group() {
}

protected void parse_access() {
}

private int valid_shadow(object _ob) {
  return false; // Oxidus doesn't use shadows anyway.
}

private int valid_bind(object _obj, object _owner, object _target) {
  return true; // sure, why not
}

private int valid_hide(object _ob) {
  return false; // Oxidus does not hide!
}

public int valid_link(string _from, string _to) {
  return false; // nope, we don't need this
}

private int valid_object(object _ob) {
  return true; // eh, for now anyway
}

private int valid_override(string _file, string _efun_name, string _mainfile) {
  return true; // yep!
}

private int valid_socket(object _caller, string _func, mixed *_info) {
  return true; // i was already doing true here anyway
}

public int valid_read(string _file, object _user, string _func) {
  return true; // the bots aren't going to like this one xD
}

public int valid_write(string _file, object _user, string _func) {
  return true; // this one even less, lol
}

private mixed valid_database(object _caller, string _fun, mixed *_info) {
  return 1; // We are using SQLITE3, so just return 1.
}

public string *query_group(string _group) {
  return ({});
}

public mapping query_groups() {
  return ([]);
}

public string *query_group_names() {
  return ({});
}

public int is_member(string _user, string _group) {
  return true; // baha
}
