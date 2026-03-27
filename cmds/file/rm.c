/**
 * @file /cmds/file/rm.c
 *
 * File removal command with optional recursive deletion.
 *
 * @created 2005-04-07 - Tacitus
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2005-04-07 - Tacitus - Created
 * 2006-08-18 - Parthenon - Last edited
 * 2026-03-23 - Gesslar - Updated to current conventions
 */

inherit STD_CMD;

private void startDelete();
private void handleDelete(string contents);

private string *dirTree;
private string dir;

void setup() {
  usage_text = "rm [-r] <file name>";
  help_text =
"This command permanently removes a file. Please note "
"that there is no 'Recycle Bin'. You must be extra "
"careful when dealing with important files. You may "
"also use the -r option to recursively remove all "
"files and folders within <dir name>. It will ask you "
"for a confirmation just to be safe.";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string str) {
  int result;

  dirTree = ({});

  if(!str)
    return _usage(caller);

  if(sscanf(str, "-r %s", dir) == 1) {
    dir = resolve_path(
      caller->query_env("cwd"), dir) + "/";

    if(!directory_exists(dir))
      return _error(caller,
        "%s: No such directory.", dir);

    if(!master()->valid_write(dir, caller, "rmdir"))
      return _error(caller, "Permission denied.");

    dirTree += ({ dir });

    tell(caller, "Are you sure you about that? ");
    input_to("confirmRecursiveDelete", caller);

    return 1;
  }

  // Check if there are any glob patterns
  if(of("*", str)) {
    string *files;
    string *failed = ({});
    string path, *parts;

    path = resolve_path(
      caller->query_env("cwd"), str);
    parts = dir_file(path);

    files = get_dir(path);
    files -= ({ ".", ".." });

    if(!sizeof(files))
      return _error(caller,
        "%s: No matching file(s).", path);

    files = map(files, (: $(parts[0]) + $1 :));

    foreach(string file in files) {
      if(!(int)master()->valid_write(
          file, caller, "rm")) {
        _error(caller,
          "%s: Permission denied.", file);
        failed += ({ file });
        continue;
      }

      if(!rm(file)) {
        failed += ({ file });
        _error(caller,
          "Could not remove %s", file);
      } else {
        _info(caller, "%s: removed", file);
      }
    }

    if(sizeof(failed))
      return _error(caller,
        "Failed to remove the following files: %s",
        implode(failed, ", "));

    return _ok(caller,
      "All files removed successfully.");
  }

  str = resolve_path(caller->query_env("cwd"), str);

  if(directory_exists(str) || !file_exists(str))
    return _error(caller, "%s: No such file.", str);

  if(!master()->valid_write(str, caller, "rm"))
    return _error(caller, "Permission denied.");

  result = rm(str);

  if(result)
    return _ok(caller, "File removed: %s", str);
  else
    return _error(caller,
      "Could not remove file: %s", str);
}

private int confirmRecursiveDelete(string arg,
    object caller) {
  if(!arg || arg == ""
  || member_array(lower_case(arg),
      ({ "y", "yes" })) == -1)
    return _info(caller, "Deletion cancelled.");

  startDelete();
}

private void startDelete() {
  mixed *contents;

  do {
    contents = get_dir(dir);

    if(sizeof(contents) > 0)
      handleDelete(contents[0]);
  } while(sizeof(contents) > 0);

  rmdir(dir)
    ? _ok("All files and directories deleted "
        "successfully.")
    : _error("All files and directories could "
        "not be deleted.");
}

private void handleDelete(string contents) {
  if(file_size(implode(dirTree, "") + contents)
      == -2) {
    dirTree += ({ contents + "/" });

    if(sizeof(get_dir(implode(dirTree, "")))
        == 0) {
      if(rmdir(implode(dirTree, ""))) {
        dirTree -= ({ contents + "/" });
        return;
      }
    } else {
      handleDelete(
        get_dir(implode(dirTree, ""))[0]);
      dirTree -= ({ contents + "/" });
      return;
    }
  } else if(file_size(
      implode(dirTree, "") + contents) == -1) {
    dirTree -= ({ contents + "/" });
  } else {
    rm(implode(dirTree, "") + contents);
  }
}
