/* master.c

 Tacitus @ LPUniversity
 April 15th, 2005
 master object

 Last edited on July 14th, 2006 by Tacitus

*/

/* Preprocessor Statement */

#include <localtime.h>
#include <logs.h>

/* inherits */

inherit __DIR__ "master/valid";

/* Functions */

private nosave mapping errors = ([]);

void create() {
  // In master/valid.c
  parse_group();
  parse_access();

  call_out_walltime(function() {
    set_privs(this_object(), "[master]");
  }, 0.01);
}

void flag(string str) {
  if(str == "builddocs") {
    // call_out((: AUTODOC_D->ci_build() :), 2);
    call_out((: shutdown, 0 :), 10);
  } else {
    debug_message("Unknown flag: " + str);
  }
}

protected object connect(int port) {
  object login_ob;
  mixed err;

  // For some reason, we keep losing privs, so we'll set them again
  set_privs(this_object(), "[master]");

  err = catch(login_ob = new(LOGIN_OB));

  if(err) {
    tell_me("I'm sorry, but it appears that mud is not functional at the moment.\n");
    tell_me(err);
    destruct(this_object());
  }

  return login_ob;
}

protected void epilog(int load_empty) {
  string str, *lines, err;
  int i;
  float time;
  string out = "";
  object ob;

  set_privs(this_object(), "[master]");
  lines = explode_file("/adm/etc/preload");

  if(!sizeof(lines))
    return;

  for(i = 0; i < sizeof(lines); i++) {
    out = "";
    out += "Preloading : " + lines[i] + "...";
    time = time_frac();
    err = catch(ob = load_object(lines[i]));
    if(err != 0) {
      out += "\nError " + err + " when loading " + lines[i];
    } else {
      out += sprintf(" Done (%.2fs)", time_frac() - time);
    }
    debug_message(out);
  }
}

void tune_into_error() {
  CHAN_D->tune("error", query_privs(), 1);
}

protected void log_error(string file, string message) {
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
  string logfile = caught ? catch_log : runtime_log;
  string what = mp["error"];
  string userid;
  string ret;

  ret = "---\n" + standard_trace(mp, 1);
  write_file(logfile, ret);

  // TODO Temporary notifications, undo when above fixed
  message("error", sprintf("(%s) Error logged %s\n%s\n",
    logfile,
    ret,
    trace_line(mp["object"], mp["program"], mp["file"], mp["line"])
), filter(users(), (: devp :)));
}

#if 0
void error_handler(mapping mp, int caught) {
    string ret;
    string logfile = caught ? mudConfig("LOG_CATCH") : mudConfig("LOG_RUNTIME");
    string what = mp["error"];
    string userid;

    ret = "---\n" + standard_trace(mp, 1);
    write_file(logfile, ret);

    // If an object didn't load, they get compile errors. Don't spam
    // or confuse them
    // if(what[0..23] == "*Error in loading object")
    //     return;

    if(this_body()) {
        userid = query_privs(this_body());
        if(!userid || userid == "")
            userid = "(none)";
        printf("%sTrace written to %s\n", what, logfile);
        errors[userid] = mp;
    } else {
        userid = "(none)";
    }
    errors["last"] = mp;

    // Strip trailing \n, and indent nicely
    what = replace_string(what[0.. < 2], "\n", "\n         *");

    // TODO: This isn't working for some reason, so we'll return to this
    // temporarily notifying all admins of errors in real time, regardless
    // of the kind.
#if 0
    CHAN_D->chat(
        "error",
        query_privs(),
        sprintf("(%s) Error logged %s\n%s\n",
            logfile,
            what,
            trace_line(mp["object"], mp["program"], mp["file"], mp["line"])
        )
    );
#endif
    // TODO: Temporary notifications, undo when above fixed
    // message("error", sprintf("(%s) Error logged %s\n%s\n",
    //     logfile,
    //     ret,
    //     trace_line(mp["object"], mp["program"], mp["file"], mp["line"])
    // ), filter(users(), (: devp :)));

}
#endif
private void crash(string crash_message, object command_giver, object current_object) {
  string mess;

  emit(SIG_SYS_CRASH);

  shout(
    "Master object shouts: Damn!\n"
    "Master object tells you: The game is crashing.\n"
  );

  // This is to allow all pending messages to be printed
  // https://www.fluffos.info/efun/system/flush_messages.html
  flush_messages();

  mess = MUD_NAME + " crashed on: " + ctime(time()) + ", error: " + crash_message + "\n";

  log_file("shutdown", mess);
  log_file("crashes", mess);

  if(command_giver) {
    log_file("crashes", "this_player: " +
      file_name(command_giver) + " :: " +
      query_privs(command_giver) +
      "\n"
    );
  }

  if(current_object)
    log_file("crashes", "this_object: " + file_name(current_object) + "\n");

  shutdown_d()->shutdown(0);
}

