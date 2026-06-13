/* master.c

 Tacitus @ LPUniversity
 April 15th, 2005
 master object

 Last edited on July 14th, 2006 by Tacitus

*/

#include <localtime.h>
#include <shutdown.h>

inherit __DIR__ "master/ed";
inherit __DIR__ "master/logging";
inherit __DIR__ "master/valid";
inherit __DIR__ "master/testing";

private nosave mapping errors = ([]);

void create() {
  // In master/valid.c
  parse_group();
  parse_access();

  call_out_walltime(function() {
    set_privs(this_object(), "[master]");
  }, 0.01);
}

// -- driver apply
private flag(string str) {
  debug_message("Unknown flag: " + str);
}

// -- driver apply
private object connect(int _port) {
  object login_ob;
  string err;

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

// -- driver apply
private void epilog(int _load_empty) {
  string *lines, err;
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

  emit(SIG_SYS_BOOT);
}

private void crash(string crash_message, object command_giver, object current_object) {
  string mess;

  catch(emit(SIG_SYS_CRASH));

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

  shutdown_d()->start(0, SYS_SHUTDOWN);
}

// -- driver apply
// The privs_file() function is called in the master object when a new
// file is created. The 'filename' of the object is passed as the argu‐
// ment, and the string that privs_file() returns is used as the new
// object's privs string.
string privs_file(string filename) {
  string temp;

  if(sscanf(filename, "/adm/daemons/%s", temp)) return "[daemon]";
  if(sscanf(filename, "/adm/obj/%s", temp)) return "[adm_object]";
  if(sscanf(filename, "/adm/%s", temp)) return "[admin]";
  if(sscanf(filename, "/cmds/adm/%s", temp)) return "[cmd_admin]";
  if(sscanf(filename, "/cmds/file/%s", temp)) return "[cmd_file]";
  if(sscanf(filename, "/cmds/object/%s", temp)) return "[cmd_object]";
  if(sscanf(filename, "/cmds/wiz/%s", temp))return "[cmd_wiz]";
  if(sscanf(filename, "/cmds/%s", temp)) return "[cmd]";
  if(sscanf(filename, "/home/%*s/%s/%*s", temp)) return "[home_" + temp + "]";
  if(sscanf(filename, "/open/%s", temp)) return "[open]";
  if(sscanf(filename, "/std/%s", temp)) return "[std_object]";
  if(sscanf(filename, "/obj/%s", temp)) return "[gen_object]";
  else return "object";
}

// -- driver apply
// This master apply is called by the sprintf() efun, when printing the
// "value" of an object. This function should return a string correspond‐
// ing to the name of the object (eg a user's name).
string object_name(/** @type {STD_OBJECT} */ object ob) {
  if(ob->query_real_name())
    return ob->query_real_name();

  return 0;
}

// -- driver apply
mixed compile_object(string file) {
  return VIRTUAL_D->compile_object(file);
}

// -- driver apply
mapping get_mud_stats() {
  return MSSP_D->get_mud_stats();
}

// -- driver apply
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
