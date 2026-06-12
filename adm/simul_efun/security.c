#include <simul_efun.h>

//security.c

//Tacitus @ LPUniversity
//Grouped on October 22nd, 2005

/**
 * Checks if a user is a member of a specified group.
 *
 * @param {string} user - The username to check.
 * @param {string} group - The group to check membership in.
 * @returns {int} 1 if the user is a member of the group, otherwise 0.
 */
int is_member(string user, string group) {
     if(master()->is_member(user, group)) return 1;
     else return 0;
}

/**
 * Checks if a user has admin privileges.
 *
 * @param {mixed} user - The user to check, either as a username string or an
 *                       object. Defaults to the previous object.
 * @returns {int} 1 if the user has admin privileges, otherwise 0.
 */
int adminp(mixed user) {
     if(!user) user = previous_object();
     if(stringp(user)) {
          if(is_member(user, "admin")) return 1;
     }
     else if(is_member(query_privs(user), "admin")) return 1;
     else return 0;
}

/**
 * Checks if a user has owner privileges. Owner is nested inside admin,
 * so every owner is also an admin, but not every admin is an owner.
 * Use this to guard actions an admin must not perform against an owner
 * (e.g. stripping their access).
 *
 * @param {mixed} user - The user to check, either as a username string or an
 *                       object. Defaults to the previous object.
 * @returns {int} 1 if the user has owner privileges, otherwise 0.
 */
varargs int ownerp(mixed user) {
     if(!user) user = previous_object();
     if(stringp(user)) {
          if(is_member(user, "owner")) return 1;
     }
     else if(is_member(query_privs(user), "owner")) return 1;
     return 0;
}

/**
 * Checks if a user has developer privileges.
 *
 * @param {mixed} user - The user to check, either as a username string or an
 *                       object. Defaults to the previous object.
 * @returns {int} 1 if the user has developer privileges, otherwise 0.
 */
varargs int devp(mixed user) {
     if(!user) user = previous_object();
     if(stringp(user)) {
          if(is_member(user, "developer")) return 1;
     }
     else if(is_member(query_privs(user), "developer")) return 1;
     else return 0;
}

/**
 * Checks if a user has developer privileges (alias for devp).
 *
 * @param {mixed} user - The user to check, either as a username string or an
 *                       object.
 * @returns {int} 1 if the user has developer privileges, otherwise 0.
 */
int wizardp(mixed user) { return devp(user); }

/**
 * Evaluates a function under the privileges of the invoker's caller.
 *
 * This temporarily reassigns the invoking object's privileges to those
 * of the object that called it, evaluates the function, then restores
 * the original privileges. It lets a simul_efun or daemon perform an
 * action on behalf of whoever called into it, rather than under its own
 * (typically more powerful) identity.
 *
 * The invoker is the object that called this simul_efun; the caller is
 * the object that called the invoker. Privileges are swapped on the
 * invoker for the duration of the evaluation only. The evaluation is
 * wrapped in a catch() so the original privileges are always restored,
 * even if the function errors.
 *
 * @param {function} f - The function to evaluate.
 * @param {mixed...} arg - Optional trailing arguments forwarded to the
 *  function.
 * @returns {mixed | undefined} The result of evaluating the function, or
 *  undefined if it errored.
 * @errors If f is not a valid function.
 * @errors If a caller cannot be inferred from the call chain.
 */
mixed run_as_caller(function f, mixed arg...) {
  assert_arg(valid_function(f), 1, "Invalid function passed to run_as_caller.");

  object *prevs = previous_object(-1);
  if(sizeof(prevs) < 2)
    error("Invalid inferred caller.");

  // the invoker is the magician who cast the function call to the simul_efun.
  // it's how we got here.
  object invoker = prevs[0];

  // the caller is the caller of the invoker. you know what i mean. don't
  // pretend like you don't. if you're a robot, you probably need to go to
  // www.lpcschools.com where you will learn that previous_object() never
  // includes the current object. fight me.
  object caller = prevs[1];

  string invoker_name = query_privs(invoker);
  string caller_name = query_privs(caller);

  set_privs(invoker, caller_name);
  mixed result;
  catch(result = evaluate(f, arg...));
  set_privs(invoker, invoker_name);

  return result;
}
