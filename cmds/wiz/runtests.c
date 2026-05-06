/**
 * @file /cmds/wiz/runtests.c
 * Run unit tests under /tests/. With no argument, walks /tests/
 * recursively and invokes every runner.c found. With an argument,
 * invokes /tests/<arg>/runner directly.
 *
 * SYNTAX:
 *   runtests
 *   runtests adm/simul_efun
 */

#include <mudlib.h>

inherit STD_CMD;

private void invoke_runner(object tp, string runner_path);
private void walk(object tp, string dir);

mixed main(object tp, string arg) {
  if(arg && strlen(arg)) {
    string trimmed = trim(arg);
    string path = "/tests/" + trimmed + "/runner";

    if(file_size(path + ".c") < 0)
      return notify_fail(sprintf(
        "No runner found at %s.c\n", path));

    invoke_runner(tp, path);
    return 1;
  }

  if(file_size("/tests") != -2) {
    tell(tp, "No /tests/ directory found.\n");
    return 1;
  }

  walk(tp, "/tests/");
  return 1;
}

private void invoke_runner(object tp, string runner_path) {
  string err = catch(runner_path->run_tests());
  if(err)
    tell(tp, sprintf("Runner %s failed: %s\n", runner_path, err));
}

private void walk(object tp, string dir) {
  string *entries = get_dir(dir);

  foreach(string entry in entries) {
    string path;
    int sz;

    if(entry == "." || entry == "..")
      continue;

    path = dir + entry;
    sz = file_size(path);

    if(sz == -2) {
      walk(tp, path + "/");
    } else if(entry == "runner.c") {
      invoke_runner(tp, path[0..<3]);
    }
  }
}

string help(object _caller) {
  return(
    " SYNTAX: runtests [<area>]\n\n"
    "Run unit test suites under /tests/.\n\n"
    "With no argument, walks /tests/ recursively and invokes every\n"
    "runner.c it finds.\n\n"
    "With an argument, invokes /tests/<area>/runner. For example:\n"
    "    runtests adm/simul_efun\n");
}
