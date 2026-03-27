/**
 * @file /cmds/object/call.c
 * @description Command to call functions on objects, supporting
 *              references and multiple targets.
 *
 * @created 1992-06-00 - Pallando (Douglas Reay)
 * @last_modified 2024-02-04 - Gesslar
 *
 * @history
 * 1992-06-00 - Pallando - Created for Ephemeral Dale (LPC2)
 * 1992-10-00 - Pallando - Added multi-reference tracer
 * 1993-01-02 - Pallando - Changed refs to be stored on player
 * 1993-03-19 - Pallando - Changed refs to get_objects()
 * 1993-07-17 - Robocoder - Switched call_other to arrays
 * 1993-08-20 - Grendel - Allowed admins to call with any euid
 * 1993-08-23 - Pallando - Changed to inherit REF_D
 * 1993-10-22 - Mobydick - Added NO_USER_CALLS define
 */

#include <daemons.h>
#include <logs.h>

#define NO_USER_CALLS

inherit STD_CMD;
inherit M_REF;

#define SYNTAX \
  "Syntax: call [-<uid>] " \
  "<object>;<function>;<arg>;<arg>\n"
#define FUNC_LIST \
  ({ "query_skills", "query_nicknames", \
     "query_aliases" })
#define TAB "\t"

private mixed doCall(object ob, string func, mixed args);

public mixed main(
  /** @type {STD_PLAYER} */ object _caller, string a
) {
  string str, *expA;
  mixed objs, funcs, args, tmp, ret, rets;
  object ob;
  int i, s, fi, fs;

  if(!a) {
    notify_fail(SYNTAX);
    return 0;
  }

  expA = explode(a, ";");
  s = sizeof(expA);
  objs = expA[0];
  if(s > 1)
    funcs = expA[1];
  else
    funcs = FUNC_LIST;
  if(s == 3)
    args = ({ expA[2] });
  if(s > 3)
    args = expA[2..(s - 1)];

  objs = resolv_ref(objs);
  if(objs == "users")
    objs = users();
  else if(objs == "actives")
    objs = filter(
      users(), (: query_idle($1) < 180 :)
    );
  else if(objs == "livings")
    objs = filter(
      livings(),
      (: clonep($1) && environment($1) :)
    );
  else if(objs == "monsters")
    objs = filter(
      livings(),
      (: clonep($1) && environment($1) &&
         !userp($1) :)
    );
  if(!pointerp(objs))
    objs = ({ objs });

  tmp = ({});
  s = sizeof(objs);
  for(i = 0; i < s; i++) {
    if(stringp(objs[i]))
      ob = get_objects(objs[i], 0, 1);
    else if(objectp(objs[i]))
      ob = objs[i];
    if(!ob)
      tell_me(
        "Can't identify " + identify(objs[i]) +
        " as an object.\n"
      );
    else
      tmp += ({ ob });
  }
  if(!sizeof(tmp))
    return 1;
  objs = tmp;

  funcs = resolv_ref(funcs);
  if(!pointerp(funcs))
    funcs = ({ funcs });
  tmp = ({});
  s = sizeof(funcs);
  for(i = 0; i < s; i++) {
    if(stringp(funcs[i]))
      tmp += ({ funcs[i] });
    else
      tell_me(
        "Can't identify " + identify(funcs[i]) +
        " as a string.\n"
      );
  }
  if(!sizeof(tmp))
    return 1;
  funcs = tmp;

  if(pointerp(args) && (s = sizeof(args)))
    for(i = 0; i < s; i++)
      args[i] = resolv_ref(resolv_str(args[i]));

  rets = ({});
  s = sizeof(objs);
  fs = sizeof(funcs);
  for(i = 0; i < s; i++) {
    str = identify(objs[i]);
    for(fi = 0; fi < fs; fi++) {
      ret = doCall(objs[i], funcs[fi], args);
      if(ret[0])
        rets += ({ ret[0] });
      if(fs == 1)
        str = str + ret[1];
      else
        str += (fi ? "" : "\n") + ret[1];
    }
    tell_me(str + "\n");
  }

  switch(sizeof(rets)) {
    case 0:
      rets = 0;
      break;
    case 1:
      rets = rets[0];
  }

  set_ref("default", rets);

  return 1;
}

private mixed doCall(
  object ob, string func, mixed args
) {
  mixed ret, err;
  int i, s;
  string str;
  object shad;

  if(!function_exists(func, ob)) {
    for(shad = shadow(ob, 0); shad;
      shad = shadow(shad, 0))
      if(function_exists(func, shad)) {
        ob = shad;
        break;
      }
    if(ob != shad)
      return ({
        0, "- does not contain " + func + "()"
      });
  }

  str = "->" + func;
  if(pointerp(args) && (s = sizeof(args))) {
    str += "(";
    for(i = 0; i < s; i++) {
      if(i)
        str += ", ";
      str += identify(args[i]);
    }
    str += ")";
  } else {
    str += "()";
  }

  if(!s)
    err = catch(ret = call_other(ob, func));
  else
    err = catch(
      ret = call_other(ob, ({ func }) + args)
    );

  if(err)
    return ({
      0,
      str + TAB + "\nERR(" + identify(err) + ")"
    });

  return ({
    ret,
    str + TAB + "\nResult: " + identify(ret)
  });
}

string query_help(object _caller) {
  return
SYNTAX + "\n"
"Calls the function <function> in object <object> "
"passing as many arguments <arg> as you give.\n\n"
"If no function is specified a dump of the object is "
"given.\n\n"
"<object> and <function> can be arrays "
"(e.g. \"users\")\n\n"
"Any of them can be references (SEE: help refs)\n";
}