// This doesn't actually seem to work and generates *Too long evaluation.
// Execution aborted. errors even though it isn't that complicated.
#if 0
public string get_save_file_name(string file, object who) {
    string temp, e;

    debug_message("Called from previous_object(): " + previous_object());

    e = catch {
        temp = sprintf("/tmp/%s.%d",
            who ? query_privs(who) : "unknown",
            time()
        );
    };

    if(e) {
        debug_message(sprintf("get_save_file_name error: %O", e));
        return null;
    }

    debug_message(sprintf("get_save_file_name new file: %s", temp));

    return temp;
}
#endif

string privs_file(string filename) {
  string temp;

  if(sscanf(filename, "/adm/daemons/%s", temp)) return "[daemon]";
  if(sscanf(filename, "/adm/obj/%s", temp)) return "[adm_obj]";
  if(sscanf(filename, "/adm/%s", temp)) return "[admin]";
  if(sscanf(filename, "/cmds/adm/%s", temp)) return "[cmd_admin]";
  if(sscanf(filename, "/cmds/file/%s", temp)) return "[cmd_file]";
  if(sscanf(filename, "/cmds/object/%s", temp)) return "[cmd_object]";
  if(sscanf(filename, "/cmds/wiz/%s", temp))return "[cmd_wiz]";
  if(sscanf(filename, "/cmds/%s", temp)) return "[cmd]";
  if(sscanf(filename, "/home/%*s/%s/%*s", temp)) return "[home_" + temp + "]";
  if(sscanf(filename, "/open/%s", temp)) return "[open]";
  if(sscanf(filename, "/std/%s", temp)) return "[std_object]";
  if(sscanf(filename, "/obj/mudlib/%s", temp)) return "[mudlib_object]";
  if(sscanf(filename, "/obj/%s", temp)) return "[gen_obj]";
  else return "object";
}

string object_name(object ob) {
  if(ob->query_real_name())
    return ob->query_real_name();

  return 0;
}

mixed compile_object(string file) {
  return VIRTUAL_D->compile_object(file);
}

string make_path_absolute(string file) {
  file = resolve_path(this_body()->query_cwd(), file);

  return file;
}

varargs void log_file(string file, string msg, mixed arg...) {
  int size;
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
  msg = sanitizeRegex(msg);
  msg = sprintf(msg, arg...);
  msg = append(msg, "\n");

  write_file(source, msg);
}

int save_ed_setup(object user, int config) {
  user->set_ed_setup(config);

  return 1;
}

int retrieve_ed_setup(object user) {
  return user->query_ed_setup();
}

mapping get_mud_stats() {
  return MSSP_D->get_mud_stats();
}

string *get_include_path(string object_path) {
  string *DEFAULT_PATH = ({ ":DEFAULT:" });
  string *parts = explode(object_path, "/");
  string *include_path = ({ });

  if(parts[0] == "std") {
    string path1, path2;

    path1 = "/" + implode(parts[0..<2], "/") + "/";
    path2 = "/" + implode(parts[0..<2], "/") + "/include/";
#ifndef __LANG_SVC__
    if(directory_exists(path1))
      include_path += ({ path1 });
    if(directory_exists(path2))
      include_path += ({ path2 });
#else
      include_path += ({ path1 });
      include_path += ({ path2 });
#endif
  }

  return DEFAULT_PATH + include_path;
}
