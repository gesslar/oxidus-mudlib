/**
 * @file /cmds/object/trace.c
 * @description Command to locate any object and its active clones,
 *              with optional destruction.
 *
 * @created 1993-03-04 - Watcher@TMI
 * @last_modified 2024-02-04 - Gesslar
 *
 * @history
 * 1993-03-04 - Watcher@TMI - Created
 * 2024-02-04 - Gesslar - General formatting
 */

inherit STD_CMD;

#define SYNTAX "Syntax: trace -[d/v] [object/filename]\n"
#define PROTECT ({ })

public mixed main(
  /** @type {STD_PLAYER} */ object caller, string str
) {
  object target;
  mixed *cloneList;
  string para, output, t1, t2;
  int dest, view, original;

  if(!str || str == "")
    return SYNTAX;

  if(sscanf(str, "-%s %s", para, str) == 2) {
    if(sscanf(" " + para + " ", "%sd%s", t1, t2) == 2)
      dest = 1;
    if(sscanf(" " + para + " ", "%sv%s", t1, t2) == 2)
      view = 1;
  }

  target = get_object(str);

  if(!target)
    return _error(
      "Could not locate requested object."
    );

  output = "Trace: " + target;

  if(environment(target))
    output += " in " + environment(target) + "\n";
  else
    output += "\n";

  cloneList = children(file_name(target));

  if(sizeof(cloneList) == 1) {
    if(base_name(cloneList[0]) == file_name(cloneList[0]))
      output +=
        "There are no active copies of this object.\n";

    if(dest) {
      if((interactive(target) || userp(target) ||
        member_array(
          base_name(target), PROTECT
        ) != -1) && adminp(caller))
        return _error(
          "You do not have authorisation to "
          "destruct that object."
        );

      target->remove();
      if(target)
        destruct(target);

      if(target)
        output +=
          "Could not remove or destroy object.\n";
      else
        output +=
          "Object has been removed and destructed.\n";
    }

    return explode(output, "\n");
  } else {
    cloneList -= ({ target });
  }

  original = sizeof(cloneList);

  if(!(dest && !view)) {
    output += "\n   There are " + sizeof(cloneList) +
      " copies active.\n\n";

    foreach(object clone in cloneList) {
      if(interactive(clone))
        output += " I ";
      else
        output += "   ";

      output += file_name(clone);

      if(environment(clone))
        output += "\tin " +
          sprintf("%O\n", environment(clone));
      else
        output += "\n";
    }
  }

  if(dest) {
    if(!is_member(query_privs(caller), "admin") &&
      member_array(
        base_name(target), PROTECT
      ) != -1)
      return _error(
        "You do not have authorisation to "
        "destruct that object group."
      );

    cloneList->remove();
    cloneList = children(file_name(target));
    cloneList -= ({ target });

    for(int i = 0; i < sizeof(cloneList); i++)
      destruct(cloneList[i]);

    cloneList = children(file_name(target));
    cloneList -= ({ target });

    output += "All " + original + " copies of " +
      file_name(target) +
      " have been removed and destroyed";

    if(!sizeof(cloneList)) {
      output += ".\n";
      return explode(output, "\n");
    }

    output += " except:\n";

    foreach(object remaining in cloneList)
      output += "  " + remaining + "\n";
  }

  return explode(output, "\n");
}

string query_help(object _caller) {
  return SYNTAX + "\n"
"This command allows the user to locate the requested "
"object and any active clones with their respective "
"locations. The parameter 'd' can be used to remove and "
"destruct every copy. When the 'd' parameter is invoked, "
"the clones and locations will not be displayed. This can "
"be overridden with the 'v' parameter.\n\n"
"The parameters can be given together and in any "
"combination.\n\n"
"For example: trace -dv /obj/dagger will destruct every "
"copy of /obj/dagger and display their respective ids "
"and locations.\n";
}
