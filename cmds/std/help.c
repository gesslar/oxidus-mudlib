/**
 * @file /cmds/std/help.c
 *
 * Help command for all things helpful.
 *
 * @created 2026-06-07 - Gesslar
 * @last_modified 2026-06-07 - Gesslar
 *
 * @history
 * 2026-06-07 - Gesslar - Created
 */

private mixed get_command_help(object tp, string topic);

void main(
  /** @type {STD_PLAYER} */ object tp,
  string topic
) {

  function valid_result = (:
       (stringp($1) && truthy($1))
    || (pointerp($1) && uniformp($1, T_STRING) && sizeof(filter($1, (: truthy :))))
  :);

  mixed result;

  result = get_command_help(tp, topic);
  if(valid_result(result))
    return result;

  return result || "No help found for the topic '"+topic+"'.";
}

private mixed get_command_help(
  /** @type {STD_PLAYER} */ object tp,
  string topic
) {
  string *path = map(tp->get_path(), (: sprintf("%s%s.c", append($1, "/"), $(topic)) :));
  string *exists = filter(path, (: file_size($1) > -1 :));

  int sz = sizeof(exists);

  if(sz > 1)
    return "Too many command matches for '"+topic+"'.";

  if(sz < 1)
    return undefined;

  /** @type {STD_CMD} */ object cmd;
  string e = catch(cmd = load_object(exists[0]));

  if(e)
    throw(e);

  return cmd->query_help(tp);
}
