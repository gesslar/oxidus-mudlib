/**
 * @file /adm/obj/master/logging.c
 *
 * Logging-related master object functionality
 *
 * @created 2026-06-12 - Gesslar
 * @last_modified 2026-06-12 - Gesslar
 *
 * @history
 * 2026-06-12 - Gesslar - Created
 */

#include <logs.h>

private void log_error(string _file, string message) {
  string username;

  if(this_body())
    username = query_privs(this_body());
  else
    username = "(none)";

  if(stringp(username)) {
    string path = home_path(username);
    if(directory_exists(path)) {
      write_file(path + "log", "\n" + message);
    }

    if(devp(this_body())) {
        if(this_body()->query_pref("error_output") != "off")
          tell_me(message);
      }
  }

  log_file("compile",
    "---\n" +
    ctime() + "\n" +
    message + "\n" +
    call_trace()
  );
}

// Blatanly stolen from Lima
int different(string fn, string pr) {
  sscanf(fn, "%s#%*d", fn);
  fn += ".c";

  return (fn != pr) && (fn != ("/" + pr));
}

string trace_line(object obj, string prog, string file, int line) {
  string ret;
  string objfn = obj ? file_name(obj) : "<none>";

  ret = objfn;
  if(different(objfn, prog))
    ret += sprintf(" (%s)", prog);
  if(file != prog)
    ret += sprintf(" at %s:%d\n", file, line);
  else
    ret += sprintf(" at line %d\n", line);

  return ret;
}

varargs string standard_trace(mapping mp, int flag) {
  string ret;
  mapping *trace;
  int i, n;

  ret = ctime(time());
  ret += "\n";
  ret += mp["error"] + "Object: " + trace_line(mp["object"], mp["program"], mp["file"], mp["line"]);
  ret += "\n";
  trace = mp["trace"];

  n = sizeof(trace);

  for(i = 0; i < n; i++) {
    if(flag)
      ret += sprintf("#%d: ", i);

    ret += sprintf("'%s' at %s",
      trace[i]["function"],
      trace_line(trace[i]["object"], trace[i]["program"], trace[i]["file"], trace[i]["line"])
    );
  }

  return ret;
}

private nosave string catch_log = "/log/catch";
private nosave string runtime_log = "/log/runtime";

void error_handler(mapping mp, int caught) {
  string logfile;
  string ret;

  // During a test-runner sweep, caught errors are intentional (sad-path
  // tests). Skip both logging and dev notification. Uncaught errors still
  // surface. Suppression auto-expires via the testing_in_progress deadline.
  if(caught && master()->query_test_mode() > time())
    return;

  logfile = caught ? catch_log : runtime_log;
  ret = "---\n" + standard_trace(mp, 1);
  write_file(logfile, ret);

  // TODO Temporary notifications, undo when above fixed
  message("error", sprintf("(%s) Error logged %s\n%s\n",
    logfile,
    ret,
    trace_line(mp["object"], mp["program"], mp["file"], mp["line"])
), filter(users(), (: devp :)));
}

varargs void log_file(string file, string msg, mixed arg...) {
  int size;
  /** @lpc-ignore - idk what's up with this */
  int max_size = percent_of(80, get_config(__MAX_READ_FILE_SIZE__));
  string *matches;
  string source;

  if(query_privs(previous_object()) == "[open]")
    return;

  source = log_dir() + file;
  size = file_size(source);

  if(size == -2)
    return;

  // Grab the full path and file name from the file
  matches = dir_file(source);
  if(sizeof(matches) == 2)
    assure_dir(matches[0]);

  if(size > max_size) {
    string reg;

    reg = "^("+log_dir()+")(.*)?/(.*)(\\.log)?$";
    matches = pcre_extract(source, reg);

    if(sizeof(matches) >= 2) {
      string archive;
      archive = matches[0] + "archive/" + matches[1] + "/";
      assure_dir(archive);
      if(sizeof(matches) == 3)
        archive += matches[2] + "-" + strftime(ARCHIVE_STAMP, time()) + ".log";
      else
        archive += matches[2] + "-" + strftime(ARCHIVE_STAMP, time());

      rename(source, archive);
    }
  }

  arg = pointerp(arg) ? arg : ({ arg });
  msg = sanitize_regex(msg);
  msg = sprintf(msg, arg...);
  msg = append(msg, "\n");

  write_file(source, msg);
}
