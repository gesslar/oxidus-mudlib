/**
 * @file /adm/simul_efun/function.c
 *
 * Advanced function manipulation and callback handling simul-efuns. Provides
 * tools for function validation, callback assembly, call tracing, and delayed
 * execution management.
 *
 * @created Unknown
 * @last_modified 2024-03-11
 */

#include <simul_efun.h>
#include <function.h>

/**
 * Verifies that a function pointer is valid and usable.
 *
 * Checks if the function exists and its owner object hasn't been destructed.
 *
 * @param {mixed} f - Function pointer to validate
 * @returns {int} 1 if function is valid and callable, 0 otherwise
 * @example
 * function f = (: object->method :);
 * if(valid_function(f)) {
 *     evaluate(f);
 * }
 */
int valid_function(mixed f) {
  int fp;

  fp = functionp(f);

  if(fp)
    return !(fp & FP_OWNER_DESTED);

  return false;
}

/**
 * Checks if an object has one or more specified functions defined.
 *
 * Accepts either a single function name or an array of function names. When
 * given an array, checks each function in order and returns the location of
 * the first one that exists.
 *
 * @param {object} ob - The object to check.
 * @param {string|string*} functionNames - Function name(s) to look for `(string|string*)`
 * @returns {string|null} The filename where a function is defined, or null
 *                        if none found or if ob is not an object.
 * @example
 * has(player, "query_level");  // Returns "/std/body.c" if exists
 * has(item, ({ "is_weapon", "is_armour" }));  // Returns filename of first existing function
 */
string has(object ob, mixed functionNames) {
  if(!objectp(ob))
    return null;

  if(stringp(functionNames))
    return function_exists(functionNames, ob);
  if(pointerp(functionNames) && uniform_array(functionNames, T_STRING))
    return eval_first(functionNames, (: function_exists($1, $(ob)) :));

  return null;
}

private nosave string *traceColours = ({
  "#0099ff",
  "#66ff33",
  "#ff33cc",
  "#ff6600",
  "#33cccc",
  "#ffcc33",
});

/**
 * Generates a formatted call stack trace.
 *
 * Creates a detailed stack trace with object paths, line numbers, and
 * function names. Each frame is color-coded when colors are enabled.
 *
 * @param {int} [colour=0] - Whether to include ANSI color codes
 * @returns {string} Formatted call stack trace
 * @example
 * debug("Error occurred:\n" + call_trace(1));
 */
varargs string call_trace(int colour) {
  string res;
  int i, n;
  object *objects;
  string *programs;
  string *functions;
  string *origins;
  string *lines;
  string *colours = colour
    ? traceColours
    : map(traceColours, (: "" :));

  res = "";
  programs = call_stack(0);
  objects = call_stack(1);
  functions = call_stack(2);
  origins = call_stack(3);
  lines = call_stack(4);

  // now fix up the lines
  lines = map(lines, function(string line) {
    int num;
    sscanf(line, "%*s:%d", num);
    return sprintf("%d", num);
  });

  n = sizeof(programs);

  if(colour || !colour) {
    object source = this_body();
    function match_body = (: $(source) && objectp($(source)) && living($(source)) && $(source) == $1 :);

    colours = map(colours, (:"{{"+$1[1..]+"}}":));

    // We don't want to include the call_trace() function itself
    res += reduce(objects[1..],
      function(string acc, object obj, int index, object *objs,
        string *programs, string *lines, string *functions, string *origins, string *cols, function match_body) {
          return sprintf("%s[%s%s{{res}}] %s%s{{res}}:%s%s{{res}}->%s%s{{res}}() (%s%s{{res}})\n",
            acc,
            match_body(obj) ? cols[5] : cols[0],
            match_body(obj) ? sprintf("%s", file_name(obj)) : sprintf("%O", obj),
            cols[1],
            programs[index+1],
            cols[2],
            lines[index+1],
            cols[3],
            functions[index+1],
            cols[4],
            origins[index+1]
          );
      }, "", programs, lines, functions, origins, colours, match_body);
  }

  if(!colour)
    res = replace_string(res, "{{res}}", "");

  return res;
}

/**
 * Creates a callback structure for delayed execution.
 *
 * Builds a standardized callback array that can be stored and executed later.
 * Supports both object methods and function pointers with arguments.
 *
 * @param {object|function} target - Object or function to callback to
 * @param {string} [method] - Method name if target is an object
 * @param {...mixed} arg - Additional arguments to pass to callback
 * @returns {mixed*} Assembled callback array
 * @errors If arguments are invalid or method doesn't exist
 * @see call_back
 * @see delay_act
 * @example
 * mixed *cb = assemble_call_back(player, "move", "/room/square");
 * mixed *cb = assemble_call_back((: do_something :), arg1, arg2);
 */
mixed *assemble_call_back(mixed arg...) {
  int sz;

  if(!pointerp(arg))
    error("ERROR: Invalid argument passed to assemble_call_back().");

  sz = sizeof(arg);
  if(!sz)
    error("ERROR: No arguments passed to assemble_call_back().");

  if(objectp(arg[0])) {
    object ob;
    string fun;

    if(sz < 2)
      error("ERROR: No function passed to assemble_call_back().");

    ob = arg[0];
    fun = arg[1];
    if(sz > 2)
      arg = arg[2..];
    else
      arg = ({});

    if(!function_exists(fun, ob))
      error("ERROR: Function does not exist in object passed to assemble_call_back().");

    return ({ ob, fun, arg... });
  } else if(valid_function(arg[0])) {
    if(sz > 1)
      return ({ arg[0], arg[1..]... });
    else
      return ({ arg[0], ({})... });
  }

  error("ERROR: Invalid function passed to assemble_call_back().");
}

