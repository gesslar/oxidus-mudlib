#include <simul_efun.h>

#include <commands.h>
#include <daemons.h>

//overrides.c

//Tacitus @ LPUniversity
//08-APR-05
//Simul-efuns

//Last edited on December 21st, 2005 by Tacitus

/**
 * @efun_override exec
 *
 * Security-wrapped override of the exec() efun. Only permits execution
 * transfers when called by an admin, a ghost object, a user object,
 * the su command, or a linked object.
 *
 * @param {object} to - The object to transfer execution to
 * @param {object} from - The object to transfer execution from
 * @returns {int} 1 on success, 0 if the caller lacks permission
 */
int exec(object to, object from) {
  if(is_member(query_privs(previous_object()), "admin") ||
     base_name(previous_object()) == STD_GHOST ||
     userp(previous_object()) ||
     base_name(previous_object()) == CMD_SU ||
     linkp(previous_object()))
    return efun::exec(to, from);
  else
    return 0;
}

/**
 * @efun_override shutdown
 *
 * Security-wrapped override of the shutdown() efun. Only the master
 * object or SHUTDOWN_D may invoke it. Emits SIG_SYS_SHUTDOWN and
 * persists all objects before shutting down the driver.
 *
 * @param {int} how - The shutdown mode passed to the driver
 */
void shutdown(int how) {
  object po = previous_object();

  if(po != master() && po != load_object(SHUTDOWN_D))
      return;

  emit(SIG_SYS_SHUTDOWN);
  PERSIST_D->persist_objects();
  efun::shutdown(how);
}

/**
 * @efun_override set_privs
 *
 * Security-wrapped override of the set_privs() efun. Permits setting
 * privileges when the caller is an admin or the master object, or when
 * the object resides in a player's home directory and the privilege
 * string matches the player name.
 *
 * @param {object} ob - The object to set privileges on
 * @param {string} privs - The privilege string to assign
 */
void set_privs(object ob, string privs) {
  string name;

  if(is_member(query_privs(previous_object()), "admin") || ob = master())
    efun::set_privs(ob, privs);

  sscanf(file_name(ob), "/home/%*s/%s/%*s", name);

  if(name == privs)
    efun::set_privs(ob, privs);
}

// void write(string msg) {
//   error("Use tell.");
// }

// varargs void say(string msg, mixed exclude) {
//   error("Use tell.");
// }

/**
 * @efun_override shout
 * @description This function is an override of the shout() efun. It will
 *              write a message to all users using the message() efun.
 * @param {string} msg - The message to write.
 * @param {mixed} exclude - The object or array of objects to exclude from
 *                          the message.
 */
// varargs void shout(string msg, mixed exclude) {
//   error("Use tell.");
// }

/**
 * @efun_override tell_object
 * @description This function is an override of the tell_object() efun. It will
 *              write a message to the object using the message() efun.
 * @param {object} ob - The object to write the message to.
 * @param {string} msg - The message to write.
 * @param {string} type - The message class of message to write.
 */
// varargs void tell_object(mixed ob, mixed msg, mixed type) {
//   error("Use tell.");
// }

/**
 * @efun_override tell_room
 * @description This function is an override of the tell_room() efun. It will
 *              write a message to the room using the message() efun.
 * @param {mixed} room - The room to write the message to.
 * @param {string} msg - The message to write.
 * @param {mixed} exclude - The object or array of objects to exclude from
 *                          the message.
 */
// varargs void tell_room(mixed room, string msg, mixed exclude) {
//   error("Use tell.");
// }

/**
 * @efun_override userp
 * @description This function is an override of the userp() efun. Unlike the
 *              efun, this function accepts a null argument and will check
 *              if the previous object is a user if no argument is provided.
 * @param {object} ob - The object to check.
 * @returns {int} - 1 if the object is a user, otherwise 0.
 */
varargs int userp(object ob) {
  if(!ob)
    ob = previous_object();

  return
    efun::userp(ob);
}

/**
 * @efun_override query_num
 * @description This function is an override of query_num(). It will return
 *              the number in string form.
 * @param {int} x - The number to convert to a string.
 * @param {int} many - If true, the number will be returned in plural form.
 * @returns {string} - The number in string form.
 */
varargs string query_num(int x, int many) {
  string sign;

  assert_arg(!nullp(x), 1, "Missing argument.");
  assert_arg(!nullp(many) && intp(many), 2, "Invalid argument.");

  sign = x < 0 ? "negative " : "";
  x = abs(x);

  return sign + efun::query_num(x, many);
}

/**
 * @efun_override ctime
 *
 * Override of the ctime() efun that returns a formatted date-time
 * string in `YYYY-MM-DD HH:MM:SS` format instead of the default
 * ctime format. Defaults to the current time if no argument is
 * provided.
 *
 * @param {int} [x] - The Unix timestamp to format; defaults to now
 * @returns {string} The formatted date-time string
 */
varargs string ctime(int x) {
  string fmt;

  if(nullp(x))
    x = time();

  fmt = "%Y-%m-%d %H:%M:%S";

  return strftime(fmt, x);
}

/**
 * @efun_override element_of
 *
 * Override of the element_of() efun that optionally uses a
 * cryptographically secure random number generator for selection.
 *
 * @param {mixed*} arr - The array to select a random element from
 * @param {int} [secure] - If true, uses secure_random() instead of
 *                          the standard PRNG
 * @returns {mixed} A randomly selected element from the array
 * @errors If a non-array value is passed
 */
varargs mixed element_of(mixed *arr, int secure) {
  if(!secure)
    return efun::element_of(arr);

  else {
    if(pointerp(arr)) {
      int sz = sizeof(arr);
      int element = secure_random(sz);

      return arr[element];
    }
  }

  error("Invalid array passed to element_of()");
}

/**
 * @efun_override debug_message
 *
 * Override of the debug_message() efun that prepends a timestamp
 * in `YYYY/MM/DD HH:MM:SS` format and also logs the message to
 * the debug.log file.
 *
 * @param {string} str - The debug message to output
 */
void debug_message(string str) {
  string timestamp;

  str = chop(str, "\n", -1);

  timestamp = strftime("%Y/%m/%d %H:%M:%S", time());
  efun::debug_message(sprintf("%s %s", timestamp, str));
  master()->log_file("debug.log", sprintf("%s %s\n", timestamp, str));
}
