/**
 * @file /cmds/action/use.c
 *
 * General use command for things and stuff.
 *
 * @created 2026-05-09 - Gesslar
 * @last_modified 2026-05-09 - Gesslar
 *
 * @history
 * 2026-05-09 - Gesslar - Created
 */

inherit STD_ACT;

private nosave string fail = "You don't know how to use that beyond what is apparent.";

mixed use(/** @type {STD_BODY} */ object tp, string str) {
  if(!strlen(str))
    return "Use what?";

  object target;

  if(!target = carried_or_local_target(tp, str))
    return 1;

  if(!call_if(target, "can_use", tp))
    return fail;

  mixed result = call_if(target, "use_obj", tp) ?? fail;

  if(!result)
    return fail;

  return result;
}