/**
 * Executes a previously assembled callback.
 *
 * Takes a callback structure created by assemble_call_back() and executes it,
 * optionally with new arguments prepended to the stored ones.
 *
 * @param {mixed*} cb - Callback array from assemble_call_back()
 * @param {...mixed} new_arg - Additional arguments to prepend
 * @returns {mixed} Result from callback execution
 * @errors If callback structure is invalid
 * @see assemble_call_back
 * @see valid_function
 * @example
 * mixed *cb = assemble_call_back(player, "tell", "Hello!");
 * call_back(cb);  // Calls player->tell("Hello!")
 */
mixed call_back(mixed cb, mixed new_arg...) {
  int sz;
  function fun;
  mixed final_arg = ({});

  if(!pointerp(cb))
    error("ERROR: Invalid argument passed to call_back().");

  sz = sizeof(cb);
  if(!sz)
    error("ERROR: No arguments passed to call_back().");

  if(objectp(cb[0])) {
    object cb_ob = cb[0];
    string cb_fun;
    mixed *curr;

    if(sz < 2)
      error("ERROR: No function passed to call_back().");

    cb_fun = cb[1];

    if(sz > 2)
      curr = cb[2..];
    else
      curr = ({});

    if(!function_exists(cb_fun, cb_ob))
      error("ERROR: Function does not exist in object passed to call_back().");

    final_arg = ({ new_arg... }) + curr;

    fun = (: call_other, cb_ob, cb_fun :);
  } else if(valid_function(cb[0])) {
    function cb_fun = cb[0];
    mixed *curr;

    if(sz > 1)
      curr = cb[1..];
    else
      curr = ({});

    fun = cb_fun;
    final_arg = ({ new_arg... }) + curr;
  } else
    return null;

  return catch(fun(final_arg...));
}

/**
 * Safely calls a function on an object if it exists.
 *
 * Attempts to call the named function, returning null if function doesn't
 * exist instead of throwing an error.
 *
 * @param {object|string} ob - Target object or filename
 * @param {string} func - Function name to call
 * @param {...mixed} [arg] - Arguments to pass to function
 * @returns {mixed} Function result or null if function doesn't exist
 * @errors If ob or func arguments are invalid types
 * @see function_exists
 * @see call_back
 * @example
 * call_if(user, "receive_message", "Hello");
 */
varargs mixed call_if(mixed ob, string func, mixed arg...) {
  if(nullp(ob))
    error("Missing argument 1 to call_if()");

  if(!objectp(ob) && !stringp(ob))
    error("Bad argument 1 to call_if()");

  if(nullp(func))
    error("Missing argument 2 to call_if()");

  if(!stringp(func))
    error("Bad argument 2 to call_if()");

  if(stringp(ob))
    ob = load_object(ob);

  if(function_exists(func, ob))
    if(sizeof(arg))
      return call_other(ob, func, arg...);
    else
      return call_other(ob, func);

  return null;
}

/**
 * Schedules an action with delayed execution.
 *
 * Creates a delayed action on the current body object, with optional
 * callback on completion.
 *
 * @param {string} act - Action description
 * @param {float} delay - Time to wait before execution
 * @param {mixed*} [cb] - Callback to execute after delay
 * @returns {int} Action ID or 0 if already acting
 * @example
 * delay_act("casting", 3.0, assemble_call_back(this_object(), "cast_spell"));
 */
varargs int delay_act(string act, float delay, mixed *cb) {
  object user = this_body();

  if(!user)
    return null;

  if(!act || !delay || !cb)
    return null;

  if(user->is_acting())
    return 0;

  return user->act(act, delay, cb);
}

/**
 * Verifies a condition, throwing an error if false.
 *
 * Enhanced assertion checking with custom error messages and evaluation
 * of complex conditions.
 *
 * @param {mixed} condition - Condition to verify
 * @param {string} [message="Assertion failed"] - Error message if condition fails
 * @throws If statement evaluates to false or null
 * @example
 * assert(hp > 0, "Health cannot be negative");
 */
varargs void assert(mixed condition, string message) {
  mixed result;
  string err;

  if(nullp(condition))
    error("Missing argument 1 to assert()");

  if(!stringp(message))
    message = "Assertion failed";

  err = catch(result = evaluate(condition));

  if(stringp(err)) {
    message = append(message, "\n");
    error(message + err);
  }

  if(nullp(result))
    error(message);

  if(result == 0)
    error(message);
}

/**
 * Verifies a function argument condition.
 *
 * Specialized assertion for validating function parameters with
 * numbered argument reference.
 *
 * @param {mixed} condition - Condition to verify
 * @param {int} arg_num - Parameter position number
 * @param {string} message - Error message prefix
 * @throws If condition is false, with formatted argument number
 * @example
 * assert_arg(stringp(name), 1, "Expected string");
 */
void assert_arg(mixed condition, int arg_num, string message) {
  message = message || "";
  message += sprintf(" [Arg #%d]", arg_num);
  assert(condition, message);
}

mixed pipe(mixed result, function *funcs...) {
  if(sizeof(funcs) < 1)
    return evaluate(result);

  assert(
    every(funcs, (: valid_function :)),
    "Pipe requires every element to be a function after the initial."
  );

  foreach(function f in funcs)
    result = f(result);

  return result;
}
